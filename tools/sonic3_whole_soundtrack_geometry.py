#!/usr/bin/env python3
"""Creator-blind whole-corpus geometry for the frozen Sonic 3/S&K VGM panel.

This is a bounded research runner around tools.vgm_creator_feature_audit. It does
not read composer metadata or attribution labels. It preserves the analyzer's
separate musical-trajectory and Genesis-realization views and emits all pairwise
scores for the committed corpus.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
from typing import Callable

from tools.vgm_creator_feature_audit import (
    audit_file,
    realization_similarity,
    structural_similarity,
)

EXPECTED_COUNT = 58
EXPECTED_SEAMS = {
    "03|04": {"structural": 0.901167471209338, "realization": 0.7594587371300954},
    "11|12": {"structural": 0.9503619095388162, "realization": 0.9892915807398976},
    "38|39": {"structural": 0.7261944646468224, "realization": 0.28066995215710777},
}
TOLERANCE = 1e-12


def track_number(name: str) -> int:
    return int(name.split(" - ", 1)[0])


def matrix(tracks: list[dict[str, object]], fn: Callable) -> list[list[float]]:
    return [[fn(left, right) for right in tracks] for left in tracks]


def top_neighbors(
    tracks: list[dict[str, object]], scores: list[list[float]], limit: int = 5
) -> dict[str, list[dict[str, object]]]:
    result: dict[str, list[dict[str, object]]] = {}
    for i, track in enumerate(tracks):
        ranked = sorted(
            (
                (scores[i][j], str(other["file"]))
                for j, other in enumerate(tracks)
                if i != j
            ),
            key=lambda item: (-item[0], item[1]),
        )
        result[str(track["file"])] = [
            {"file": name, "score": score} for score, name in ranked[:limit]
        ]
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpus", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path, required=True)
    args = parser.parse_args()

    paths = sorted(
        (
            path
            for path in args.corpus.iterdir()
            if path.is_file() and path.suffix.lower() in {".vgm", ".vgz"}
        ),
        key=lambda path: track_number(path.name),
    )
    if len(paths) != EXPECTED_COUNT:
        raise SystemExit(f"expected {EXPECTED_COUNT} tracks, found {len(paths)}")

    source_manifest = [
        {
            "index": track_number(path.name),
            "file": path.name,
            "sha1": hashlib.sha1(path.read_bytes()).hexdigest(),
            "bytes": path.stat().st_size,
        }
        for path in paths
    ]
    tracks = [audit_file(path) for path in paths]
    structural = matrix(tracks, structural_similarity)
    realization = matrix(tracks, realization_similarity)

    validation: dict[str, object] = {}
    for pair, expected in EXPECTED_SEAMS.items():
        a, b = (int(value) for value in pair.split("|"))
        observed = {
            "structural": structural[a - 1][b - 1],
            "realization": realization[a - 1][b - 1],
        }
        deltas = {key: abs(observed[key] - expected[key]) for key in expected}
        passed = all(delta <= TOLERANCE for delta in deltas.values())
        validation[pair] = {
            "expected": expected,
            "observed": observed,
            "absolute_delta": deltas,
            "passed": passed,
        }
        if not passed:
            raise SystemExit(f"frozen seam validation failed for {pair}: {validation[pair]}")

    result = {
        "schema": "sonic3-whole-soundtrack-creator-blind-geometry-001",
        "corpus_track_count": len(tracks),
        "unique_pair_count": len(tracks) * (len(tracks) - 1) // 2,
        "label_policy": "No composer/artist metadata or attribution labels are read before extraction or scoring.",
        "claim_boundary": (
            "Structural similarity is physical-channel full-key-on trajectory similarity, not notation-level "
            "or persistent-part identity. Realization similarity describes observed Genesis execution. "
            "Neither view alone establishes composer identity."
        ),
        "method": {
            "extractor": "tools/vgm_creator_feature_audit.py",
            "structural_components": [
                "relative_interval_cosine",
                "interval_bigram_cosine",
                "normalized_onset_gap_cosine",
                "contour_cosine",
            ],
            "realization_components": [
                "core_patch_use_cosine",
                "core_patch_set_jaccard",
                "algorithm_cosine",
                "feedback_cosine",
                "pan_cosine",
            ],
        },
        "source_manifest": source_manifest,
        "validation": validation,
        "tracks": tracks,
        "structural_matrix": structural,
        "realization_matrix": realization,
        "top_structural_neighbors": top_neighbors(tracks, structural),
        "top_realization_neighbors": top_neighbors(tracks, realization),
    }
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "track_count": len(tracks),
                "unique_pair_count": result["unique_pair_count"],
                "validation": validation,
                "output": str(args.json),
                "output_sha256": hashlib.sha256(args.json.read_bytes()).hexdigest(),
            },
            indent=2,
            sort_keys=True,
        )
    )


if __name__ == "__main__":
    main()
