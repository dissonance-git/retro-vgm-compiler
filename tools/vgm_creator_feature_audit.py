#!/usr/bin/env python3
"""Blind Genesis VGM feature audit for creator-attribution research.

This tool extracts two deliberately separate views:

1. musical_trajectory:
   transposition-tolerant relative pitch/onset relations observed at ordinary
   YM2612 full key-ons on physical channels.
2. realization:
   YM2612 patch/control/channel, PSG, DAC and routing behavior.

It never reads composer/artist tags and does not claim either view is original
notation, persistent-part identity, or composer identity.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import hashlib
import json
import math
import pathlib
import statistics
import struct
from dataclasses import dataclass
from typing import Iterable


@dataclass(frozen=True)
class Command:
    tick: int
    opcode: int
    args: tuple[int, ...]


@dataclass(frozen=True)
class FmOnset:
    tick: int
    channel: int
    fnum: int
    block: int
    patch_core: str
    patch_full: str
    algorithm: int
    feedback: int
    ams: int
    fms: int
    pan: int

    @property
    def frequency_measure(self) -> float:
        return float(self.fnum) * (2.0 ** self.block)


def data_offset(raw: bytes) -> int:
    if len(raw) < 0x40 or raw[:4] != b"Vgm ":
        raise ValueError("not a VGM stream")
    version = struct.unpack_from("<I", raw, 8)[0]
    if version < 0x150:
        return 0x40
    relative = struct.unpack_from("<I", raw, 0x34)[0]
    return 0x40 if relative == 0 else 0x34 + relative


def command_stream(raw: bytes) -> Iterable[Command]:
    pos = data_offset(raw)
    size = len(raw)
    tick = 0

    def need(count: int) -> None:
        if pos + count > size:
            raise ValueError("truncated VGM command")

    while pos < size:
        opcode = raw[pos]
        pos += 1

        if opcode == 0x66:
            return
        if opcode in (0x52, 0x53):
            need(2)
            args = (raw[pos], raw[pos + 1])
            pos += 2
            yield Command(tick, opcode, args)
        elif opcode in (0x4F, 0x50):
            need(1)
            args = (raw[pos],)
            pos += 1
            yield Command(tick, opcode, args)
        elif opcode == 0x61:
            need(2)
            wait = struct.unpack_from("<H", raw, pos)[0]
            pos += 2
            tick += wait
        elif opcode == 0x62:
            tick += 735
        elif opcode == 0x63:
            tick += 882
        elif 0x70 <= opcode <= 0x7F:
            tick += (opcode & 0x0F) + 1
        elif 0x80 <= opcode <= 0x8F:
            # YM2612 DAC stream write plus encoded wait. The data write itself
            # is not reconstructed here; timing and DAC-stream usage are.
            yield Command(tick, opcode, ())
            tick += opcode & 0x0F
        elif opcode == 0x67:
            need(6)
            if raw[pos] != 0x66:
                raise ValueError("malformed VGM data block")
            block_size = struct.unpack_from("<I", raw, pos + 2)[0]
            pos += 6
            need(block_size)
            pos += block_size
        elif opcode == 0xE0:
            need(4)
            pos += 4
        elif opcode == 0x90:
            need(4)
            args = tuple(raw[pos : pos + 4])
            pos += 4
            yield Command(tick, opcode, args)
        elif opcode == 0x91:
            need(4)
            args = tuple(raw[pos : pos + 4])
            pos += 4
            yield Command(tick, opcode, args)
        elif opcode == 0x92:
            need(5)
            args = tuple(raw[pos : pos + 5])
            pos += 5
            yield Command(tick, opcode, args)
        elif opcode == 0x93:
            need(10)
            args = tuple(raw[pos : pos + 10])
            pos += 10
            yield Command(tick, opcode, args)
        elif opcode == 0x94:
            need(1)
            args = (raw[pos],)
            pos += 1
            yield Command(tick, opcode, args)
        elif opcode == 0x95:
            need(4)
            args = tuple(raw[pos : pos + 4])
            pos += 4
            yield Command(tick, opcode, args)
        else:
            raise ValueError(f"unsupported Genesis VGM command 0x{opcode:02X}")


def _fingerprint(values: Iterable[int]) -> str:
    return hashlib.sha1(bytes(v & 0xFF for v in values)).hexdigest()[:16]


def _quantized_interval(previous: FmOnset, current: FmOnset) -> str | None:
    a = previous.frequency_measure
    b = current.frequency_measure
    if a <= 0.0 or b <= 0.0:
        return None
    semitones = 12.0 * math.log2(b / a)
    rounded = int(round(semitones))
    if rounded < -24:
        return "<-24"
    if rounded > 24:
        return ">24"
    return str(rounded)


def _histogram(values: Iterable[str]) -> dict[str, int]:
    counts = collections.Counter(values)
    return {key: counts[key] for key in sorted(counts)}


def _cosine(lhs: dict[str, int], rhs: dict[str, int]) -> float | None:
    keys = set(lhs) | set(rhs)
    if not keys:
        return None
    dot = sum(lhs.get(k, 0) * rhs.get(k, 0) for k in keys)
    left = math.sqrt(sum(lhs.get(k, 0) ** 2 for k in keys))
    right = math.sqrt(sum(rhs.get(k, 0) ** 2 for k in keys))
    if left == 0.0 or right == 0.0:
        return None
    return dot / (left * right)


def _jaccard(lhs: set[str], rhs: set[str]) -> float | None:
    union = lhs | rhs
    if not union:
        return None
    return len(lhs & rhs) / len(union)


def _mean_available(values: Iterable[float | None]) -> float:
    present = [value for value in values if value is not None]
    if not present:
        return 0.0
    return sum(present) / len(present)


class GenesisAuditState:
    def __init__(self) -> None:
        self.pitch_low = [0] * 6
        self.pitch_high = [0] * 6
        self.operator = [[[0] * 7 for _ in range(4)] for _ in range(6)]
        self.algorithm_feedback = [0] * 6
        self.route_ams_fms = [0] * 6
        self.dac_enabled = False
        self.ch3_special_mode = False

    @staticmethod
    def channel_for_port_register(port: int, register: int, base: int) -> int | None:
        local = register - base
        if not 0 <= local <= 2:
            return None
        return local + port * 3

    def update(self, port: int, register: int, value: int) -> None:
        if port == 0 and register == 0x2B:
            self.dac_enabled = bool(value & 0x80)
            return
        if port == 0 and register == 0x27:
            self.ch3_special_mode = bool(value & 0x40)
            return

        if 0xA0 <= register <= 0xA2:
            channel = self.channel_for_port_register(port, register, 0xA0)
            if channel is not None:
                self.pitch_low[channel] = value
            return
        if 0xA4 <= register <= 0xA6:
            channel = self.channel_for_port_register(port, register, 0xA4)
            if channel is not None:
                self.pitch_high[channel] = value
            return

        if 0xB0 <= register <= 0xB2:
            channel = self.channel_for_port_register(port, register, 0xB0)
            if channel is not None:
                self.algorithm_feedback[channel] = value
            return
        if 0xB4 <= register <= 0xB6:
            channel = self.channel_for_port_register(port, register, 0xB4)
            if channel is not None:
                self.route_ams_fms[channel] = value
            return

        group_bases = (0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90)
        high = register & 0xF0
        if high not in group_bases:
            return
        local = register & 0x0F
        channel_index = local & 0x03
        if channel_index == 3:
            return
        slot = (local >> 2) & 0x03
        channel = channel_index + port * 3
        group_index = group_bases.index(high)
        self.operator[channel][slot][group_index] = value

    def onset(self, tick: int, channel: int) -> FmOnset | None:
        if channel == 5 and self.dac_enabled:
            return None
        if channel == 2 and self.ch3_special_mode:
            return None
        high = self.pitch_high[channel]
        fnum = ((high & 0x07) << 8) | self.pitch_low[channel]
        block = (high >> 3) & 0x07
        if fnum == 0:
            return None

        af = self.algorithm_feedback[channel]
        route = self.route_ams_fms[channel]
        algorithm = af & 0x07
        feedback = (af >> 3) & 0x07
        ams = (route >> 4) & 0x03
        fms = route & 0x07
        pan = (route >> 6) & 0x03

        core_values: list[int] = [algorithm, feedback]
        full_values: list[int] = [algorithm, feedback, ams, fms]
        for slot in range(4):
            params = self.operator[channel][slot]
            # Core excludes TL (group index 1), which drivers often modify for
            # channel volume. Full retains it as a realization-sensitive view.
            core_values.extend(params[index] for index in (0, 2, 3, 4, 5, 6))
            full_values.extend(params)

        return FmOnset(
            tick=tick,
            channel=channel,
            fnum=fnum,
            block=block,
            patch_core=_fingerprint(core_values),
            patch_full=_fingerprint(full_values),
            algorithm=algorithm,
            feedback=feedback,
            ams=ams,
            fms=fms,
            pan=pan,
        )


def audit_file(path: pathlib.Path) -> dict[str, object]:
    raw = gzip.decompress(path.read_bytes()) if path.suffix.lower() == ".vgz" else path.read_bytes()
    state = GenesisAuditState()

    onsets: list[FmOnset] = []
    psg_writes = 0
    psg_noise_writes = 0
    stereo_writes = 0
    lfo_writes = 0
    pan_changes = 0
    dac_enable_writes = 0
    dac_stream_commands = 0
    last_pan = [0] * 6
    last_tick = 0

    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

    for command in command_stream(raw):
        last_tick = max(last_tick, command.tick)

        if command.opcode == 0x50:
            psg_writes += 1
            value = command.args[0]
            if (value & 0xF0) == 0xE0:
                psg_noise_writes += 1
            continue
        if command.opcode == 0x4F:
            stereo_writes += 1
            continue
        if 0x80 <= command.opcode <= 0x8F or 0x90 <= command.opcode <= 0x95:
            dac_stream_commands += 1
            continue
        if command.opcode not in (0x52, 0x53):
            continue

        register, value = command.args
        port = 0 if command.opcode == 0x52 else 1

        if port == 0 and register == 0x22:
            lfo_writes += 1
        if port == 0 and register == 0x2B:
            dac_enable_writes += 1

        if 0xB4 <= register <= 0xB6:
            channel = register - 0xB4 + port * 3
            pan = (value >> 6) & 0x03
            if pan != last_pan[channel]:
                pan_changes += 1
                last_pan[channel] = pan

        state.update(port, register, value)

        if port == 0 and register == 0x28:
            key_mask = value & 0xF0
            encoded_channel = value & 0x07
            if key_mask != 0xF0 or encoded_channel not in channel_map:
                continue
            channel = channel_map[encoded_channel]
            onset = state.onset(command.tick, channel)
            if onset is not None:
                onsets.append(onset)

    by_channel: dict[int, list[FmOnset]] = collections.defaultdict(list)
    for onset in onsets:
        by_channel[onset.channel].append(onset)

    interval_values: list[str] = []
    interval_bigrams: list[str] = []
    contour_values: list[str] = []
    normalized_gap_values: list[str] = []

    for events in by_channel.values():
        intervals: list[str] = []
        for previous, current in zip(events, events[1:]):
            interval = _quantized_interval(previous, current)
            if interval is None:
                continue
            intervals.append(interval)
            interval_values.append(interval)
            numeric = None
            if interval not in ("<-24", ">24"):
                numeric = int(interval)
            contour_values.append(
                "up" if interval == ">24" or (numeric is not None and numeric > 0)
                else "down" if interval == "<-24" or (numeric is not None and numeric < 0)
                else "same"
            )
        for left, right in zip(intervals, intervals[1:]):
            interval_bigrams.append(f"{left},{right}")

        gaps = [
            current.tick - previous.tick
            for previous, current in zip(events, events[1:])
            if current.tick > previous.tick
        ]
        if gaps:
            median_gap = statistics.median(gaps)
            if median_gap > 0:
                for gap in gaps:
                    ratio = min(4.0, gap / median_gap)
                    quantized = round(ratio * 4.0) / 4.0
                    normalized_gap_values.append(f"{quantized:.2f}")

    core_patch_usage = collections.Counter(onset.patch_core for onset in onsets)
    full_patch_usage = collections.Counter(onset.patch_full for onset in onsets)
    algorithm_hist = collections.Counter(str(onset.algorithm) for onset in onsets)
    feedback_hist = collections.Counter(str(onset.feedback) for onset in onsets)
    pan_hist = collections.Counter(str(onset.pan) for onset in onsets)
    channel_hist = collections.Counter(str(onset.channel) for onset in onsets)

    duration_seconds = last_tick / 44100.0 if last_tick > 0 else 0.0
    density = len(onsets) / duration_seconds if duration_seconds > 0.0 else 0.0
    normalizer = max(1, len(onsets))

    return {
        "file": path.name,
        "claim_boundary": (
            "musical_trajectory is inferred from ordinary full YM2612 key-ons on physical channels; "
            "it is not original notation or persistent-part identity. realization features describe "
            "observed Genesis execution and do not by themselves establish composer identity."
        ),
        "duration_vgm_samples": last_tick,
        "ordinary_full_fm_key_ons": len(onsets),
        "musical_trajectory": {
            "interval_histogram_semitones": _histogram(interval_values),
            "interval_bigram_histogram": _histogram(interval_bigrams),
            "contour_histogram": _histogram(contour_values),
            "normalized_onset_gap_histogram": _histogram(normalized_gap_values),
            "physical_channel_key_on_histogram": {
                key: channel_hist[key] for key in sorted(channel_hist)
            },
            "fm_key_on_density_per_second": density,
        },
        "realization": {
            "core_patch_usage": {
                key: core_patch_usage[key] for key in sorted(core_patch_usage)
            },
            "full_patch_usage": {
                key: full_patch_usage[key] for key in sorted(full_patch_usage)
            },
            "unique_core_patches": len(core_patch_usage),
            "unique_full_patches": len(full_patch_usage),
            "algorithm_histogram": {
                key: algorithm_hist[key] for key in sorted(algorithm_hist)
            },
            "feedback_histogram": {
                key: feedback_hist[key] for key in sorted(feedback_hist)
            },
            "pan_histogram": {key: pan_hist[key] for key in sorted(pan_hist)},
            "lfo_writes_per_1000_key_ons": 1000.0 * lfo_writes / normalizer,
            "pan_changes_per_1000_key_ons": 1000.0 * pan_changes / normalizer,
            "psg_writes_per_1000_key_ons": 1000.0 * psg_writes / normalizer,
            "psg_noise_writes_per_1000_key_ons": 1000.0 * psg_noise_writes / normalizer,
            "stereo_writes_per_1000_key_ons": 1000.0 * stereo_writes / normalizer,
            "dac_enable_writes": dac_enable_writes,
            "dac_stream_commands": dac_stream_commands,
        },
    }


def structural_similarity(lhs: dict[str, object], rhs: dict[str, object]) -> float:
    a = lhs["musical_trajectory"]
    b = rhs["musical_trajectory"]
    assert isinstance(a, dict) and isinstance(b, dict)
    return _mean_available(
        (
            _cosine(a["interval_histogram_semitones"], b["interval_histogram_semitones"]),
            _cosine(a["interval_bigram_histogram"], b["interval_bigram_histogram"]),
            _cosine(a["normalized_onset_gap_histogram"], b["normalized_onset_gap_histogram"]),
            _cosine(a["contour_histogram"], b["contour_histogram"]),
        )
    )


def realization_similarity(lhs: dict[str, object], rhs: dict[str, object]) -> float:
    a = lhs["realization"]
    b = rhs["realization"]
    assert isinstance(a, dict) and isinstance(b, dict)

    patch_cosine = _cosine(a["core_patch_usage"], b["core_patch_usage"])
    patch_jaccard = _jaccard(set(a["core_patch_usage"]), set(b["core_patch_usage"]))
    algorithm = _cosine(a["algorithm_histogram"], b["algorithm_histogram"])
    feedback = _cosine(a["feedback_histogram"], b["feedback_histogram"])
    pan = _cosine(a["pan_histogram"], b["pan_histogram"])
    return _mean_available((patch_cosine, patch_jaccard, algorithm, feedback, pan))


def _neighbors(
    items: list[dict[str, object]],
    score_fn,
    limit: int,
) -> dict[str, list[dict[str, object]]]:
    result: dict[str, list[dict[str, object]]] = {}
    for index, item in enumerate(items):
        candidates: list[tuple[float, str]] = []
        for other_index, other in enumerate(items):
            if index == other_index:
                continue
            score = score_fn(item, other)
            candidates.append((score, str(other["file"])))
        candidates.sort(key=lambda value: (-value[0], value[1]))
        result[str(item["file"])] = [
            {"file": name, "score": score}
            for score, name in candidates[:limit]
        ]
    return result


def audit_corpus(corpus: pathlib.Path, neighbor_count: int = 5) -> dict[str, object]:
    paths = sorted(
        path for path in corpus.iterdir()
        if path.is_file() and path.suffix.lower() in (".vgm", ".vgz")
    )
    if not paths:
        raise ValueError(f"no VGM/VGZ files found in {corpus}")

    tracks = [audit_file(path) for path in paths]
    return {
        "model": "blind Genesis VGM musical-trajectory + realization audit",
        "label_policy": (
            "No embedded composer/artist metadata is read. Filenames are retained only as track identifiers."
        ),
        "claim_boundary": (
            "Structural similarity uses physical-channel full-key-on trajectories and is not yet persistent-part "
            "or notation-level similarity. Realization similarity is kept separate and must not be interpreted "
            "as composer similarity without role-scoped cross-soundtrack evidence."
        ),
        "tracks": tracks,
        "top_structural_neighbors": _neighbors(
            tracks, structural_similarity, neighbor_count
        ),
        "top_realization_neighbors": _neighbors(
            tracks, realization_similarity, neighbor_count
        ),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpus", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path)
    parser.add_argument("--neighbors", type=int, default=5)
    args = parser.parse_args()

    result = audit_corpus(args.corpus, max(0, args.neighbors))
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
