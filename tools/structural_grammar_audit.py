#!/usr/bin/env python3
"""Aggregate creator-free musical-structure observations across soundtracks.

The input is intentionally downstream of musical extraction and upstream of
creator labels. It groups recurring rule signatures across independent works
and soundtracks, using the same basic epistemic idea as the C++ composer-grammar
kernel: several views of one work are useful, but do not establish a portable
creator habit.

This tool must not be given composer/artist attribution fields. Freeze its
output before any identity-bearing evaluation is joined later.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
from pathlib import Path
from typing import Any, Iterable


SCHEMA = "gmi-structural-grammar-observations-v1"
GROUNDING_THRESHOLD = 0.60
FORBIDDEN_IDENTITY_KEYS = {
    "artist",
    "artist_name",
    "attribution",
    "candidate",
    "composer",
    "composer_name",
    "creator",
    "creator_name",
    "target_control_people",
}


def _load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object in {path}")
    return value


def _reject_identity_fields(value: Any, *, path: str = "root") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() in FORBIDDEN_IDENTITY_KEYS:
                raise ValueError(f"identity-bearing field {key!r} forbidden at {path}")
            _reject_identity_fields(child, path=f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_identity_fields(child, path=f"{path}[{index}]")


def _validated_observation(raw: Any, source_path: Path) -> dict[str, Any]:
    if not isinstance(raw, dict):
        raise ValueError(f"non-object observation in {source_path}")
    required = {
        "soundtrack_id",
        "work_family_id",
        "representation",
        "rule_key",
        "dimension",
        "role_scope",
        "confidence",
        "source",
    }
    missing = sorted(required.difference(raw))
    if missing:
        raise ValueError(f"missing fields {missing} in {source_path}")

    result = {key: raw[key] for key in required}
    result["detail"] = raw.get("detail", "")
    for key in (
        "soundtrack_id",
        "work_family_id",
        "representation",
        "rule_key",
        "dimension",
        "role_scope",
        "source",
        "detail",
    ):
        if not isinstance(result[key], str):
            raise ValueError(f"{key} must be a string in {source_path}")
    for key in ("soundtrack_id", "work_family_id", "rule_key", "source"):
        if not result[key]:
            raise ValueError(f"{key} must be non-empty in {source_path}")

    confidence = result["confidence"]
    if isinstance(confidence, bool) or not isinstance(confidence, (int, float)):
        raise ValueError(f"confidence must be numeric in {source_path}")
    confidence = float(confidence)
    if not 0.0 <= confidence <= 1.0:
        raise ValueError(f"confidence must be in [0, 1] in {source_path}")
    result["confidence"] = confidence
    result["input_file"] = str(source_path)
    return result


def load_observations(paths: Iterable[Path]) -> list[dict[str, Any]]:
    observations: list[dict[str, Any]] = []
    for path in paths:
        payload = _load(path)
        _reject_identity_fields(payload)
        if payload.get("schema") != SCHEMA:
            raise ValueError(f"unsupported structural grammar schema in {path}")
        raw_observations = payload.get("observations")
        if not isinstance(raw_observations, list):
            raise ValueError(f"observations must be an array in {path}")
        observations.extend(
            _validated_observation(raw, path) for raw in raw_observations
        )
    return observations


def _second_strongest(values: Iterable[float]) -> float:
    ordered = sorted(values, reverse=True)
    if not ordered:
        return 0.0
    return ordered[1] if len(ordered) >= 2 else ordered[0]


def audit_observations(observations: list[dict[str, Any]]) -> dict[str, Any]:
    by_rule: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for observation in observations:
        by_rule[str(observation["rule_key"])].append(observation)

    rules: list[dict[str, Any]] = []
    for rule_key, items in by_rule.items():
        soundtrack_support: dict[str, float] = {}
        work_support: dict[tuple[str, str], float] = {}
        representations: set[str] = set()
        dimensions: set[str] = set()
        roles: set[str] = set()

        for item in items:
            if item["confidence"] < GROUNDING_THRESHOLD:
                continue
            soundtrack = str(item["soundtrack_id"])
            work = (soundtrack, str(item["work_family_id"]))
            soundtrack_support[soundtrack] = max(
                soundtrack_support.get(soundtrack, 0.0),
                float(item["confidence"]),
            )
            work_support[work] = max(
                work_support.get(work, 0.0),
                float(item["confidence"]),
            )
            representations.add(str(item["representation"]))
            dimensions.add(str(item["dimension"]))
            roles.add(str(item["role_scope"]))

        independent_soundtrack_ceiling = _second_strongest(
            soundtrack_support.values()
        )
        independent_work_ceiling = _second_strongest(work_support.values())
        portable_ceiling = min(
            independent_soundtrack_ceiling,
            independent_work_ceiling,
        )
        cross_work = len(work_support) >= 2
        cross_soundtrack = len(soundtrack_support) >= 2

        rules.append(
            {
                "rule_key": rule_key,
                "observation_count": len(items),
                "grounding_observation_count": sum(
                    item["confidence"] >= GROUNDING_THRESHOLD for item in items
                ),
                "independent_work_family_count": len(work_support),
                "independent_soundtrack_count": len(soundtrack_support),
                "representation_count": len(representations),
                "dimensions": sorted(dimensions),
                "role_scopes": sorted(roles),
                "cross_work_grounded": cross_work,
                "cross_soundtrack_grounded": cross_soundtrack,
                "portable_support_ceiling": portable_ceiling,
                "eligible_for_candidate_aggregation": cross_work and cross_soundtrack,
                "soundtracks": sorted(soundtrack_support),
                "works": [
                    {"soundtrack_id": soundtrack, "work_family_id": work}
                    for soundtrack, work in sorted(work_support)
                ],
            }
        )

    rules.sort(
        key=lambda item: (
            not bool(item["eligible_for_candidate_aggregation"]),
            -int(item["independent_soundtrack_count"]),
            -float(item["portable_support_ceiling"]),
            str(item["rule_key"]),
        )
    )
    return {
        "schema": "gmi-structural-grammar-audit-v1",
        "stage": "blind-cross-soundtrack-structural-grammar",
        "input_schema": SCHEMA,
        "identity_firewall": (
            "Inputs containing composer/artist/candidate/creator attribution fields are rejected. "
            "Freeze this output before joining identity labels."
        ),
        "grounding_threshold": GROUNDING_THRESHOLD,
        "observation_count": len(observations),
        "rule_count": len(rules),
        "portable_rule_count": sum(
            bool(rule["eligible_for_candidate_aggregation"]) for rule in rules
        ),
        "rules": rules,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    result = audit_observations(load_observations(args.inputs))
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
