#!/usr/bin/env python3
"""Creator-blind Genesis VGM part matcher for attribution calibration.

Generation 2 keeps ordinary YM2612 key-on evidence separated by physical
channel, then removes channel-number correspondence by maximizing a one-to-one
match between active parts.  It deliberately uses only the two pitch-shape
features frozen in the Gen-2 preregistration:

- relative-semitone interval histogram
- interval-bigram histogram

No composer/artist metadata is read.  Similarity is evidence for calibration,
not authorship.
"""

from __future__ import annotations

import argparse
import gzip
import itertools
import json
import pathlib
from typing import Iterable

import vgm_creator_feature_audit as gen1


MODEL = "blind Genesis VGM permutation-invariant part matcher"
LABEL_POLICY = "No composer/artist metadata. Physical channel number is not a cross-track correspondence."


def _raw_vgm(path: pathlib.Path) -> bytes:
    packed = path.read_bytes()
    return gzip.decompress(packed) if path.suffix.lower() == ".vgz" else packed


def extract_part_features(path: pathlib.Path) -> dict[str, object]:
    """Extract the frozen Gen-2 per-channel pitch-shape representation."""
    state = gen1.GenesisAuditState()
    by_channel: dict[int, list[gen1.FmOnset]] = {index: [] for index in range(6)}
    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

    for command in gen1.command_stream(_raw_vgm(path)):
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

        channel = channel_map[encoded_channel]
        onset = state.onset(command.tick, channel)
        if onset is not None:
            by_channel[channel].append(onset)

    parts: list[dict[str, object]] = []
    for channel in range(6):
        events = by_channel[channel]
        if len(events) < 2:
            continue

        intervals: list[str] = []
        for previous, current in zip(events, events[1:]):
            interval = gen1._quantized_interval(previous, current)
            if interval is not None:
                intervals.append(interval)

        if not intervals:
            continue
        bigrams = [f"{left},{right}" for left, right in zip(intervals, intervals[1:])]
        parts.append(
            {
                "channel": channel,
                "key_ons": len(events),
                "interval_histogram_semitones": gen1._histogram(intervals),
                "interval_bigram_histogram": gen1._histogram(bigrams),
            }
        )

    return {
        "file": path.name,
        "path": str(path),
        "parts": parts,
    }


def part_similarity(lhs: dict[str, object], rhs: dict[str, object]) -> float:
    interval = gen1._cosine(
        lhs["interval_histogram_semitones"],  # type: ignore[arg-type]
        rhs["interval_histogram_semitones"],  # type: ignore[arg-type]
    )
    bigram = gen1._cosine(
        lhs["interval_bigram_histogram"],  # type: ignore[arg-type]
        rhs["interval_bigram_histogram"],  # type: ignore[arg-type]
    )
    # Frozen Gen-2 definition: equal untuned weighting. A missing bigram view
    # (for a two-onset part) contributes zero rather than changing the weight.
    return 0.5 * (0.0 if interval is None else interval) + 0.5 * (
        0.0 if bigram is None else bigram
    )


def _best_injective_assignment(
    smaller: list[dict[str, object]],
    larger: list[dict[str, object]],
) -> tuple[float, tuple[int, ...]]:
    """Return maximum summed similarity and larger-side indices by row.

    YM2612 exposes only six FM channels, so exhaustive injective assignment is
    bounded by 6! = 720 cases and avoids a numerical-optimization dependency.
    """
    if not smaller:
        return 0.0, ()

    scores = [
        [part_similarity(left, right) for right in larger]
        for left in smaller
    ]
    best_score = float("-inf")
    best_columns: tuple[int, ...] = ()
    for columns in itertools.permutations(range(len(larger)), len(smaller)):
        score = sum(scores[row][column] for row, column in enumerate(columns))
        if score > best_score:
            best_score = score
            best_columns = columns
    return best_score, best_columns


def track_similarity(
    lhs: dict[str, object],
    rhs: dict[str, object],
    *,
    include_assignment: bool = False,
) -> float | dict[str, object]:
    """Permutation-invariant track similarity from matched active parts.

    Unmatched active parts are ignored, exactly as preregistered.  The returned
    score is therefore the arithmetic mean across the smaller active-part set.
    """
    left_parts = list(lhs.get("parts", []))  # type: ignore[arg-type]
    right_parts = list(rhs.get("parts", []))  # type: ignore[arg-type]
    if not left_parts or not right_parts:
        result = {"similarity": 0.0, "matched_parts": 0, "assignment": []}
        return result if include_assignment else 0.0

    swapped = len(left_parts) > len(right_parts)
    smaller, larger = (right_parts, left_parts) if swapped else (left_parts, right_parts)
    total, columns = _best_injective_assignment(smaller, larger)
    score = total / len(smaller)

    if not include_assignment:
        return score

    pairs: list[dict[str, object]] = []
    for row, column in enumerate(columns):
        small = smaller[row]
        large = larger[column]
        left, right = (large, small) if swapped else (small, large)
        pairs.append(
            {
                "left_channel": left["channel"],
                "right_channel": right["channel"],
                "part_similarity": part_similarity(left, right),
            }
        )
    return {
        "similarity": score,
        "matched_parts": len(smaller),
        "assignment": pairs,
    }


def _collect_paths(inputs: Iterable[str]) -> list[pathlib.Path]:
    paths: list[pathlib.Path] = []
    for raw in inputs:
        path = pathlib.Path(raw)
        if path.is_dir():
            paths.extend(
                candidate
                for candidate in path.rglob("*")
                if candidate.is_file() and candidate.suffix.lower() in {".vgm", ".vgz"}
            )
        elif path.is_file() and path.suffix.lower() in {".vgm", ".vgz"}:
            paths.append(path)
        else:
            raise SystemExit(f"not a VGM/VGZ file or directory: {path}")
    return sorted(dict.fromkeys(paths), key=lambda item: str(item).lower())


def build_audit(paths: list[pathlib.Path]) -> dict[str, object]:
    tracks = [extract_part_features(path) for path in paths]
    matrix = [
        [1.0 if i == j else float(track_similarity(left, right)) for j, right in enumerate(tracks)]
        for i, left in enumerate(tracks)
    ]
    return {
        "schema_version": 1,
        "model": MODEL,
        "label_policy": LABEL_POLICY,
        "similarity_definition": {
            "part_features": [
                "relative_semitone_interval_histogram",
                "interval_bigram_histogram",
            ],
            "part_similarity": "0.5 * interval_cosine + 0.5 * bigram_cosine",
            "assignment": "maximum-weight one-to-one assignment",
            "track_similarity": "mean matched part similarity",
            "unmatched_parts": "ignored",
        },
        "tracks": tracks,
        "similarity_matrix": matrix,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="VGM/VGZ files or directories")
    parser.add_argument("--json", dest="json_path", type=pathlib.Path)
    args = parser.parse_args()

    paths = _collect_paths(args.inputs)
    if not paths:
        raise SystemExit("no VGM/VGZ inputs found")
    payload = build_audit(paths)
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.json_path is not None:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
