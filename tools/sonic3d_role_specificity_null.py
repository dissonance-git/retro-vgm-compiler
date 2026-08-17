#!/usr/bin/env python3
"""Count-matched permutation null for Sonic 3D Blast role specificity.

The feature audit is frozen and creator-blind before this tool runs. Documentary
composer and arranger/programmer labels are then compared with permutations that
preserve each role's exact label multiset. Work-family membership never moves,
and same-family candidates remain excluded in every observed and null retrieval.
The primary family-block null also preserves the observed within-family label
package structure among families of the same size. Optional mapping-state
exclusions create matched sensitivity nulls only after the complete audit and
role/family maps have been validated.
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


def _load_role_eval():
    path = pathlib.Path(__file__).with_name("sonic3d_role_specificity_eval.py")
    spec = importlib.util.spec_from_file_location("sonic3d_role_specificity_eval", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


role_eval = _load_role_eval()
ScoreFn = Callable[[dict[str, object], dict[str, object]], float]


def _shuffle_labels(labels: dict[str, str], rng: random.Random) -> dict[str, str]:
    """Permute labels across fixed track ids while preserving exact class counts."""
    keys = sorted(labels)
    values = [labels[key] for key in keys]
    rng.shuffle(values)
    return dict(zip(keys, values))


def _shuffle_labels_by_family(
    labels: dict[str, str],
    families: dict[str, str],
    rng: random.Random,
) -> dict[str, str]:
    """Permute same-size family label packages while preserving family correlation.

    A source family's complete label multiset is moved to another family with the
    same number of selected tracks. Labels are shuffled within the target family
    so arbitrary lexical track ordering does not become part of the null.
    """
    if set(labels) != set(families):
        raise ValueError("family-block permutation requires one family id per labelled track")

    family_tracks: dict[str, list[str]] = {}
    for track_id, family_id in families.items():
        family_tracks.setdefault(family_id, []).append(track_id)
    for track_ids in family_tracks.values():
        track_ids.sort()

    by_size: dict[int, list[str]] = {}
    for family_id, track_ids in family_tracks.items():
        by_size.setdefault(len(track_ids), []).append(family_id)

    result: dict[str, str] = {}
    for size in sorted(by_size):
        target_families = sorted(by_size[size])
        source_packages = [
            [labels[track_id] for track_id in family_tracks[family_id]]
            for family_id in target_families
        ]
        rng.shuffle(source_packages)

        for target_family, package in zip(target_families, source_packages):
            shuffled_package = list(package)
            rng.shuffle(shuffled_package)
            target_tracks = family_tracks[target_family]
            for track_id, label in zip(target_tracks, shuffled_package):
                result[track_id] = label

    if set(result) != set(labels):
        raise AssertionError("family-block permutation lost labelled tracks")
    if sorted(result.values()) != sorted(labels.values()):
        raise AssertionError("family-block permutation changed global label counts")
    return result


def _permute_labels(
    labels: dict[str, str],
    families: dict[str, str],
    rng: random.Random,
    permutation_unit: str,
) -> dict[str, str]:
    if permutation_unit == "track":
        return _shuffle_labels(labels, rng)
    if permutation_unit == "family-block":
        return _shuffle_labels_by_family(labels, families, rng)
    raise ValueError(f"unsupported permutation_unit: {permutation_unit}")


def _snapshot(metric: dict[str, object]) -> dict[str, float]:
    recalls = metric.get("per_class_recall")
    if not isinstance(recalls, dict) or not recalls:
        raise ValueError("role metric requires per_class_recall")
    return {
        "balanced_top1_accuracy": float(metric["balanced_top1_accuracy"]),
        "minimum_learnable_class_recall": min(float(value) for value in recalls.values()),
        "mean_reciprocal_rank": float(metric["mean_reciprocal_rank"]),
        "mean_best_same_minus_best_other": float(metric["mean_best_same_minus_best_other"]),
    }


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


def _evaluate_view(
    tracks: dict[str, dict[str, object]],
    labels: dict[str, str],
    classes: set[str],
    score_fn: ScoreFn,
    families: dict[str, str],
    sentinels: set[str] | None = None,
) -> dict[str, object]:
    return role_eval._evaluate_role(
        tracks,
        labels,
        classes,
        score_fn,
        families=families,
        sentinel_classes=sentinels,
    )


def permutation_null(
    audit: dict[str, object],
    policy: dict[str, object],
    family_policy: dict[str, object],
    *,
    permutations: int = 5000,
    seed: int = 20260816,
    excluded_mapping_states: set[str] | None = None,
    permutation_unit: str = "family-block",
) -> dict[str, object]:
    if permutations <= 0:
        raise ValueError("permutations must be positive")
    if permutation_unit not in {"track", "family-block"}:
        raise ValueError("permutation_unit must be 'track' or 'family-block'")

    role_eval.maeda._validate_blind_audit(audit, policy)
    indexed = role_eval.maeda._index_tracks(audit)
    soundtrack_id, mapped, specificity = role_eval._role_map(policy)
    families = role_eval._family_map(family_policy, soundtrack_id, set(mapped))

    all_tracks = {
        key: value
        for key, value in indexed.items()
        if str(value["soundtrack_id"]) == soundtrack_id
    }
    if set(all_tracks) != set(mapped):
        missing = sorted(set(mapped) - set(all_tracks))
        extra = sorted(set(all_tracks) - set(mapped))
        raise ValueError(
            f"Sonic 3D Blast audit/role map mismatch; missing={missing}, extra={extra}"
        )

    mapped, families, tracks, excluded_tracks = role_eval._filter_mapping_states(
        mapped,
        families,
        all_tracks,
        excluded_mapping_states,
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
    role_eval._require_repeated_classes(composer_labels, composer_classes, "composition")
    role_eval._require_repeated_classes(
        arranger_labels, arranger_classes, "arrangement/programming"
    )

    views: dict[str, tuple[str, dict[str, str], set[str], ScoreFn, set[str]]] = {
        "structural": (
            "composition",
            composer_labels,
            composer_classes,
            role_eval.maeda.base.structural_similarity,
            composer_sentinels,
        ),
        "structural_pitch": (
            "composition",
            composer_labels,
            composer_classes,
            role_eval.maeda.structural_pitch_similarity,
            composer_sentinels,
        ),
        "structural_rhythm": (
            "composition",
            composer_labels,
            composer_classes,
            role_eval.maeda.structural_rhythm_similarity,
            composer_sentinels,
        ),
        "realization": (
            "arrangement_programming",
            arranger_labels,
            arranger_classes,
            role_eval.maeda.base.realization_similarity,
            set(),
        ),
    }

    observed: dict[str, dict[str, object]] = {}
    observed_snapshots: dict[str, dict[str, float]] = {}
    for view_name, (lane, labels, classes, score_fn, sentinels) in views.items():
        metric = _evaluate_view(
            tracks,
            labels,
            classes,
            score_fn,
            families,
            sentinels,
        )
        observed[view_name] = {"lane": lane, "metric": metric}
        observed_snapshots[view_name] = _snapshot(metric)

    fields = (
        "balanced_top1_accuracy",
        "minimum_learnable_class_recall",
        "mean_reciprocal_rank",
        "mean_best_same_minus_best_other",
    )
    null_values: dict[str, dict[str, list[float]]] = {
        view_name: {field: [] for field in fields}
        for view_name in views
    }

    rng = random.Random(seed)
    for _ in range(permutations):
        permuted_composer = _permute_labels(
            composer_labels, families, rng, permutation_unit
        )
        permuted_arranger = _permute_labels(
            arranger_labels, families, rng, permutation_unit
        )

        for view_name, (lane, _labels, classes, score_fn, sentinels) in views.items():
            permuted_labels = (
                permuted_composer if lane == "composition" else permuted_arranger
            )
            metric = _evaluate_view(
                tracks,
                permuted_labels,
                classes,
                score_fn,
                families,
                sentinels,
            )
            snapshot = _snapshot(metric)
            for field in fields:
                null_values[view_name][field].append(snapshot[field])

    results: dict[str, object] = {}
    for view_name, (lane, labels, classes, _score_fn, sentinels) in views.items():
        metric = observed[view_name]["metric"]
        assert isinstance(metric, dict)
        results[view_name] = {
            "lane": lane,
            "observed_label_counts": dict(
                sorted(
                    {
                        label: sum(1 for value in labels.values() if value == label)
                        for label in set(labels.values())
                    }.items()
                )
            ),
            "learnable_classes": sorted(classes),
            "sentinel_classes": sorted(sentinels),
            "same_family_candidates_excluded": metric[
                "same_family_candidates_excluded"
            ],
            "observed_per_class_recall": metric["per_class_recall"],
            "observed_singleton_top1_intrusions": metric[
                "sentinel_top1_intrusions"
            ],
            "statistics": {
                field: _null_summary(
                    observed_snapshots[view_name][field],
                    null_values[view_name][field],
                )
                for field in fields
            },
        }

    family_structure_policy = (
        "Whole label packages are permuted only among selected families with the same number "
        "of tracks; labels may reorder within the target family. This preserves the multiset "
        "of within-family label compositions while breaking creator-to-feature alignment."
        if permutation_unit == "family-block"
        else "Labels are permuted independently across selected tracks; family ids remain fixed."
    )

    return {
        "model": "Sonic 3D Blast role-specificity label-permutation null",
        "soundtrack_id": soundtrack_id,
        "seed": seed,
        "permutations": permutations,
        "permutation_unit": permutation_unit,
        "excluded_mapping_states": sorted(excluded_mapping_states or set()),
        "excluded_tracks": excluded_tracks,
        "included_track_count": len(mapped),
        "null_hypothesis": (
            "Creator-role labels are exchangeable under the selected permutation unit across "
            "the fixed Sonic 3D Blast feature geometry. Exact selected-panel role counts are "
            "preserved; work-family membership remains fixed and same-family candidates remain "
            "excluded in every observed and permuted retrieval."
        ),
        "family_structure_policy": family_structure_policy,
        "label_policy": (
            "The complete frozen creator-blind audit and complete documentary role/family maps "
            "are validated before mapping-state exclusions, documentary role labels, or "
            "permutations are applied. This tool never extracts VGM features."
        ),
        "claim_boundary": (
            "Empirical significance measures whether documentary creator-role labels align with "
            "the frozen, anti-sibling feature geometry better than the selected count-matched "
            "randomization. Composition and arrangement/programming lanes remain separate, and "
            "no result assigns Sonic 3 authorship."
        ),
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
    parser.add_argument(
        "--families",
        type=pathlib.Path,
        default=pathlib.Path("research/projects/sonic3/sonic3d-role-family-policy.json"),
    )
    parser.add_argument(
        "--exclude-mapping-state",
        action="append",
        default=[],
        help="Exclude a documentary mapping state after the complete blind panel is validated.",
    )
    parser.add_argument(
        "--permutation-unit",
        choices=("track", "family-block"),
        default="family-block",
        help="Primary default preserves same-size family label-package structure.",
    )
    parser.add_argument("--permutations", type=int, default=5000)
    parser.add_argument("--seed", type=int, default=20260816)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    audit = json.loads(args.audit_json.read_text(encoding="utf-8"))
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    family_policy = json.loads(args.families.read_text(encoding="utf-8"))
    result = permutation_null(
        audit,
        policy,
        family_policy,
        permutations=args.permutations,
        seed=args.seed,
        excluded_mapping_states=set(args.exclude_mapping_state),
        permutation_unit=args.permutation_unit,
    )
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
