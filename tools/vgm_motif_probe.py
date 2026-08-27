#!/usr/bin/env python3
"""Label-blind Genesis VGM local-motif probe.

This is an exploratory corpus instrument below the persistent-part model. It
looks only at ordinary full YM2612 key-ons grouped by physical channel and asks
whether short joint interval/rhythm cells recur across tracks/soundtracks.

The probe is intentionally not called composer attribution, phrase analysis,
or persistent-part recovery. Its job is to pressure-test whether local musical
shape adds information beyond the older bag-of-interval baseline while the
stronger C++ execution -> part -> motif pipeline is being integrated with real
corpus execution.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import importlib.util
import json
import math
from pathlib import Path
import statistics
import sys
from typing import Iterable


def _load_base():
    path = Path(__file__).with_name("vgm_creator_feature_audit.py")
    spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


base = _load_base()


def _read_vgm_payload(path: Path) -> bytes:
    """Read VGM payload bytes without trusting the filename as compression truth."""
    source = path.read_bytes()
    if source.startswith(b"\x1f\x8b"):
        try:
            return gzip.decompress(source)
        except (OSError, EOFError) as exc:
            raise ValueError("gzip-marked VGM source could not be decompressed") from exc
    return source


def _collect_onsets(path: Path):
    raw = _read_vgm_payload(path)
    state = base.GenesisAuditState()
    onsets = []
    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

    for command in base.command_stream(raw):
        if command.opcode not in (0x52, 0x53):
            continue
        register, value = command.args
        port = 0 if command.opcode == 0x52 else 1
        state.update(port, register, value)
        if port != 0 or register != 0x28:
            continue
        key_mask = value & 0xF0
        encoded_channel = value & 0x07
        if key_mask != 0xF0 or encoded_channel not in channel_map:
            continue
        onset = state.onset(command.tick, channel_map[encoded_channel])
        if onset is not None:
            onsets.append(onset)
    return onsets


def _quantize(value: float, step: float) -> float:
    if step <= 0.0:
        raise ValueError("quantization step must be positive")
    return round(value / step) * step


def _window_signature(events, interval_step_octaves: float = 1.0 / 24.0) -> str | None:
    if len(events) < 3:
        return None

    intervals: list[float] = []
    gaps: list[int] = []
    for previous, current in zip(events, events[1:]):
        if previous.frequency_measure <= 0.0 or current.frequency_measure <= 0.0:
            return None
        if current.tick <= previous.tick:
            return None
        intervals.append(
            _quantize(
                math.log2(current.frequency_measure / previous.frequency_measure),
                interval_step_octaves,
            )
        )
        gaps.append(current.tick - previous.tick)

    median_gap = statistics.median(gaps)
    if median_gap <= 0:
        return None
    rhythm = [
        _quantize(min(4.0, gap / median_gap), 0.25)
        for gap in gaps
    ]
    contour = [1 if value > 1e-9 else -1 if value < -1e-9 else 0 for value in intervals]

    interval_text = ",".join(f"{value:+.5f}" for value in intervals)
    rhythm_text = ",".join(f"{value:.2f}" for value in rhythm)
    contour_text = ",".join(str(value) for value in contour)
    return f"i:{interval_text}|r:{rhythm_text}|c:{contour_text}"


def motif_probe_histogram(
    onsets: Iterable,
    window_events: int = 4,
) -> dict[str, int]:
    if window_events < 3:
        raise ValueError("motif probe requires windows of at least three events")

    by_channel: dict[int, list] = collections.defaultdict(list)
    for onset in onsets:
        by_channel[int(onset.channel)].append(onset)

    counts: collections.Counter[str] = collections.Counter()
    for events in by_channel.values():
        events.sort(key=lambda item: item.tick)
        for start in range(0, len(events) - window_events + 1):
            signature = _window_signature(events[start : start + window_events])
            if signature is not None:
                counts[signature] += 1
    return {key: counts[key] for key in sorted(counts)}


def _cosine(lhs: dict[str, int], rhs: dict[str, int]) -> float:
    keys = set(lhs) | set(rhs)
    if not keys:
        return 0.0
    dot = sum(lhs.get(key, 0) * rhs.get(key, 0) for key in keys)
    left = math.sqrt(sum(value * value for value in lhs.values()))
    right = math.sqrt(sum(value * value for value in rhs.values()))
    if left == 0.0 or right == 0.0:
        return 0.0
    return dot / (left * right)


def audit_file(path: Path, window_events: int = 4) -> dict[str, object]:
    onsets = _collect_onsets(path)
    histogram = motif_probe_histogram(onsets, window_events=window_events)
    total_windows = sum(histogram.values())
    repeated_windows = sum(max(0, count - 1) for count in histogram.values())
    return {
        "file": path.name,
        "ordinary_full_fm_key_ons": len(onsets),
        "window_events": window_events,
        "motif_probe_window_count": total_windows,
        "unique_motif_probe_signatures": len(histogram),
        "repeated_motif_probe_windows": repeated_windows,
        "motif_probe_histogram": histogram,
        "claim_boundary": (
            "Local motif probes use ordinary full YM2612 key-ons on physical channels. "
            "They are transposition/tempo-scale tolerant probes of joint interval/rhythm shape, "
            "not persistent-part identity, phrase truth, source notation, or composer evidence."
        ),
    }


def motif_probe_similarity(lhs: dict[str, object], rhs: dict[str, object]) -> float:
    first = lhs.get("motif_probe_histogram")
    second = rhs.get("motif_probe_histogram")
    if not isinstance(first, dict) or not isinstance(second, dict):
        raise ValueError("motif probe track is missing histogram data")
    return _cosine(first, second)


def _track_id(track: dict[str, object]) -> str:
    return f"{track['soundtrack_id']}::{track['file']}"


def _neighbors(
    tracks: list[dict[str, object]],
    limit: int,
    cross_soundtrack_only: bool,
) -> dict[str, list[dict[str, object]]]:
    result: dict[str, list[dict[str, object]]] = {}
    for index, track in enumerate(tracks):
        candidates: list[tuple[float, str, str]] = []
        for other_index, other in enumerate(tracks):
            if index == other_index:
                continue
            if cross_soundtrack_only and track["soundtrack_id"] == other["soundtrack_id"]:
                continue
            candidates.append((
                motif_probe_similarity(track, other),
                str(other["soundtrack_id"]),
                str(other["file"]),
            ))
        candidates.sort(key=lambda item: (-item[0], item[1], item[2]))
        result[_track_id(track)] = [
            {"soundtrack_id": soundtrack, "file": file_name, "score": score}
            for score, soundtrack, file_name in candidates[:limit]
        ]
    return result


def audit_soundtracks(
    corpora: list[Path],
    *,
    window_events: int = 4,
    neighbor_count: int = 5,
    cross_soundtrack_only: bool = True,
) -> dict[str, object]:
    if not corpora:
        raise ValueError("at least one corpus directory is required")

    seen: set[str] = set()
    tracks: list[dict[str, object]] = []
    for corpus in corpora:
        soundtrack_id = corpus.name
        if soundtrack_id in seen:
            raise ValueError(f"duplicate soundtrack identity {soundtrack_id!r}")
        seen.add(soundtrack_id)
        paths = sorted(
            path
            for path in corpus.iterdir()
            if path.is_file() and path.suffix.lower() in (".vgm", ".vgz")
        )
        if not paths:
            raise ValueError(f"no VGM/VGZ files found in {corpus}")
        for path in paths:
            track = audit_file(path, window_events=window_events)
            track["soundtrack_id"] = soundtrack_id
            track["track_id"] = f"{soundtrack_id}::{path.name}"
            tracks.append(track)

    return {
        "model": "blind cross-soundtrack Genesis VGM physical-channel motif probe",
        "label_policy": (
            "Only corpus-directory soundtrack identity is used. Composer/artist metadata and "
            "curated Sonic 3 attribution labels are not read."
        ),
        "claim_boundary": (
            "This is an exploratory physical-channel motif probe below persistent-part recovery. "
            "Its neighbors are candidates for deeper analysis, not composer attributions."
        ),
        "window_events": window_events,
        "cross_soundtrack_only": cross_soundtrack_only,
        "soundtracks": sorted(seen),
        "track_count": len(tracks),
        "tracks": tracks,
        "top_motif_probe_neighbors": _neighbors(
            tracks,
            max(0, neighbor_count),
            cross_soundtrack_only,
        ),
    }


def _synthetic_self_test() -> dict[str, float]:
    def onset(tick: int, channel: int, measure: float):
        # Only the fields used by motif_probe_histogram are needed here.
        class Synthetic:
            pass
        item = Synthetic()
        item.tick = tick
        item.channel = channel
        item.frequency_measure = measure
        return item

    a = [
        onset(0, 0, 100.0),
        onset(100, 0, 112.5),
        onset(200, 0, 125.0),
        onset(400, 0, 106.25),
    ]
    b = [
        onset(0, 4, 200.0),
        onset(200, 4, 225.0),
        onset(400, 4, 250.0),
        onset(800, 4, 212.5),
    ]
    c = [
        onset(0, 1, 100.0),
        onset(100, 1, 80.0),
        onset(350, 1, 120.0),
        onset(450, 1, 70.0),
    ]
    pa = {"motif_probe_histogram": motif_probe_histogram(a)}
    pb = {"motif_probe_histogram": motif_probe_histogram(b)}
    pc = {"motif_probe_histogram": motif_probe_histogram(c)}
    same = motif_probe_similarity(pa, pb)
    different = motif_probe_similarity(pa, pc)
    if abs(same - 1.0) > 1e-12 or not different < same:
        raise AssertionError("motif probe synthetic invariance regression failed")
    return {"transposed_tempo_scaled_similarity": same, "different_shape_similarity": different}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpora", type=Path, nargs="*")
    parser.add_argument("--json", type=Path)
    parser.add_argument("--window-events", type=int, default=4)
    parser.add_argument("--neighbors", type=int, default=5)
    parser.add_argument("--include-within-soundtrack", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        result: dict[str, object] = {"self_test": _synthetic_self_test()}
    else:
        if not args.corpora:
            parser.error("at least one corpus directory is required unless --self-test is used")
        result = audit_soundtracks(
            args.corpora,
            window_events=args.window_events,
            neighbor_count=args.neighbors,
            cross_soundtrack_only=not args.include_within_soundtrack,
        )

    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
