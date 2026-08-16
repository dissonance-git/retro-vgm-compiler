#!/usr/bin/env python3
"""Emit role-safe blind attribution matches from admitted Genesis VGM controls.

The feature extractor never receives creator labels. All query/control VGM files
are analyzed first; candidate labels are joined only afterward from an
attribution-control registry produced by attribution_control_registry.py.

Composer-role matches use only the current musical-trajectory view. Realization
features are reserved for arrangement/programming, driver/toolchain, and
patch/sample roles. This tool emits evidence for the higher-level blind
attribution experiment; it does not pick a winner itself.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import sys
from typing import Iterable


VALID_ROLES = {
    "composer",
    "arranger_programmer",
    "driver_toolchain",
    "patch_sample_designer",
}


def _load_vgm_audit():
    path = pathlib.Path(__file__).with_name("vgm_creator_feature_audit.py")
    spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


base = _load_vgm_audit()


def _canonical_path(path: pathlib.Path) -> str:
    return path.as_posix()


def _is_vgm_path(path: str) -> bool:
    return pathlib.Path(path).suffix.lower() in {".vgm", ".vgz"}


def _read_registry(path: pathlib.Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("control registry must be a JSON object")
    controls = value.get("controls")
    if not isinstance(controls, list):
        raise ValueError("control registry requires a controls list")
    return value


def eligible_controls(
    registry: dict[str, object],
    role: str,
    candidates: set[str] | None = None,
) -> list[dict[str, object]]:
    if role not in VALID_ROLES:
        raise ValueError(f"unsupported attribution role {role!r}")
    raw_controls = registry.get("controls", [])
    if not isinstance(raw_controls, list):
        raise ValueError("control registry controls must be a list")

    controls: list[dict[str, object]] = []
    for raw in raw_controls:
        if not isinstance(raw, dict):
            raise ValueError("control registry entries must be objects")
        if raw.get("role") != role:
            continue
        candidate = raw.get("candidate")
        fixture = raw.get("fixture_path")
        if not isinstance(candidate, str) or not candidate:
            raise ValueError("admitted control requires candidate")
        if candidates is not None and candidate not in candidates:
            continue
        if not isinstance(fixture, str) or not fixture:
            raise ValueError("admitted control requires fixture_path")
        if not _is_vgm_path(fixture):
            continue
        controls.append(raw)

    return sorted(
        controls,
        key=lambda item: (
            str(item.get("candidate", "")),
            str(item.get("corpus_id", "")),
            str(item.get("fixture_path", "")),
        ),
    )


def extract_blind_features(paths: Iterable[pathlib.Path]) -> dict[str, dict[str, object]]:
    # No labels or candidate identities enter this function.
    features: dict[str, dict[str, object]] = {}
    unique = sorted({_canonical_path(path): path for path in paths}.items())
    for key, path in unique:
        if not path.is_file():
            raise FileNotFoundError(path)
        if path.suffix.lower() not in {".vgm", ".vgz"}:
            raise ValueError(f"blind Genesis feature extraction does not support {path}")
        features[key] = base.audit_file(path)
    return features


def _dimension_for_role(role: str) -> str:
    if role == "composer":
        return "melody"
    if role == "patch_sample_designer":
        return "timbre_synthesis"
    if role == "arranger_programmer":
        return "arrangement_orchestration"
    return "performance_execution"


def _similarity_for_role(
    role: str,
    query_feature: dict[str, object],
    control_feature: dict[str, object],
) -> tuple[str, float]:
    if role == "composer":
        return "musical_trajectory", float(
            base.structural_similarity(query_feature, control_feature)
        )
    return "realization", float(base.realization_similarity(query_feature, control_feature))


def build_match_manifest(
    *,
    query_id: str,
    query_path: pathlib.Path,
    role: str,
    query_feature: dict[str, object],
    controls: list[dict[str, object]],
    control_features: dict[str, dict[str, object]],
    query_platform_id: str = "",
    query_implementation_family_id: str = "",
) -> dict[str, object]:
    if not query_id:
        raise ValueError("query_id is required")
    if role not in VALID_ROLES:
        raise ValueError(f"unsupported attribution role {role!r}")

    matches: list[dict[str, object]] = []
    for control in controls:
        fixture = str(control["fixture_path"])
        candidate = str(control["candidate"])
        if pathlib.Path(fixture) == query_path:
            continue
        feature = control_features.get(pathlib.Path(fixture).as_posix())
        if feature is None:
            raise ValueError(f"missing blind feature extraction for admitted control {fixture}")

        view, strength = _similarity_for_role(role, query_feature, feature)
        confidence_value = control.get("confidence")
        if not isinstance(confidence_value, (int, float)):
            raise ValueError(f"control {fixture} has invalid confidence")
        confidence = float(confidence_value)
        if confidence < 0.0 or confidence > 1.0:
            raise ValueError(f"control {fixture} confidence must be in [0, 1]")

        work_family = str(control.get("work_family_id", ""))
        if not work_family:
            work_family = pathlib.Path(fixture).stem

        matches.append(
            {
                "query_id": query_id,
                "candidate": candidate,
                "role": role,
                "control_id": fixture,
                "soundtrack_id": str(control.get("corpus_id", "")),
                "work_family_id": work_family,
                "platform_id": str(control.get("platform", "")),
                "implementation_family_id": str(
                    control.get("implementation_family_id", "")
                ),
                "representation": "driver_execution",
                "dimension": _dimension_for_role(role),
                "polarity": "supports",
                # Similarity is an inference even when the control's historical
                # role credit is exact.
                "status": "hypothesis",
                "match_strength": strength,
                "confidence": confidence,
                "source": f"blind-genesis-vgm:{view}",
                "detail": (
                    f"blind {view} similarity to admitted control; "
                    f"control_admission_status={control.get('status', '')}; "
                    f"control_admission_source={control.get('source', '')}"
                ),
            }
        )

    return {
        "model": "role-safe blind attribution match manifest",
        "query_id": query_id,
        "query_path": query_path.as_posix(),
        "role": role,
        "query_platform_id": query_platform_id,
        "query_implementation_family_id": query_implementation_family_id,
        "claim_boundary": (
            "Composer matches use only the current blind musical-trajectory view; "
            "Genesis realization similarity is never promoted into composer support. "
            "All candidate labels are joined after blind feature extraction, and all "
            "controls must already have passed the evidence-safe admission registry."
        ),
        "match_count": len(matches),
        "matches": sorted(
            matches,
            key=lambda item: (
                str(item["candidate"]),
                -float(item["match_strength"]),
                str(item["control_id"]),
            ),
        ),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("query", type=pathlib.Path)
    parser.add_argument("--registry", type=pathlib.Path, required=True)
    parser.add_argument("--query-id", required=True)
    parser.add_argument("--role", choices=sorted(VALID_ROLES), required=True)
    parser.add_argument("--candidate", action="append", default=[])
    parser.add_argument("--query-platform", default="")
    parser.add_argument("--query-implementation-family", default="")
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    registry = _read_registry(args.registry)
    candidate_filter = set(args.candidate) if args.candidate else None
    controls = eligible_controls(registry, args.role, candidate_filter)
    if not controls:
        raise ValueError("no admitted VGM/VGZ controls match the requested role/filter")

    query_key = args.query.as_posix()
    feature_paths = [args.query] + [pathlib.Path(str(item["fixture_path"])) for item in controls]
    all_features = extract_blind_features(feature_paths)
    query_feature = all_features[query_key]

    manifest = build_match_manifest(
        query_id=args.query_id,
        query_path=args.query,
        role=args.role,
        query_feature=query_feature,
        controls=controls,
        control_features=all_features,
        query_platform_id=args.query_platform,
        query_implementation_family_id=args.query_implementation_family,
    )
    text = json.dumps(manifest, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
