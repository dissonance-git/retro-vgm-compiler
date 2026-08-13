#!/usr/bin/env python3
"""Audit Sonic 3 & Knuckles VGM FM key-ons against the S&K SMPS pitch table.

This is an inverse-analysis diagnostic, not a source-token recovery tool.
It asks which transposed SMPS table pitches and signed frequency-code
residuals are compatible with each observed ordinary YM2612 key-on, then
uses within-channel continuity to see when one persistent displacement model
survives across consecutive notes.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import json
import pathlib
import struct
from dataclasses import dataclass
from typing import Iterable

# Sonic & Knuckles zFMFrequencies, derived from sonicretro/skdisasm commit
# 9fad8e21b6ca86f3e8fb654d9519d83a45b3e1f9. The disassembly builds these
# one-octave C..B FNUM values with zMakeFMFrequency and applies the octave at
# runtime. Keep the exact driver table as the reference rather than rounding
# VGM output to a generic equal-tempered note grid.
S3K_FM_FIRST_OCTAVE = (
    0x0284, 0x02AB, 0x02D3, 0x02FE, 0x032D, 0x035C,
    0x038F, 0x03C5, 0x03FF, 0x043C, 0x047C, 0x04C0,
)
SIGNED_BYTE_MIN = -128
SIGNED_BYTE_MAX = 127


@dataclass(frozen=True)
class KeyOn:
    file: str
    channel: int
    fnum: int
    block: int

    @property
    def frequency_code(self) -> int:
        return (self.block << 11) | self.fnum


def table_entries() -> tuple[tuple[int, int], ...]:
    return tuple(
        (octave * 12 + pitch_class, (octave << 11) | fnum)
        for octave in range(8)
        for pitch_class, fnum in enumerate(S3K_FM_FIRST_OCTAVE)
    )


TABLE_ENTRIES = table_entries()


def displacement_candidates(frequency_code: int) -> set[int]:
    """Return signed-byte residuals mapping some S&K table pitch to the code.

    A residual is only a source-compatible frequency-displacement hypothesis.
    VGM alone does not prove that it was the SMPS Detune byte, nor does it
    recover the pre-transposition sequence token.
    """
    return {
        frequency_code - table_code
        for _, table_code in TABLE_ENTRIES
        if SIGNED_BYTE_MIN <= frequency_code - table_code <= SIGNED_BYTE_MAX
    }


def pitch_for_displacement(frequency_code: int, displacement: int) -> int | None:
    """Return the unique transposed table pitch under one displacement model."""
    target = frequency_code - displacement
    matches = [index for index, table_code in TABLE_ENTRIES if table_code == target]
    if len(matches) != 1:
        return None
    return matches[0]


def data_offset(raw: bytes) -> int:
    if raw[:4] != b"Vgm ":
        raise ValueError("not a VGM stream")
    version = struct.unpack_from("<I", raw, 8)[0]
    if version < 0x150:
        return 0x40
    relative = struct.unpack_from("<I", raw, 0x34)[0]
    return 0x40 if relative == 0 else 0x34 + relative


def command_stream(raw: bytes) -> Iterable[tuple[int, tuple[int, ...]]]:
    pos = data_offset(raw)
    size = len(raw)
    while pos < size:
        command = raw[pos]
        pos += 1
        if command == 0x66:
            return
        if command in (0x52, 0x53):
            yield command, (raw[pos], raw[pos + 1])
            pos += 2
        elif command in (0x4F, 0x50):
            pos += 1
        elif command == 0x61:
            pos += 2
        elif command in (0x62, 0x63):
            pass
        elif command == 0x67:
            if raw[pos] != 0x66:
                raise ValueError("malformed VGM data block")
            block_size = struct.unpack_from("<I", raw, pos + 2)[0]
            pos += 6 + block_size
        elif 0x70 <= command <= 0x8F:
            pass
        elif command == 0xE0:
            pos += 4
        else:
            raise ValueError(f"unsupported VGM command 0x{command:02X}")


def extract_key_ons(path: pathlib.Path) -> list[KeyOn]:
    raw = gzip.decompress(path.read_bytes())
    state = [{"low": 0, "high": 0} for _ in range(6)]
    dac_enabled = False
    result: list[KeyOn] = []
    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

    for command, args in command_stream(raw):
        if command not in (0x52, 0x53):
            continue
        register, value = args
        port = 0 if command == 0x52 else 1

        if port == 0 and register == 0x2B:
            dac_enabled = bool(value & 0x80)

        if 0xA0 <= register <= 0xA2:
            channel = register - 0xA0 + port * 3
            state[channel]["low"] = value
        elif 0xA4 <= register <= 0xA6:
            channel = register - 0xA4 + port * 3
            state[channel]["high"] = value
        elif port == 0 and register == 0x28 and (value & 0xF0) == 0xF0:
            encoded_channel = value & 0x07
            if encoded_channel not in channel_map:
                continue
            channel = channel_map[encoded_channel]
            if channel == 5 and dac_enabled:
                continue
            high = state[channel]["high"]
            fnum = ((high & 0x07) << 8) | state[channel]["low"]
            block = (high >> 3) & 0x07
            if fnum:
                result.append(KeyOn(path.name, channel, fnum, block))

    return result


def continuity_segments(events: list[KeyOn]) -> list[dict[str, object]]:
    grouped: dict[tuple[str, int], list[KeyOn]] = collections.defaultdict(list)
    for event in events:
        grouped[(event.file, event.channel)].append(event)

    segments: list[dict[str, object]] = []
    for (file, channel), channel_events in grouped.items():
        start = 0
        surviving: set[int] | None = None
        for index, event in enumerate(channel_events):
            current = displacement_candidates(event.frequency_code)
            narrowed = current if surviving is None else surviving & current
            if surviving is not None and not narrowed:
                segments.append({
                    "file": file,
                    "channel": channel,
                    "start_key_on": start,
                    "end_key_on": index,
                    "key_ons": index - start,
                    "compatible_displacements": sorted(surviving),
                })
                start = index
                surviving = current
            else:
                surviving = narrowed
        if surviving is not None:
            segments.append({
                "file": file,
                "channel": channel,
                "start_key_on": start,
                "end_key_on": len(channel_events),
                "key_ons": len(channel_events) - start,
                "compatible_displacements": sorted(surviving),
            })
    return segments


def audit(corpus: pathlib.Path) -> dict[str, object]:
    files = sorted(corpus.glob("*.vgz"))
    if not files:
        raise SystemExit(f"no .vgz files found in {corpus}")

    events = [event for path in files for event in extract_key_ons(path)]
    candidate_histogram = collections.Counter(
        len(displacement_candidates(event.frequency_code)) for event in events
    )
    segments = continuity_segments(events)
    unique_segments = [s for s in segments if len(s["compatible_displacements"]) == 1]
    unique_event_count = sum(int(s["key_ons"]) for s in unique_segments)

    displacement_by_events: collections.Counter[int] = collections.Counter()
    for segment in unique_segments:
        displacement = int(segment["compatible_displacements"][0])
        displacement_by_events[displacement] += int(segment["key_ons"])

    return {
        "model": "Sonic 3 & Knuckles SMPS FM table + persistent signed frequency displacement",
        "claim_boundary": (
            "A unique segment identifies one transposed table-pitch trajectory under this model. "
            "It does not prove the original SMPS note token, track transposition, spelling, Detune byte, "
            "or heard pitch independently."
        ),
        "files": len(files),
        "ordinary_full_fm_key_ons": len(events),
        "single_key_on_displacement_candidate_count_histogram": {
            str(key): value for key, value in sorted(candidate_histogram.items())
        },
        "continuity_segments": len(segments),
        "unique_displacement_segments": len(unique_segments),
        "unique_model_key_ons": unique_event_count,
        "unique_model_key_on_coverage_percent": (
            100.0 * unique_event_count / len(events) if events else 0.0
        ),
        "unique_displacement_key_on_histogram": {
            str(key): value for key, value in sorted(displacement_by_events.items())
        },
        "segments": segments,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpus", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()
    result = audit(args.corpus)
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
