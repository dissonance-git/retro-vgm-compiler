#!/usr/bin/env python3
"""Persistent creator-blind song capsules for Genesis VGM/VGZ research.

The expensive operation is parsing the source stream. Do it once, cache a
creator-blind analysis capsule, and derive later hypotheses from that capsule.
Creator/role evidence is deliberately stored in a separate JSONL index.

This is a research cache, not a replacement for the immutable corpus files.
It intentionally does not hash source files in the hot path. A capsule is
reused when its extractor version and recorded source size still match; use
--refresh when a source object or extractor semantics intentionally changes.
"""
from __future__ import annotations

import argparse
import collections
import gzip
import json
import math
import pathlib
import statistics
import struct
from dataclasses import dataclass
from typing import Iterable, Iterator

SCHEMA_VERSION = 3
EXTRACTOR_NAME = "creator-blind-genesis-song-capsule"
EXTRACTOR_VERSION = 3
DEFAULT_CACHE_ROOT = pathlib.Path("research/cache/vgm-song-capsules")
DEFAULT_CACHE_INDEX = DEFAULT_CACHE_ROOT / "index.jsonl"


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
    patch_core: tuple[int, ...]
    patch_full: tuple[int, ...]
    algorithm: int
    feedback: int
    ams: int
    fms: int
    pan: int
    key_gate_event_index: int

    @property
    def frequency_measure(self) -> float:
        return float(self.fnum) * (2.0 ** self.block)


def _data_offset(raw: bytes) -> int:
    if len(raw) < 0x40 or raw[:4] != b"Vgm ":
        raise ValueError("not a VGM stream")
    version = struct.unpack_from("<I", raw, 8)[0]
    if version < 0x150:
        return 0x40
    relative = struct.unpack_from("<I", raw, 0x34)[0]
    return 0x40 if relative == 0 else 0x34 + relative


def _command_stream(raw: bytes) -> Iterator[Command]:
    pos = _data_offset(raw)
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
            tick += struct.unpack_from("<H", raw, pos)[0]
            pos += 2
        elif opcode == 0x62:
            tick += 735
        elif opcode == 0x63:
            tick += 882
        elif 0x70 <= opcode <= 0x7F:
            tick += (opcode & 0x0F) + 1
        elif 0x80 <= opcode <= 0x8F:
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
        elif opcode in (0x90, 0x91, 0x95):
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
        else:
            raise ValueError(f"unsupported Genesis VGM command 0x{opcode:02X}")


class _GenesisState:
    def __init__(self) -> None:
        self.pitch_low = [0] * 6
        self.pitch_high = [0] * 6
        self.operator = [[[0] * 7 for _ in range(4)] for _ in range(6)]
        self.algorithm_feedback = [0] * 6
        self.route_ams_fms = [0] * 6
        self.dac_enabled = False
        self.ch3_special_mode = False

    @staticmethod
    def _channel(port: int, register: int, base: int) -> int | None:
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
            channel = self._channel(port, register, 0xA0)
            if channel is not None:
                self.pitch_low[channel] = value
            return
        if 0xA4 <= register <= 0xA6:
            channel = self._channel(port, register, 0xA4)
            if channel is not None:
                self.pitch_high[channel] = value
            return
        if 0xB0 <= register <= 0xB2:
            channel = self._channel(port, register, 0xB0)
            if channel is not None:
                self.algorithm_feedback[channel] = value
            return
        if 0xB4 <= register <= 0xB6:
            channel = self._channel(port, register, 0xB4)
            if channel is not None:
                self.route_ams_fms[channel] = value
            return

        groups = (0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90)
        high = register & 0xF0
        if high not in groups:
            return
        local = register & 0x0F
        channel_index = local & 0x03
        if channel_index == 3:
            return
        slot = (local >> 2) & 0x03
        channel = channel_index + port * 3
        self.operator[channel][slot][groups.index(high)] = value

    def onset(self, tick: int, channel: int, key_gate_event_index: int) -> FmOnset | None:
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

        core: list[int] = [algorithm, feedback]
        full: list[int] = [algorithm, feedback, ams, fms]
        for slot in range(4):
            params = self.operator[channel][slot]
            core.extend(params[i] for i in (0, 2, 3, 4, 5, 6))
            full.extend(params)
        return FmOnset(
            tick, channel, fnum, block, tuple(core), tuple(full),
            algorithm, feedback, ams, fms, pan, key_gate_event_index,
        )


def _interval_token(previous: FmOnset, current: FmOnset) -> str | None:
    a, b = previous.frequency_measure, current.frequency_measure
    if a <= 0 or b <= 0:
        return None
    rounded = int(round(12.0 * math.log2(b / a)))
    if rounded < -24:
        return "<-24"
    if rounded > 24:
        return ">24"
    return str(rounded)


def _dictionary_encode(values: Iterable[tuple[int, ...]]) -> tuple[list[list[int]], list[int]]:
    dictionary: list[tuple[int, ...]] = []
    ids: dict[tuple[int, ...], int] = {}
    encoded: list[int] = []
    for value in values:
        index = ids.get(value)
        if index is None:
            index = len(dictionary)
            ids[value] = index
            dictionary.append(value)
        encoded.append(index)
    return [list(value) for value in dictionary], encoded


def _source_bytes(path: pathlib.Path) -> bytes:
    packed = path.read_bytes()
    return gzip.decompress(packed) if path.suffix.lower() == ".vgz" else packed


def extract_capsule(path: pathlib.Path, *, corpus_id: str | None = None) -> dict[str, object]:
    raw = _source_bytes(path)
    state = _GenesisState()
    onsets: list[FmOnset] = []
    key_gate_ticks: list[int] = []
    key_gate_channels: list[int] = []
    key_gate_masks: list[int] = []
    psg_ticks: list[int] = []
    psg_values: list[int] = []
    stereo_ticks: list[int] = []
    stereo_values: list[int] = []
    dac_stream: list[list[object]] = []
    counters = collections.Counter()
    last_pan = [0] * 6
    last_tick = 0
    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

    for command in _command_stream(raw):
        last_tick = max(last_tick, command.tick)
        if command.opcode == 0x50:
            value = command.args[0]
            psg_ticks.append(command.tick)
            psg_values.append(value)
            counters["psg_writes"] += 1
            if (value & 0xF0) == 0xE0:
                counters["psg_noise_writes"] += 1
            continue
        if command.opcode == 0x4F:
            stereo_ticks.append(command.tick)
            stereo_values.append(command.args[0])
            counters["stereo_writes"] += 1
            continue
        if 0x80 <= command.opcode <= 0x8F or 0x90 <= command.opcode <= 0x95:
            counters["dac_stream_commands"] += 1
            dac_stream.append([command.tick, command.opcode, list(command.args)])
            continue
        if command.opcode not in (0x52, 0x53):
            continue

        register, value = command.args
        port = 0 if command.opcode == 0x52 else 1
        if port == 0 and register == 0x22:
            counters["lfo_writes"] += 1
        if port == 0 and register == 0x2B:
            counters["dac_enable_writes"] += 1
        if 0xB4 <= register <= 0xB6:
            channel = register - 0xB4 + port * 3
            pan = (value >> 6) & 0x03
            if pan != last_pan[channel]:
                counters["pan_changes"] += 1
                last_pan[channel] = pan

        state.update(port, register, value)
        if port == 0 and register == 0x28:
            encoded_channel = value & 0x07
            if encoded_channel not in channel_map:
                continue
            channel = channel_map[encoded_channel]
            key_gate_event_index = len(key_gate_ticks)
            key_gate_ticks.append(command.tick)
            key_gate_channels.append(channel)
            key_gate_masks.append(value & 0xF0)
            counters["fm_key_gate_writes"] += 1
            if (value & 0xF0) != 0xF0:
                continue
            onset = state.onset(command.tick, channel, key_gate_event_index)
            if onset is not None:
                onsets.append(onset)

    core_dict, core_ids = _dictionary_encode(onset.patch_core for onset in onsets)
    full_dict, full_ids = _dictionary_encode(onset.patch_full for onset in onsets)

    channels: list[dict[str, object]] = []
    for channel in range(6):
        indices = [i for i, onset in enumerate(onsets) if onset.channel == channel]
        if not indices:
            continue
        events = [onsets[i] for i in indices]
        intervals = [
            token
            for previous, current in zip(events, events[1:])
            if (token := _interval_token(previous, current)) is not None
        ]
        gaps = [current.tick - previous.tick for previous, current in zip(events, events[1:])]
        positive_gaps = [gap for gap in gaps if gap > 0]
        channels.append({
            "channel": channel,
            "event_indices": indices,
            "interval_tokens": intervals,
            "onset_gap_samples": gaps,
            "median_positive_onset_gap_samples": statistics.median(positive_gaps) if positive_gaps else None,
        })

    event_columns = {
        "tick": [o.tick for o in onsets],
        "channel": [o.channel for o in onsets],
        "fnum": [o.fnum for o in onsets],
        "block": [o.block for o in onsets],
        "patch_core_id": core_ids,
        "patch_full_id": full_ids,
        "algorithm": [o.algorithm for o in onsets],
        "feedback": [o.feedback for o in onsets],
        "ams": [o.ams for o in onsets],
        "fms": [o.fms for o in onsets],
        "pan": [o.pan for o in onsets],
        "key_gate_event_index": [o.key_gate_event_index for o in onsets],
    }

    return {
        "schema_version": SCHEMA_VERSION,
        "extractor": {"name": EXTRACTOR_NAME, "version": EXTRACTOR_VERSION},
        "label_policy": "Creator, composer, artist, arranger, and programmer metadata are never read by this extractor.",
        "claim_boundary": (
            "This is cached execution evidence from Genesis VGM/VGZ. Physical YM2612 channels are observations, "
            "not persistent musical-part identity. Raw YM2612 key-gate transitions preserve episode-boundary evidence, "
            "and every admitted full-key onset points to the exact gate event that created it so same-tick ordering survives. "
            "Gate writes are not notes or musical parts. Creator roles live in a separate index."
        ),
        "source": {
            "path": path.as_posix(),
            "file": path.name,
            "corpus_id": corpus_id,
            "size_bytes": path.stat().st_size,
        },
        "timing": {"duration_vgm_samples": last_tick},
        "ym2612": {
            "ordinary_full_key_ons": len(onsets),
            "patch_core_dictionary": core_dict,
            "patch_full_dictionary": full_dict,
            "events": event_columns,
            "key_gate_events": {
                "tick": key_gate_ticks,
                "channel": key_gate_channels,
                "operator_mask": key_gate_masks,
            },
            "channels": channels,
        },
        "psg": {"ticks": psg_ticks, "values": psg_values},
        "game_gear_stereo": {"ticks": stereo_ticks, "values": stereo_values},
        "dac_stream": dac_stream,
        "realization_counters": dict(sorted(counters.items())),
    }


def _safe_component(value: str) -> str:
    text = "".join(ch.lower() if ch.isalnum() else "-" for ch in value)
    while "--" in text:
        text = text.replace("--", "-")
    return text.strip("-") or "track"


def cache_path_for(source: pathlib.Path, corpus_id: str, cache_root: pathlib.Path) -> pathlib.Path:
    return cache_root / _safe_component(corpus_id) / f"{_safe_component(source.stem)}.json"


def _cache_current(cache_path: pathlib.Path, source: pathlib.Path) -> bool:
    if not cache_path.is_file():
        return False
    try:
        data = json.loads(cache_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False
    return (
        data.get("schema_version") == SCHEMA_VERSION
        and data.get("extractor") == {"name": EXTRACTOR_NAME, "version": EXTRACTOR_VERSION}
        and isinstance(data.get("source"), dict)
        and data["source"].get("size_bytes") == source.stat().st_size
    )


def build_one(
    source: pathlib.Path,
    *,
    corpus_id: str,
    cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
    refresh: bool = False,
) -> tuple[pathlib.Path, bool, dict[str, object]]:
    destination = cache_path_for(source, corpus_id, cache_root)
    if not refresh and _cache_current(destination, source):
        return destination, False, json.loads(destination.read_text(encoding="utf-8"))
    capsule = extract_capsule(source, corpus_id=corpus_id)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(capsule, separators=(",", ":"), sort_keys=True) + "\n", encoding="utf-8")
    return destination, True, capsule


def _read_jsonl(path: pathlib.Path) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        record = json.loads(line)
        if not isinstance(record, dict):
            raise ValueError(f"{path}:{line_number}: expected JSON object")
        records.append(record)
    return records


def selected_credits(
    records: Iterable[dict[str, object]],
    *,
    creator: str,
    role: str | None,
    admitted_statuses: set[str],
) -> list[dict[str, object]]:
    selected = []
    for record in records:
        if record.get("creator") != creator:
            continue
        if role is not None and record.get("role") != role:
            continue
        if record.get("status") not in admitted_statuses:
            continue
        selected.append(record)
    selected.sort(key=lambda item: str(item.get("fixture_path", "")))
    return selected


def _infer_corpus_id(fixture_path: pathlib.Path) -> str:
    parts = fixture_path.as_posix().split("/")
    try:
        index = parts.index("corpus")
        return parts[index + 1]
    except (ValueError, IndexError):
        return fixture_path.parent.name


def _index_record(cache_path: pathlib.Path, capsule: dict[str, object]) -> dict[str, object]:
    source = capsule["source"]
    ym2612 = capsule["ym2612"]
    assert isinstance(source, dict) and isinstance(ym2612, dict)
    return {
        "cache_path": cache_path.as_posix(),
        "source_path": source["path"],
        "corpus_id": source["corpus_id"],
        "source_size_bytes": source["size_bytes"],
        "ordinary_full_key_ons": ym2612["ordinary_full_key_ons"],
        "extractor_version": EXTRACTOR_VERSION,
    }


def write_cache_index(records: Iterable[dict[str, object]], path: pathlib.Path = DEFAULT_CACHE_INDEX) -> None:
    unique = {str(record["source_path"]): record for record in records}
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "".join(json.dumps(unique[key], sort_keys=True) + "\n" for key in sorted(unique)),
        encoding="utf-8",
    )


def build_creator(
    *,
    credit_index: pathlib.Path,
    creator: str,
    role: str | None,
    repo_root: pathlib.Path,
    cache_root: pathlib.Path,
    cache_index: pathlib.Path,
    refresh: bool,
    admitted_statuses: set[str],
) -> dict[str, object]:
    credits = selected_credits(
        _read_jsonl(credit_index), creator=creator, role=role,
        admitted_statuses=admitted_statuses,
    )
    built = reused = 0
    index_records: list[dict[str, object]] = []
    for credit in credits:
        fixture = credit.get("fixture_path")
        if not isinstance(fixture, str):
            raise ValueError("credit record missing fixture_path")
        relative = pathlib.Path(fixture)
        source = repo_root / relative
        if not source.is_file():
            raise FileNotFoundError(source)
        corpus_id = str(credit.get("corpus_id") or _infer_corpus_id(relative))
        destination, changed, capsule = build_one(
            source, corpus_id=corpus_id, cache_root=cache_root, refresh=refresh,
        )
        built += int(changed)
        reused += int(not changed)
        index_records.append(_index_record(destination, capsule))

    existing = _read_jsonl(cache_index) if cache_index.is_file() else []
    write_cache_index([*existing, *index_records], cache_index)
    return {
        "creator": creator,
        "role": role,
        "selected_tracks": len(credits),
        "built": built,
        "reused": reused,
        "cache_root": cache_root.as_posix(),
        "cache_index": cache_index.as_posix(),
    }


def _paths_in_corpus(corpus: pathlib.Path) -> list[pathlib.Path]:
    if corpus.is_file():
        return [corpus]
    return sorted(
        path for path in corpus.iterdir()
        if path.is_file() and path.suffix.lower() in {".vgm", ".vgz"}
    )


def build_corpus(
    corpus: pathlib.Path,
    *,
    corpus_id: str,
    cache_root: pathlib.Path,
    cache_index: pathlib.Path,
    refresh: bool,
) -> dict[str, object]:
    paths = _paths_in_corpus(corpus)
    if not paths:
        raise ValueError(f"no VGM/VGZ files found in {corpus}")
    built = reused = 0
    index_records = []
    for source in paths:
        destination, changed, capsule = build_one(
            source, corpus_id=corpus_id, cache_root=cache_root, refresh=refresh
        )
        built += int(changed)
        reused += int(not changed)
        index_records.append(_index_record(destination, capsule))
    existing = _read_jsonl(cache_index) if cache_index.is_file() else []
    write_cache_index([*existing, *index_records], cache_index)
    return {"corpus_id": corpus_id, "tracks": len(paths), "built": built, "reused": reused}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    corpus = sub.add_parser("build-corpus", help="cache every VGM/VGZ in one corpus")
    corpus.add_argument("corpus", type=pathlib.Path)
    corpus.add_argument("--corpus-id")
    corpus.add_argument("--cache-root", type=pathlib.Path, default=DEFAULT_CACHE_ROOT)
    corpus.add_argument("--cache-index", type=pathlib.Path, default=DEFAULT_CACHE_INDEX)
    corpus.add_argument("--refresh", action="store_true")

    creator = sub.add_parser("build-creator", help="cache tracks selected by the separate role-credit index")
    creator.add_argument("--credits", type=pathlib.Path, required=True)
    creator.add_argument("--creator", required=True)
    creator.add_argument("--role")
    creator.add_argument("--repo-root", type=pathlib.Path, default=pathlib.Path("."))
    creator.add_argument("--cache-root", type=pathlib.Path, default=DEFAULT_CACHE_ROOT)
    creator.add_argument("--cache-index", type=pathlib.Path, default=DEFAULT_CACHE_INDEX)
    creator.add_argument("--status", action="append", dest="statuses", default=[])
    creator.add_argument("--refresh", action="store_true")

    lookup = sub.add_parser("lookup", help="list role-index records without touching source music")
    lookup.add_argument("--credits", type=pathlib.Path, required=True)
    lookup.add_argument("--creator", required=True)
    lookup.add_argument("--role")
    lookup.add_argument("--status", action="append", dest="statuses", default=[])

    args = parser.parse_args()
    statuses = set(getattr(args, "statuses", []) or ["exact", "derived", "whole_soundtrack"])
    if args.command == "build-corpus":
        result = build_corpus(
            args.corpus,
            corpus_id=args.corpus_id or args.corpus.name,
            cache_root=args.cache_root,
            cache_index=args.cache_index,
            refresh=args.refresh,
        )
    elif args.command == "build-creator":
        result = build_creator(
            credit_index=args.credits,
            creator=args.creator,
            role=args.role,
            repo_root=args.repo_root,
            cache_root=args.cache_root,
            cache_index=args.cache_index,
            refresh=args.refresh,
            admitted_statuses=statuses,
        )
    else:
        result = selected_credits(
            _read_jsonl(args.credits),
            creator=args.creator,
            role=args.role,
            admitted_statuses=statuses,
        )
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
