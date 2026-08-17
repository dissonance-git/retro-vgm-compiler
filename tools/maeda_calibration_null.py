#!/usr/bin/env python3
"""Empirical label-permutation null for the frozen Maeda calibration.

This tool never extracts VGM features. It takes the same creator-blind frozen
audit as maeda_calibration_eval.py, applies documentary labels after extraction,
then repeatedly shuffles candidate labels within the mixed-composer control
worlds while preserving each world's positive count.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import random
import statistics
import sys
from typing import Callable


def _load_evaluator():
    path = pathlib.Path(__file__).with_name("maeda_calibration_eval.py")
    spec = importlib.util.spec_from_file_location("maeda_calibration_eval", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


evaluator = _load_evaluator()
ScoreFn = Callable[[dict[str, object], dict[str, object]], float]


def _permute_partition(
    pool: set[str],
    positive_count: int,
    rng: random.Random,
) -> tuple[set[str], set[str]]:
    if not 0 <= positive_count <= len(pool):
        raise ValueError("positive_count must fit the partition pool")
    ordered = sorted(pool)
    positives = set(rng.sample(ordered, positive_count))
    return positives, pool - positives


def _null_summary(observed: float, samples: list[float]) -> dict[str, float | int]:
    if not samples:
        raise ValueError("null distribution must contain at least one sample")
    greater_or_equal = sum(1 for value in samples if value >= observed)
    return {
        "observed": observed,
        "null_mean": statistics.mean(samples),
        "null_population_stdev": statistics.pstdev(samples),
        "empirical_p_greater_or_equal": (greater_or_equal + 1) / (len(samples) + 1),
        "permutations": len(samples),
    }


def _metric_snapshot(metric: dict[str, object]) -> dict[str, float]:
    return {
        "precision_lift_over_chance": float(metric["precision_lift_over_chance"]),
        "mean_reciprocal_rank": float(metric["mean_reciprocal_rank"]),
        "mean_best_positive_minus_best_negative": float(
            metric["mean_best_positive_minus_best_negative"]
        ),
    }


def _score_partition(
    tracks: dict[str, dict[str, object]],
    ga_pos: set[str],
    ga_neg: set[str],
    s3d_pos: set[str],
    s3d_neg: set[str],
    fixed_whole_positive: set[str],
    score_fn: ScoreFn,
    k: int,
) -> dict[str, object]:
    all_pos = ga_pos | s3d_pos | fixed_whole_positive
    all_neg = ga_neg | s3d_neg
    ga = evaluator._precision_at_k(tracks, ga_pos, ga_neg, score_fn, k)
    s3d = evaluator._precision_at_k(tracks, s3d_pos, s3d_neg, score_fn, k)
    cross = evaluator._precision_at_k(
        tracks,
        all_pos,
        all_neg,
        score_fn,
        k,
        cross_soundtrack_only=True,
    )
    return {
        "golden_axe_iii_within_soundtrack": ga,
        "sonic_3d_blast_within_soundtrack": s3d,
        "genesis_cross_soundtrack": cross,
        "genesis_cross_soundtrack_by_query_world": evaluator._summarize_query_worlds(cross),
    }


def permutation_null(
    audit: dict[str, object],
    policy: dict[str, object],
    *,
    k: int = 3,
    permutations: int = 2000,
    seed: int = 20260816,
) -> dict[str, object]:
    if permutations <= 0:
        raise ValueError("permutations must be positive")

    evaluator._validate_blind_audit(audit, policy)
    tracks = evaluator._index_tracks(audit)
    ga_pos, ga_neg, ga_quarantine = evaluator._golden_axe_partition(policy)
    s3d_pos, s3d_neg = evaluator._sonic_3d_partition(policy, tracks)
    fixed_whole = evaluator._whole_soundtrack_positives(policy, "mega-drive")

    ga_pool = ga_pos | ga_neg
    s3d_pool = s3d_pos | s3d_neg
    rng = random.Random(seed)

    views: dict[str, ScoreFn] = {
        "structural": evaluator.base.structural_similarity,
        "structural_pitch": evaluator.structural_pitch_similarity,
        "structural_rhythm": evaluator.structural_rhythm_similarity,
        "realization": evaluator.base.realization_similarity,
    }

    observed = {
        name: _score_partition(
            tracks, ga_pos, ga_neg, s3d_pos, s3d_neg, fixed_whole, score_fn, k
        )
        for name, score_fn in views.items()
    }

    scalar_fields = (
        "precision_lift_over_chance",
        "mean_reciprocal_rank",
        "mean_best_positive_minus_best_negative",
    )
    null_values: dict[str, dict[str, dict[str, list[float]]]] = {
        view: {
            test: {field: [] for field in scalar_fields}
            for test in (
                "golden_axe_iii_within_soundtrack",
                "sonic_3d_blast_within_soundtrack",
                "genesis_cross_soundtrack",
            )
        }
        for view in views
    }
    null_world_lift: dict[str, dict[str, list[float]]] = {view: {} for view in views}

    for _ in range(permutations):
        perm_ga_pos, perm_ga_neg = _permute_partition(ga_pool, len(ga_pos), rng)
        perm_s3d_pos, perm_s3d_neg = _permute_partition(s3d_pool, len(s3d_pos), rng)

        for view_name, score_fn in views.items():
            scored = _score_partition(
                tracks,
                perm_ga_pos,
                perm_ga_neg,
                perm_s3d_pos,
                perm_s3d_neg,
                fixed_whole,
                score_fn,
                k,
            )
            for test_name in (
                "golden_axe_iii_within_soundtrack",
                "sonic_3d_blast_within_soundtrack",
                "genesis_cross_soundtrack",
            ):
                snapshot = _metric_snapshot(scored[test_name])
                for field in scalar_fields:
                    null_values[view_name][test_name][field].append(snapshot[field])

            worlds = scored["genesis_cross_soundtrack_by_query_world"]
            assert isinstance(worlds, dict)
            for world_name, world_metric in worlds.items():
                assert isinstance(world_metric, dict)
                null_world_lift[view_name].setdefault(str(world_name), []).append(
                    float(world_metric["precision_lift_over_chance"])
                )

    results: dict[str, object] = {}
    for view_name in views:
        view_result: dict[str, object] = {}
        observed_view = observed[view_name]
        assert isinstance(observed_view, dict)
        for test_name in (
            "golden_axe_iii_within_soundtrack",
            "sonic_3d_blast_within_soundtrack",
            "genesis_cross_soundtrack",
        ):
            observed_metric = observed_view[test_name]
            assert isinstance(observed_metric, dict)
            snapshot = _metric_snapshot(observed_metric)
            view_result[test_name] = {
                field: _null_summary(
                    snapshot[field], null_values[view_name][test_name][field]
                )
                for field in scalar_fields
            }

        observed_worlds = observed_view["genesis_cross_soundtrack_by_query_world"]
        assert isinstance(observed_worlds, dict)
        world_results: dict[str, object] = {}
        for world_name, world_metric in observed_worlds.items():
            assert isinstance(world_metric, dict)
            samples = null_world_lift[view_name].get(str(world_name), [])
            world_results[str(world_name)] = {
                "precision_lift_over_chance": _null_summary(
                    float(world_metric["precision_lift_over_chance"]), samples
                )
            }
        view_result["genesis_cross_soundtrack_by_query_world"] = world_results
        results[view_name] = view_result

    return {
        "model": "Maeda documentary-label permutation null",
        "seed": seed,
        "permutations": permutations,
        "k": k,
        "null_hypothesis": (
            "Within Golden Axe III and Sonic 3D Blast, Maeda labels are exchangeable among "
            "scored tracks while each soundtrack's observed Maeda count is preserved. "
            "J.League Pro Striker 2 remains a fixed whole-soundtrack positive world."
        ),
        "claim_boundary": (
            "Small empirical p-values show that documentary labels align with the frozen "
            "feature geometry better than random count-matched labels. They do not establish "
            "Sonic 3 authorship or make diagnostic subviews independent evidence."
        ),
        "quarantined_controls": sorted(ga_quarantine),
        "views": results,
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
    parser.add_argument("--permutations", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=20260816)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    audit = json.loads(args.audit_json.read_text(encoding="utf-8"))
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    result = permutation_null(
        audit,
        policy,
        k=args.k,
        permutations=args.permutations,
        seed=args.seed,
    )
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
