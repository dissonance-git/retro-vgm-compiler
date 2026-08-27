#!/usr/bin/env python3
"""Run bounded label-blind SPC runtime corpus pressure.

This orchestration layer never reads ID666/catalog/creator metadata. It runs the
pinned forensic extractor on explicitly supplied SPC objects, validates the
resulting runtime/part evidence, and emits an aggregate observatory summary.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
from typing import Any


FORBIDDEN_FEATURE_KEYS = {
    "artist",
    "author",
    "catalog",
    "composer",
    "creator",
    "game",
    "title",
}


def _require_int(mapping: dict[str, Any], key: str) -> int:
    value = mapping.get(key)
    if not isinstance(value, int):
        raise ValueError(f"{key} must be an integer")
    return value


def validate_sidecar(payload: dict[str, Any]) -> dict[str, int]:
    if payload.get("model") != "label-blind SPC forensic feature sidecar":
        raise ValueError("unexpected SPC forensic sidecar model")

    controlled = payload.get("controlled_execution")
    capture = payload.get("capture")
    replay = payload.get("replay")
    features = payload.get("features")
    if not all(isinstance(item, dict) for item in (controlled, capture, replay, features)):
        raise ValueError("SPC forensic sidecar is missing required sections")

    if _require_int(controlled, "source_bytes") <= 0:
        raise ValueError("SPC forensic source must contain bytes")
    if _require_int(controlled, "requested_seconds") <= 0:
        raise ValueError("SPC forensic execution window must be positive")

    stored = _require_int(capture, "stored_event_count")
    dropped = _require_int(capture, "dropped_event_count")
    overflowed = _require_int(capture, "overflowed_window_count")
    cross_lane_backsteps = _require_int(capture, "cross_lane_backstep_count")
    max_cross_lane_backstep = _require_int(capture, "max_cross_lane_backstep_ticks")
    if stored <= 0:
        raise ValueError("SPC runtime pressure requires stored runtime events")
    if dropped != 0 or overflowed != 0:
        raise ValueError("SPC runtime pressure requires lossless capture")
    if cross_lane_backsteps < 0 or max_cross_lane_backstep < 0:
        raise ValueError("SPC cross-lane backstep diagnostics must be nonnegative")
    if cross_lane_backsteps == 0 and max_cross_lane_backstep != 0:
        raise ValueError("SPC backstep magnitude requires at least one cross-lane backstep")
    if cross_lane_backsteps > 0 and max_cross_lane_backstep == 0:
        raise ValueError("SPC cross-lane backsteps require a positive recorded magnitude")

    materialized = _require_int(replay, "records_materialized")
    continuity_breaks = _require_int(replay, "continuity_breaks")
    if materialized != stored:
        raise ValueError("SPC replay/materialization count must match stored events")
    if continuity_breaks != 0:
        raise ValueError("SPC runtime pressure requires no replay continuity gaps")

    voice_episodes = _require_int(features, "voice_episode_count")
    eligible = _require_int(features, "eligible_episode_count")
    candidates = _require_int(features, "candidate_transition_count")
    strong = _require_int(features, "strong_transition_count")
    rejected = _require_int(features, "rejected_transition_count")
    emitted = _require_int(features, "emitted_part_count")
    profiles = _require_int(features, "part_profile_count")

    if voice_episodes <= 0 or eligible <= 0:
        raise ValueError("SPC runtime pressure requires eligible physical voice episodes")
    if eligible > voice_episodes:
        raise ValueError("eligible SPC episodes cannot exceed all voice episodes")
    if strong + rejected != candidates:
        raise ValueError("SPC transition accounting must partition candidate transitions")
    if profiles > emitted:
        raise ValueError("SPC motif profiles cannot exceed emitted persistent-part trajectories")

    profile_values = features.get("part_profiles")
    if not isinstance(profile_values, list) or len(profile_values) != profiles:
        raise ValueError("SPC part profile count does not match serialized profiles")
    for profile in profile_values:
        if not isinstance(profile, dict):
            raise ValueError("SPC part profile must be an object")
        if _require_int(profile, "gesture_count") < 3:
            raise ValueError("SPC motif profile requires at least three gestures")

    def reject_identity_keys(value: Any) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                if key.lower() in FORBIDDEN_FEATURE_KEYS:
                    raise ValueError(f"identity-bearing feature key is forbidden: {key}")
                reject_identity_keys(child)
        elif isinstance(value, list):
            for child in value:
                reject_identity_keys(child)

    reject_identity_keys(payload)

    return {
        "stored_event_count": stored,
        "cross_lane_backstep_count": cross_lane_backsteps,
        "max_cross_lane_backstep_ticks": max_cross_lane_backstep,
        "voice_episode_count": voice_episodes,
        "eligible_episode_count": eligible,
        "candidate_transition_count": candidates,
        "strong_transition_count": strong,
        "rejected_transition_count": rejected,
        "emitted_part_count": emitted,
        "part_profile_count": profiles,
    }


def run_pressure(
    extractor: Path,
    fixtures: list[Path],
    output_dir: Path,
    seconds: int,
) -> dict[str, Any]:
    if seconds <= 0:
        raise ValueError("SPC corpus pressure seconds must be positive")
    if not extractor.is_file():
        raise ValueError(f"SPC forensic extractor is missing: {extractor}")
    if len(fixtures) < 2:
        raise ValueError("SPC runtime corpus pressure requires at least two fixtures")

    output_dir.mkdir(parents=True, exist_ok=True)
    reports: list[dict[str, Any]] = []
    for index, fixture in enumerate(fixtures, start=1):
        if not fixture.is_file():
            raise ValueError(f"SPC pressure fixture is missing: {fixture}")
        sidecar = output_dir / f"fixture-{index:03d}.json"
        subprocess.run(
            [str(extractor), str(fixture), str(sidecar), str(seconds)],
            check=True,
        )
        payload = json.loads(sidecar.read_text(encoding="utf-8"))
        metrics = validate_sidecar(payload)
        reports.append({
            "fixture_index": index,
            **metrics,
        })

    if not any(item["strong_transition_count"] > 0 for item in reports):
        raise ValueError("SPC corpus pressure requires at least one strong part transition")
    if not any(item["rejected_transition_count"] > 0 for item in reports):
        raise ValueError("SPC corpus pressure requires at least one rejected part transition")
    if not any(item["part_profile_count"] > 0 for item in reports):
        raise ValueError("SPC corpus pressure requires at least one real motif profile")

    summary = {
        "schema": "spc-runtime-corpus-pressure-v1",
        "claim_boundary": (
            "Controlled runtime evidence only. Positive and rejected persistent-part "
            "transitions are observatory outputs, not creator identity, phrase syntax, "
            "notation, or cross-source musical equivalence."
        ),
        "fixture_count": len(reports),
        "execution_seconds_per_fixture": seconds,
        "totals": {
            key: sum(item[key] for item in reports)
            for key in (
                "stored_event_count",
                "cross_lane_backstep_count",
                "voice_episode_count",
                "eligible_episode_count",
                "candidate_transition_count",
                "strong_transition_count",
                "rejected_transition_count",
                "emitted_part_count",
                "part_profile_count",
            )
        },
        "max_cross_lane_backstep_ticks": max(
            item["max_cross_lane_backstep_ticks"] for item in reports
        ),
        "fixtures": reports,
        "promotion": {
            "cross_source_family_equivalence": "blocked",
            "phrase_role": "blocked",
            "creator_identity": "blocked",
        },
    }
    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--extractor", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=3)
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("fixtures", type=Path, nargs="+")
    args = parser.parse_args()

    summary = run_pressure(
        args.extractor,
        args.fixtures,
        args.output_dir,
        args.seconds,
    )
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    args.summary.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print("SPC_RUNTIME_CORPUS_PRESSURE", summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
