#!/usr/bin/env python3
"""Unblind a frozen creator-blind VGM audit against Maeda control labels.

The input audit must already contain extracted track features. This evaluator
loads documentary labels only after that frozen audit exists, preventing
composer names from influencing extraction. It reports structural and
realization retrieval separately.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import sys
from typing import Callable


def _load_base():
    path = pathlib.Path(__file__).with_name("vgm_creator_feature_audit.py")
    spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


base = _load_base()
ScoreFn = Callable[[dict[str, object], dict[str, object]], float]


def track_id(soundtrack_id: str, fixture_path: str) -> str:
    return f"{soundtrack_id}::{pathlib.Path(fixture_path).name}"


def _index_tracks(audit: dict[str, object]) -> dict[str, dict[str, object]]:
    tracks = audit.get("tracks")
    if not isinstance(tracks, list):
        raise ValueError("audit must contain a tracks list")
    result: dict[str, dict[str, object]] = {}
    for track in tracks:
        if not isinstance(track, dict):
            raise ValueError("audit track entries must be objects")
        soundtrack_id = track.get("soundtrack_id")
        file_name = track.get("file")
        if not isinstance(soundtrack_id, str) or not isinstance(file_name, str):
            raise ValueError("audit tracks require soundtrack_id and file")
        key = f"{soundtrack_id}::{file_name}"
        if key in result:
            raise ValueError(f"duplicate audit track {key}")
        result[key] = track
    return result


def _precision_at_k(
    tracks: dict[str, dict[str, object]],
    positives: set[str],
    negatives: set[str],
    score_fn: ScoreFn,
    k: int,
    cross_soundtrack_only: bool = False,
) -> dict[str, object]:
    if k <= 0:
        raise ValueError("k must be positive")
    eligible = positives | negatives
    missing = sorted(eligible - set(tracks))
    if missing:
        raise ValueError(f"control tracks missing from frozen audit: {missing}")

    per_query: list[dict[str, object]] = []
    precisions: list[float] = []
    reciprocal_ranks: list[float] = []
    margins: list[float] = []

    for query_id in sorted(positives):
        query = tracks[query_id]
        candidates: list[tuple[float, str, bool]] = []
        query_soundtrack = str(query["soundtrack_id"])
        for candidate_id in sorted(eligible):
            if candidate_id == query_id:
                continue
            candidate = tracks[candidate_id]
            if cross_soundtrack_only and str(candidate["soundtrack_id"]) == query_soundtrack:
                continue
            candidates.append((score_fn(query, candidate), candidate_id, candidate_id in positives))
        candidates.sort(key=lambda row: (-row[0], row[1]))
        if not candidates:
            continue

        top = candidates[:k]
        precision = sum(1 for _, _, is_positive in top if is_positive) / len(top)
        precisions.append(precision)

        first_positive_rank = next(
            (index for index, (_, _, is_positive) in enumerate(candidates, start=1) if is_positive),
            None,
        )
        reciprocal_rank = 0.0 if first_positive_rank is None else 1.0 / first_positive_rank
        reciprocal_ranks.append(reciprocal_rank)

        positive_scores = [score for score, _, flag in candidates if flag]
        negative_scores = [score for score, _, flag in candidates if not flag]
        margin = (
            max(positive_scores) - max(negative_scores)
            if positive_scores and negative_scores
            else 0.0
        )
        margins.append(margin)

        per_query.append(
            {
                "query": query_id,
                "precision_at_k": precision,
                "first_positive_rank": first_positive_rank,
                "reciprocal_rank": reciprocal_rank,
                "best_positive_minus_best_negative": margin,
                "top": [
                    {"track_id": candidate_id, "score": score, "is_positive": is_positive}
                    for score, candidate_id, is_positive in top
                ],
            }
        )

    def mean(values: list[float]) -> float:
        return 0.0 if not values else sum(values) / len(values)

    return {
        "positive_queries": len(per_query),
        "positive_controls": len(positives),
        "negative_controls": len(negatives),
        "precision_at_k": mean(precisions),
        "mean_reciprocal_rank": mean(reciprocal_ranks),
        "mean_best_positive_minus_best_negative": mean(margins),
        "queries": per_query,
    }


def _golden_axe_partition(policy: dict[str, object]) -> tuple[set[str], set[str], set[str]]:
    world = policy["golden_axe_iii_track_resolved_world"]
    assert isinstance(world, dict)
    soundtrack_id = str(world["corpus_id"])
    positives: set[str] = set()
    negatives: set[str] = set()
    quarantined: set[str] = set()
    tracks = world["tracks"]
    assert isinstance(tracks, list)
    candidate = str(policy["candidate"])
    for entry in tracks:
        assert isinstance(entry, dict)
        key = track_id(soundtrack_id, str(entry["fixture_path"]))
        if entry.get("use") == "quarantined_conflict" or entry.get("credit_state") == "conflict":
            quarantined.add(key)
        elif entry.get("composer") == candidate:
            positives.add(key)
        else:
            negatives.add(key)
    return positives, negatives, quarantined


def _sonic_3d_partition(
    policy: dict[str, object],
    tracks: dict[str, dict[str, object]],
) -> tuple[set[str], set[str]]:
    world = policy["sonic_3d_blast_exact_track_world"]
    assert isinstance(world, dict)
    soundtrack_id = str(world["corpus_id"])
    positives = {
        track_id(soundtrack_id, str(path))
        for path in world["maeda_fixtures"]
    }
    if world.get("partition_complete_for_candidate") is not True:
        raise ValueError(
            "Sonic 3D Blast negatives require partition_complete_for_candidate=true in policy"
        )
    corpus_ids = {
        key for key, value in tracks.items() if str(value["soundtrack_id"]) == soundtrack_id
    }
    negatives = corpus_ids - positives
    return positives, negatives


def _whole_soundtrack_positives(policy: dict[str, object], platform_id: str) -> set[str]:
    result: set[str] = set()
    worlds = policy["whole_soundtrack_worlds"]
    assert isinstance(worlds, list)
    for world in worlds:
        assert isinstance(world, dict)
        if world.get("platform_id") != platform_id:
            continue
        soundtrack_id = str(world["corpus_id"])
        fixtures = world["fixtures"]
        assert isinstance(fixtures, list)
        result.update(track_id(soundtrack_id, str(path)) for path in fixtures)
    return result


def evaluate(
    audit: dict[str, object],
    policy: dict[str, object],
    k: int = 3,
) -> dict[str, object]:
    tracks = _index_tracks(audit)
    ga_pos, ga_neg, ga_quarantine = _golden_axe_partition(policy)
    s3d_pos, s3d_neg = _sonic_3d_partition(policy, tracks)
    genesis_whole = _whole_soundtrack_positives(policy, "mega-drive")

    all_genesis_pos = ga_pos | s3d_pos | genesis_whole
    all_genesis_neg = ga_neg | s3d_neg

    views = {
        "structural": base.structural_similarity,
        "realization": base.realization_similarity,
    }
    result_views: dict[str, object] = {}
    for name, score_fn in views.items():
        result_views[name] = {
            "golden_axe_iii_within_soundtrack": _precision_at_k(
                tracks, ga_pos, ga_neg, score_fn, k, cross_soundtrack_only=False
            ),
            "sonic_3d_blast_within_soundtrack": _precision_at_k(
                tracks, s3d_pos, s3d_neg, score_fn, k, cross_soundtrack_only=False
            ),
            "genesis_cross_soundtrack": _precision_at_k(
                tracks,
                all_genesis_pos,
                all_genesis_neg,
                score_fn,
                k,
                cross_soundtrack_only=True,
            ),
        }

    return {
        "model": "post-extraction Maeda control unblinding",
        "label_policy": (
            "The audit is loaded and indexed before documentary Maeda labels are applied. "
            "This evaluator never extracts VGM features."
        ),
        "claim_boundary": (
            "Retrieval quality calibrates whether the current feature views can rediscover "
            "known Maeda controls. It does not assign Sonic 3 authorship. Structural and "
            "realization evidence remain separate."
        ),
        "candidate": policy["candidate"],
        "quarantined_controls": sorted(ga_quarantine),
        "unsupported_cross_platform_worlds": [
            {
                "corpus_id": world["corpus_id"],
                "platform_id": world["platform_id"],
                "reason": (
                    "Current creator audit is Genesis/YM2612-centric; do not mix Game Gear "
                    "PSG-only controls into composition-level Maeda scoring yet."
                ),
            }
            for world in policy["whole_soundtrack_worlds"]
            if isinstance(world, dict) and world.get("platform_id") != "mega-drive"
        ],
        "views": result_views,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("audit_json", type=pathlib.Path)
    parser.add_argument(
        "--policy",
        type=pathlib.Path,
        default=pathlib.Path("research/projects/sonic3/maeda-calibration-policy.json"),
    )
    parser.add_argument("--k", type=int, default=3)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    # Intentional order: frozen feature audit first, labels second.
    audit = json.loads(args.audit_json.read_text(encoding="utf-8"))
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    result = evaluate(audit, policy, k=args.k)
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
