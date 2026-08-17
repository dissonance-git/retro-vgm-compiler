#!/usr/bin/env python3
"""Role-specific leave-one-track-out controls for Sonic 3D Blast.

This tool consumes an already-frozen creator-blind cross-soundtrack audit and
only then loads documentary Sonic 3D Blast composer/arranger labels. Structural
views calibrate composition-facing identity; realization calibrates the
arrangement/programming lane. Those lanes are never collapsed.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import statistics
import sys
from typing import Callable


def _load_maeda_eval():
    path = pathlib.Path(__file__).with_name("maeda_calibration_eval.py")
    spec = importlib.util.spec_from_file_location("maeda_calibration_eval", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


maeda = _load_maeda_eval()
ScoreFn = Callable[[dict[str, object], dict[str, object]], float]


def _role_map(policy: dict[str, object]) -> tuple[str, dict[str, dict[str, str]], dict[str, object]]:
    world = policy["sonic_3d_blast_exact_track_world"]
    if not isinstance(world, dict):
        raise ValueError("Sonic 3D Blast policy world must be an object")
    soundtrack_id = str(world["corpus_id"])
    credits = world.get("track_credits")
    if not isinstance(credits, list):
        raise ValueError("Sonic 3D Blast role-specificity requires track_credits")

    mapped: dict[str, dict[str, str]] = {}
    for credit in credits:
        if not isinstance(credit, dict):
            raise ValueError("track_credits entries must be objects")
        key = maeda.track_id(soundtrack_id, str(credit["fixture_path"]))
        if key in mapped:
            raise ValueError(f"duplicate Sonic 3D Blast role mapping {key}")
        mapped[key] = {
            "composer": str(credit["composer"]),
            "arranger_programmer": str(credit["arranger_programmer"]),
            "mapping_state": str(credit["mapping_state"]),
        }

    expected_count = world.get("corpus_fixture_count")
    if isinstance(expected_count, int) and len(mapped) != expected_count:
        raise ValueError(
            f"role map incomplete: expected {expected_count} fixtures, found {len(mapped)}"
        )
    specificity = world.get("role_specificity_policy")
    if not isinstance(specificity, dict):
        raise ValueError("role_specificity_policy is required")
    return soundtrack_id, mapped, specificity


def _mean(values: list[float]) -> float:
    return 0.0 if not values else statistics.mean(values)


def _evaluate_role(
    tracks: dict[str, dict[str, object]],
    labels: dict[str, str],
    learnable_classes: set[str],
    score_fn: ScoreFn,
    *,
    sentinel_classes: set[str] | None = None,
) -> dict[str, object]:
    sentinel_classes = sentinel_classes or set()
    missing = sorted(set(labels) - set(tracks))
    if missing:
        raise ValueError(f"role-mapped fixtures missing from frozen audit: {missing}")

    query_ids = sorted(key for key, label in labels.items() if label in learnable_classes)
    if not query_ids:
        raise ValueError("role evaluation has no learnable queries")

    per_query: list[dict[str, object]] = []
    confusion: dict[str, dict[str, int]] = {
        truth: {} for truth in sorted(learnable_classes)
    }
    class_hits: dict[str, list[int]] = {name: [] for name in learnable_classes}
    reciprocal_ranks: list[float] = []
    margins: list[float] = []
    sentinel_intrusions = 0

    for query_id in query_ids:
        truth = labels[query_id]
        query = tracks[query_id]
        ranked = sorted(
            (
                (score_fn(query, tracks[candidate_id]), candidate_id, labels[candidate_id])
                for candidate_id in labels
                if candidate_id != query_id
            ),
            key=lambda row: (-row[0], row[1]),
        )
        if not ranked:
            raise ValueError("role evaluation requires at least one candidate")

        top_score, top_id, predicted = ranked[0]
        hit = int(predicted == truth)
        class_hits[truth].append(hit)
        confusion[truth][predicted] = confusion[truth].get(predicted, 0) + 1
        if predicted in sentinel_classes:
            sentinel_intrusions += 1

        same_role_rank = next(
            (
                rank
                for rank, (_, _, candidate_role) in enumerate(ranked, start=1)
                if candidate_role == truth
            ),
            None,
        )
        reciprocal_rank = 0.0 if same_role_rank is None else 1.0 / same_role_rank
        reciprocal_ranks.append(reciprocal_rank)

        same_scores = [score for score, _, role in ranked if role == truth]
        other_scores = [score for score, _, role in ranked if role != truth]
        margin = (
            max(same_scores) - max(other_scores)
            if same_scores and other_scores
            else 0.0
        )
        margins.append(margin)

        per_query.append(
            {
                "query": query_id,
                "truth": truth,
                "predicted_top1": predicted,
                "top1_track": top_id,
                "top1_score": top_score,
                "top1_correct": bool(hit),
                "first_same_role_rank": same_role_rank,
                "reciprocal_rank": reciprocal_rank,
                "best_same_minus_best_other": margin,
            }
        )

    per_class_recall = {
        name: _mean([float(hit) for hit in class_hits[name]])
        for name in sorted(learnable_classes)
    }
    return {
        "query_count": len(query_ids),
        "learnable_classes": sorted(learnable_classes),
        "sentinel_classes": sorted(sentinel_classes),
        "top1_accuracy": _mean([float(row["top1_correct"]) for row in per_query]),
        "balanced_top1_accuracy": _mean(list(per_class_recall.values())),
        "per_class_recall": per_class_recall,
        "mean_reciprocal_rank": _mean(reciprocal_ranks),
        "mean_best_same_minus_best_other": _mean(margins),
        "sentinel_top1_intrusions": sentinel_intrusions,
        "confusion": confusion,
        "queries": per_query,
    }


def evaluate(audit: dict[str, object], policy: dict[str, object]) -> dict[str, object]:
    maeda._validate_blind_audit(audit, policy)
    tracks = maeda._index_tracks(audit)
    soundtrack_id, mapped, specificity = _role_map(policy)

    s3d_tracks = {
        key: value
        for key, value in tracks.items()
        if str(value["soundtrack_id"]) == soundtrack_id
    }
    if set(s3d_tracks) != set(mapped):
        missing = sorted(set(mapped) - set(s3d_tracks))
        extra = sorted(set(s3d_tracks) - set(mapped))
        raise ValueError(
            f"Sonic 3D Blast audit/role map mismatch; missing={missing}, extra={extra}"
        )

    composer_policy = specificity["composer"]
    arranger_policy = specificity["arranger_programmer"]
    if not isinstance(composer_policy, dict) or not isinstance(arranger_policy, dict):
        raise ValueError("role specificity subpolicies must be objects")

    composer_labels = {key: value["composer"] for key, value in mapped.items()}
    arranger_labels = {key: value["arranger_programmer"] for key, value in mapped.items()}
    composer_classes = set(composer_policy["learnable_classes"])
    composer_sentinels = set(composer_policy.get("singleton_sentinels", {}))
    arranger_classes = set(arranger_policy["learnable_classes"])

    composition = {
        "structural": _evaluate_role(
            s3d_tracks,
            composer_labels,
            composer_classes,
            maeda.base.structural_similarity,
            sentinel_classes=composer_sentinels,
        ),
        "structural_pitch": _evaluate_role(
            s3d_tracks,
            composer_labels,
            composer_classes,
            maeda.structural_pitch_similarity,
            sentinel_classes=composer_sentinels,
        ),
        "structural_rhythm": _evaluate_role(
            s3d_tracks,
            composer_labels,
            composer_classes,
            maeda.structural_rhythm_similarity,
            sentinel_classes=composer_sentinels,
        ),
    }
    arrangement = {
        "realization": _evaluate_role(
            s3d_tracks,
            arranger_labels,
            arranger_classes,
            maeda.base.realization_similarity,
        )
    }

    return {
        "model": "post-extraction Sonic 3D Blast role-specificity calibration",
        "soundtrack_id": soundtrack_id,
        "label_policy": (
            "The frozen creator-blind audit is validated before documentary role labels are "
            "loaded. This tool never extracts VGM features."
        ),
        "claim_boundary": (
            "Composition-facing structural retrieval and implementation-facing realization "
            "retrieval are separate calibration lanes. Success in arrangement/programming "
            "specificity cannot support a Sonic 3 composition credit."
        ),
        "composition": composition,
        "arrangement_programming": arrangement,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("audit_json", type=pathlib.Path)
    parser.add_argument(
        "--policy",
        type=pathlib.Path,
        default=pathlib.Path("research/projects/sonic3/maeda-calibration-policy.json"),
    )
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    audit = json.loads(args.audit_json.read_text(encoding="utf-8"))
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    result = evaluate(audit, policy)
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
