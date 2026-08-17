#!/usr/bin/env python3
"""Reveal creator labels only after a creator-blind SPC similarity freeze exists.

This evaluator never extracts music. It consumes an already-frozen opaque cue
matrix, then joins fixture identity to independently maintained composer-control
admissions and calibration-policy holdouts. Team-level corpora remain unlabeled
validation worlds and cannot become training truth here.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
from collections import defaultdict
from dataclasses import dataclass
from typing import Any


EXPECTED_FREEZE_MODEL = "creator-blind SPC forensic corpus freeze"
SUPPORTED_GROUNDING_STATUSES = {"exact", "derived"}
DEFAULT_MINIMUM_MARGIN = 0.08
DEFAULT_FALSE_POSITIVE_THRESHOLD = 0.65


@dataclass(frozen=True)
class GroundedControl:
    cue_id: str
    fixture_path: str
    candidate: str
    soundtrack_id: str
    work_family_id: str
    confidence: float
    status: str


def corpus_id_from_fixture(fixture_path: str) -> str:
    parts = pathlib.PurePosixPath(fixture_path).parts
    if len(parts) < 4 or tuple(parts[:2]) != ("tests", "corpus"):
        raise ValueError(f"fixture is not below tests/corpus: {fixture_path}")
    return parts[2]


def file_sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_panel(path: pathlib.Path) -> tuple[dict[str, str], dict[str, str]]:
    value = json.loads(path.read_text(encoding="utf-8"))
    cues = value.get("cues") if isinstance(value, dict) else None
    if not isinstance(cues, list):
        raise ValueError("panel requires cues list")
    fixture_by_cue: dict[str, str] = {}
    cue_by_fixture: dict[str, str] = {}
    for entry in cues:
        if not isinstance(entry, dict):
            raise ValueError("panel cue must be an object")
        cue_id = entry.get("cue_id")
        fixture = entry.get("fixture_path")
        if not isinstance(cue_id, str) or not isinstance(fixture, str):
            raise ValueError("panel cue requires string cue_id and fixture_path")
        if cue_id in fixture_by_cue or fixture in cue_by_fixture:
            raise ValueError("panel cue ids and fixture paths must be unique")
        fixture_by_cue[cue_id] = fixture
        cue_by_fixture[fixture] = cue_id
    return fixture_by_cue, cue_by_fixture


def load_frozen(path: pathlib.Path, expected_cues: set[str]) -> tuple[dict[str, dict[str, float]], str]:
    raw = path.read_bytes()
    value = json.loads(raw.decode("utf-8"))
    if not isinstance(value, dict) or value.get("model") != EXPECTED_FREEZE_MODEL:
        raise ValueError("input is not a creator-blind SPC forensic corpus freeze")
    cues = value.get("cues")
    matrix = value.get("similarity_matrix")
    if not isinstance(cues, list) or not isinstance(matrix, dict):
        raise ValueError("frozen corpus requires cues and similarity_matrix")
    actual = {cue.get("cue_id") for cue in cues if isinstance(cue, dict)}
    if actual != expected_cues:
        missing = sorted(expected_cues - actual)
        extra = sorted(actual - expected_cues)
        raise ValueError(f"frozen cue set does not match panel; missing={missing} extra={extra}")
    if set(matrix) != expected_cues:
        raise ValueError("similarity matrix row set does not match panel")
    normalized: dict[str, dict[str, float]] = {}
    for cue_id in sorted(expected_cues):
        row = matrix.get(cue_id)
        if not isinstance(row, dict) or set(row) != expected_cues:
            raise ValueError(f"similarity matrix column set invalid for {cue_id}")
        normalized[cue_id] = {}
        for other, score in row.items():
            if not isinstance(score, (int, float)) or isinstance(score, bool):
                raise ValueError("similarity scores must be numeric")
            score = float(score)
            if not 0.0 <= score <= 1.0:
                raise ValueError("similarity scores must be in [0, 1]")
            normalized[cue_id][other] = score
            mirror = matrix.get(other, {}).get(cue_id) if isinstance(matrix.get(other), dict) else None
            if not isinstance(mirror, (int, float)) or abs(float(mirror) - score) > 1e-9:
                raise ValueError("similarity matrix must be symmetric")
    return normalized, hashlib.sha256(raw).hexdigest()


def load_grounded_controls(
    path: pathlib.Path,
    cue_by_fixture: dict[str, str],
) -> list[GroundedControl]:
    result: list[GroundedControl] = []
    seen_fixture: set[str] = set()
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw_line.strip():
            continue
        entry = json.loads(raw_line)
        if not isinstance(entry, dict):
            raise ValueError(f"admission line {line_number} must be an object")
        if entry.get("role") != "composer" or entry.get("status") not in SUPPORTED_GROUNDING_STATUSES:
            continue
        fixture = entry.get("fixture_path")
        candidate = entry.get("candidate")
        confidence = entry.get("confidence")
        work_family = entry.get("work_family_id")
        if fixture not in cue_by_fixture:
            continue
        if not isinstance(candidate, str) or not candidate:
            raise ValueError(f"admission line {line_number} missing candidate")
        if not isinstance(confidence, (int, float)) or isinstance(confidence, bool):
            raise ValueError(f"admission line {line_number} has invalid confidence")
        confidence = float(confidence)
        if not 0.0 <= confidence <= 1.0:
            raise ValueError(f"admission line {line_number} confidence outside [0,1]")
        if not isinstance(work_family, str) or not work_family:
            raise ValueError(f"admission line {line_number} missing work_family_id")
        if fixture in seen_fixture:
            raise ValueError(f"duplicate grounded fixture admission: {fixture}")
        seen_fixture.add(fixture)
        result.append(GroundedControl(
            cue_id=cue_by_fixture[fixture],
            fixture_path=fixture,
            candidate=candidate,
            soundtrack_id=corpus_id_from_fixture(fixture),
            work_family_id=work_family,
            confidence=confidence,
            status=str(entry["status"]),
        ))
    if len(result) < 2:
        raise ValueError("calibration requires at least two grounded controls present in the panel")
    return result


def candidate_affinity(
    query_id: str,
    candidate_controls: list[GroundedControl],
    matrix: dict[str, dict[str, float]],
    *,
    exclude_soundtrack: str | None = None,
) -> float | None:
    by_soundtrack: dict[str, list[tuple[float, float]]] = defaultdict(list)
    for control in candidate_controls:
        if control.cue_id == query_id:
            continue
        if exclude_soundtrack is not None and control.soundtrack_id == exclude_soundtrack:
            continue
        by_soundtrack[control.soundtrack_id].append(
            (matrix[query_id][control.cue_id], control.confidence)
        )
    if not by_soundtrack:
        return None

    soundtrack_scores: list[float] = []
    for observations in by_soundtrack.values():
        weight = sum(confidence for _score, confidence in observations)
        if weight <= 0.0:
            continue
        soundtrack_scores.append(
            sum(score * confidence for score, confidence in observations) / weight
        )
    if not soundtrack_scores:
        return None
    return sum(soundtrack_scores) / len(soundtrack_scores)


def candidate_scores(
    query_id: str,
    candidates: list[str],
    controls_by_candidate: dict[str, list[GroundedControl]],
    matrix: dict[str, dict[str, float]],
    *,
    exclude_soundtrack: str | None = None,
) -> dict[str, float | None]:
    return {
        candidate: candidate_affinity(
            query_id,
            controls_by_candidate[candidate],
            matrix,
            exclude_soundtrack=exclude_soundtrack,
        )
        for candidate in candidates
    }


def rank_scores(scores: dict[str, float | None], minimum_margin: float) -> dict[str, Any]:
    available = sorted(
        ((score, candidate) for candidate, score in scores.items() if score is not None),
        reverse=True,
    )
    complete = len(available) == len(scores) and len(available) >= 2
    if len(available) < 2:
        return {
            "complete_candidate_coverage": complete,
            "winner": None,
            "winner_margin": None,
            "decisive": False,
        }
    winner_score, winner = available[0]
    runner_score, _runner = available[1]
    margin = winner_score - runner_score
    return {
        "complete_candidate_coverage": complete,
        "winner": winner if complete else None,
        "winner_margin": margin,
        "decisive": complete and margin >= minimum_margin,
    }


def policy_fixture_sets(policy: dict[str, Any]) -> dict[str, dict[str, dict[str, Any]]]:
    result: dict[str, dict[str, dict[str, Any]]] = {
        "evaluation_only_clues": {},
        "shared_composition_holdouts": {},
        "disputed_mapping_holdouts": {},
        "third_party_decoys": {},
    }
    for lane in result:
        entries = policy.get(lane, [])
        if not isinstance(entries, list):
            raise ValueError(f"policy lane {lane} must be a list")
        for entry in entries:
            if not isinstance(entry, dict) or not isinstance(entry.get("fixture_path"), str):
                raise ValueError(f"policy lane {lane} has invalid entry")
            result[lane][entry["fixture_path"]] = entry
    return result


def evaluate(
    *,
    matrix: dict[str, dict[str, float]],
    frozen_sha256: str,
    fixture_by_cue: dict[str, str],
    controls: list[GroundedControl],
    policy: dict[str, Any],
    minimum_margin: float,
    false_positive_threshold: float,
) -> dict[str, Any]:
    controls_by_candidate: dict[str, list[GroundedControl]] = defaultdict(list)
    control_by_cue: dict[str, GroundedControl] = {}
    for control in controls:
        controls_by_candidate[control.candidate].append(control)
        control_by_cue[control.cue_id] = control
    candidates = sorted(controls_by_candidate)
    if len(candidates) < 2:
        raise ValueError("calibration requires at least two composer candidates")

    grounded_results: list[dict[str, Any]] = []
    diagnostic_correct = 0
    diagnostic_decisive = 0
    strict_correct = 0
    strict_decisive = 0
    strict_complete = 0
    for cue_id, truth in sorted(control_by_cue.items()):
        loose_scores = candidate_scores(cue_id, candidates, controls_by_candidate, matrix)
        loose_rank = rank_scores(loose_scores, minimum_margin)
        if loose_rank["decisive"]:
            diagnostic_decisive += 1
            diagnostic_correct += int(loose_rank["winner"] == truth.candidate)

        strict_scores = candidate_scores(
            cue_id,
            candidates,
            controls_by_candidate,
            matrix,
            exclude_soundtrack=truth.soundtrack_id,
        )
        strict_rank = rank_scores(strict_scores, minimum_margin)
        if strict_rank["complete_candidate_coverage"]:
            strict_complete += 1
        if strict_rank["decisive"]:
            strict_decisive += 1
            strict_correct += int(strict_rank["winner"] == truth.candidate)

        grounded_results.append({
            "cue_id": cue_id,
            "fixture_path": truth.fixture_path,
            "expected_candidate": truth.candidate,
            "soundtrack_id": truth.soundtrack_id,
            "work_family_id": truth.work_family_id,
            "admission_status": truth.status,
            "admission_confidence": truth.confidence,
            "diagnostic_leave_one_cue_out": {
                "candidate_scores": loose_scores,
                **loose_rank,
                "correct_if_decisive": (
                    loose_rank["winner"] == truth.candidate if loose_rank["decisive"] else None
                ),
                "claim_boundary": "Diagnostic only; same-soundtrack controls are allowed and may contain implementation/context confounds."
            },
            "strict_leave_soundtrack_out": {
                "candidate_scores": strict_scores,
                **strict_rank,
                "correct_if_decisive": (
                    strict_rank["winner"] == truth.candidate if strict_rank["decisive"] else None
                ),
                "claim_boundary": "Creator-facing transfer test; every grounded control from the query soundtrack is removed before ranking."
            },
        })

    lanes = policy_fixture_sets(policy)
    lane_by_fixture: dict[str, tuple[str, dict[str, Any]]] = {}
    for lane, entries in lanes.items():
        for fixture, entry in entries.items():
            lane_by_fixture[fixture] = (lane, entry)

    validation_by_corpus: dict[str, list[dict[str, Any]]] = defaultdict(list)
    stress_results: list[dict[str, Any]] = []
    for cue_id, fixture in sorted(fixture_by_cue.items()):
        if cue_id in control_by_cue:
            continue
        scores = candidate_scores(cue_id, candidates, controls_by_candidate, matrix)
        ranked = rank_scores(scores, minimum_margin)
        lane_entry = lane_by_fixture.get(fixture)
        if lane_entry is not None:
            lane, evidence = lane_entry
            item: dict[str, Any] = {
                "cue_id": cue_id,
                "fixture_path": fixture,
                "lane": lane,
                "candidate_scores": scores,
                **ranked,
            }
            if lane == "evaluation_only_clues":
                item["historical_clue_candidate"] = evidence.get("candidate")
                item["historical_clue_confidence"] = evidence.get("confidence")
                item["agrees_with_clue_if_decisive"] = (
                    ranked["winner"] == evidence.get("candidate") if ranked["decisive"] else None
                )
            elif lane == "shared_composition_holdouts":
                expected = evidence.get("composers")
                item["documented_composers"] = expected
                available = [score for score in scores.values() if score is not None]
                item["candidate_balance_gap"] = (
                    max(available) - min(available) if len(available) >= 2 else None
                )
            elif lane == "third_party_decoys":
                item["documented_other_candidate"] = evidence.get("candidate")
                available = [score for score in scores.values() if score is not None]
                maximum = max(available) if available else None
                item["max_cube_candidate_affinity"] = maximum
                item["false_positive_risk"] = (
                    maximum is not None and maximum >= false_positive_threshold
                )
            stress_results.append(item)
            continue

        corpus_id = corpus_id_from_fixture(fixture)
        validation_by_corpus[corpus_id].append({
            "cue_id": cue_id,
            "fixture_path": fixture,
            "candidate_scores": scores,
            **ranked,
            "claim_boundary": "Unlabeled/team-level validation only; winner is an affinity lean, not an authorship result."
        })

    validation_worlds: dict[str, Any] = {}
    for corpus_id, items in sorted(validation_by_corpus.items()):
        means: dict[str, float | None] = {}
        for candidate in candidates:
            values = [
                float(item["candidate_scores"][candidate])
                for item in items
                if item["candidate_scores"].get(candidate) is not None
            ]
            means[candidate] = sum(values) / len(values) if values else None
        validation_worlds[corpus_id] = {
            "cue_count": len(items),
            "mean_candidate_affinity": means,
            "cues": items,
            "claim_boundary": "Corpus personnel/team evidence constrains interpretation but does not provide cue-level correctness labels."
        }

    candidate_grounding = {
        candidate: {
            "cue_count": len(grouped),
            "soundtracks": sorted({item.soundtrack_id for item in grouped}),
            "work_family_count": len({(item.soundtrack_id, item.work_family_id) for item in grouped}),
        }
        for candidate, grouped in sorted(controls_by_candidate.items())
    }

    limitations: list[str] = []
    for candidate, summary in candidate_grounding.items():
        if len(summary["soundtracks"]) < 2:
            limitations.append(
                f"{candidate} has grounded composer controls in only {len(summary['soundtracks'])} soundtrack world(s); strict cross-soundtrack generalization for that candidate remains underdetermined."
            )

    return {
        "model": "post-freeze Cube composer calibration reveal",
        "claim_boundary": (
            "Creator identities are joined only after the opaque SPC similarity matrix is frozen. "
            "Grounded cue-level admissions support evaluation; team/partial corpora remain unlabeled validation worlds."
        ),
        "frozen_corpus_sha256": frozen_sha256,
        "candidate_grounding": candidate_grounding,
        "thresholds": {
            "minimum_winner_margin": minimum_margin,
            "third_party_false_positive_threshold": false_positive_threshold,
        },
        "grounded_evaluation": {
            "cue_count": len(grounded_results),
            "diagnostic_decisive_count": diagnostic_decisive,
            "diagnostic_correct_count": diagnostic_correct,
            "diagnostic_accuracy_when_decisive": (
                diagnostic_correct / diagnostic_decisive if diagnostic_decisive else None
            ),
            "strict_complete_candidate_coverage_count": strict_complete,
            "strict_decisive_count": strict_decisive,
            "strict_correct_count": strict_correct,
            "strict_accuracy_when_decisive": (
                strict_correct / strict_decisive if strict_decisive else None
            ),
            "cues": grounded_results,
        },
        "validation_worlds": validation_worlds,
        "stress_holdouts": stress_results,
        "limitations": limitations,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--freeze", type=pathlib.Path, required=True)
    parser.add_argument("--panel", type=pathlib.Path, required=True)
    parser.add_argument("--admissions", type=pathlib.Path, required=True)
    parser.add_argument("--policy", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--minimum-margin", type=float, default=DEFAULT_MINIMUM_MARGIN)
    parser.add_argument(
        "--false-positive-threshold",
        type=float,
        default=DEFAULT_FALSE_POSITIVE_THRESHOLD,
    )
    args = parser.parse_args()

    if not 0.0 <= args.minimum_margin <= 1.0:
        raise ValueError("minimum margin must be in [0,1]")
    if not 0.0 <= args.false_positive_threshold <= 1.0:
        raise ValueError("false-positive threshold must be in [0,1]")

    fixture_by_cue, cue_by_fixture = load_panel(args.panel)
    matrix, frozen_sha256 = load_frozen(args.freeze, set(fixture_by_cue))
    controls = load_grounded_controls(args.admissions, cue_by_fixture)
    policy = json.loads(args.policy.read_text(encoding="utf-8"))
    if not isinstance(policy, dict):
        raise ValueError("calibration policy must be a JSON object")

    result = evaluate(
        matrix=matrix,
        frozen_sha256=frozen_sha256,
        fixture_by_cue=fixture_by_cue,
        controls=controls,
        policy=policy,
        minimum_margin=args.minimum_margin,
        false_positive_threshold=args.false_positive_threshold,
    )
    result["reveal_inputs"] = {
        "panel_sha256": file_sha256(args.panel),
        "admissions_sha256": file_sha256(args.admissions),
        "policy_sha256": file_sha256(args.policy),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
