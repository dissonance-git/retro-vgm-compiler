#!/usr/bin/env python3
"""Primary Sonic 3 integration-testbed entry point.

The runner discovers the Sonic 3 target and predeclared attribution-control
soundtracks from the corpus manifest without reading creator names or curated
track-attribution labels during blind stages.

Current executable lane:
  * inventory: report testbed coverage and lane eligibility.
  * vgm-baseline: run the existing label-blind Genesis VGM trajectory/
    realization audit over Sonic 3 plus eligible cross-soundtrack controls.

Future lanes (persistent parts, SMPS oracle, ROM forensics, SPC musical
relations, composer grammar) should join this entry point rather than creating
parallel testbed metadata.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CORPUS_ROOT = ROOT / "tests" / "corpus"
MANIFEST_PATH = CORPUS_ROOT / "manifest.json"
TARGET_CORPUS_ID = "sonic-3-knuckles"
CONTROL_SELECTION_MARKER = "Sonic 3 attribution-control candidate"


def _load_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"expected object in {path}")
    return data


def _manifest_sets(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    sets = manifest.get("sets")
    if not isinstance(sets, list):
        raise ValueError("corpus manifest has no sets array")
    result: list[dict[str, Any]] = []
    for item in sets:
        if not isinstance(item, dict):
            raise ValueError("corpus manifest contains a non-object set")
        result.append(item)
    return result


def _is_predeclared_control(item: dict[str, Any]) -> bool:
    reason = item.get("selection_reason")
    return isinstance(reason, str) and CONTROL_SELECTION_MARKER.lower() in reason.lower()


def _selected_sets(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    selected: list[dict[str, Any]] = []
    found_target = False
    for item in _manifest_sets(manifest):
        corpus_id = item.get("corpus_id")
        if corpus_id == TARGET_CORPUS_ID:
            found_target = True
            selected.append(item)
        elif _is_predeclared_control(item):
            selected.append(item)
    if not found_target:
        raise ValueError(f"target corpus {TARGET_CORPUS_ID!r} missing from manifest")
    return selected


def _source_family(item: dict[str, Any]) -> str:
    value = item.get("source_family")
    return value.upper() if isinstance(value, str) else "UNKNOWN"


def _devices(item: dict[str, Any]) -> tuple[str, ...]:
    values = item.get("device_families")
    if not isinstance(values, list):
        return ()
    return tuple(sorted(str(value) for value in values))


def _blind_genesis_vgm_eligible(item: dict[str, Any]) -> bool:
    return _source_family(item) in {"VGM", "VGZ"} and "YM2612" in _devices(item)


def _spc_eligible(item: dict[str, Any]) -> bool:
    return _source_family(item) == "SPC"


def _public_set_record(item: dict[str, Any]) -> dict[str, Any]:
    """Return only non-attribution routing data safe for blind-stage output."""

    return {
        "corpus_id": item.get("corpus_id"),
        "path": item.get("path"),
        "work": item.get("work"),
        "source_family": _source_family(item),
        "device_families": list(_devices(item)),
        "fixture_count": item.get("fixture_count"),
        "set_sha256": item.get("set_sha256"),
        "role": "target" if item.get("corpus_id") == TARGET_CORPUS_ID else "control",
        "blind_genesis_vgm_eligible": _blind_genesis_vgm_eligible(item),
        "spc_future_lane_eligible": _spc_eligible(item),
    }


def build_inventory(manifest_path: Path = MANIFEST_PATH) -> dict[str, Any]:
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    if not isinstance(manifest, dict):
        raise ValueError("corpus manifest root must be an object")

    selected = _selected_sets(manifest)
    public = [_public_set_record(item) for item in selected]
    target = [item for item in public if item["role"] == "target"]
    controls = [item for item in public if item["role"] == "control"]

    by_family: dict[str, int] = {}
    for item in public:
        family = str(item["source_family"])
        by_family[family] = by_family.get(family, 0) + 1

    return {
        "testbed": "sonic3-primary-integration-testbed",
        "target_corpus_id": TARGET_CORPUS_ID,
        "blind_policy": (
            "Inventory and extraction stages do not expose curated Sonic 3 track "
            "attributions or target_control_people from external tags."
        ),
        "manifest_sha256": hashlib.sha256(manifest_bytes).hexdigest(),
        "selected_set_count": len(public),
        "control_set_count": len(controls),
        "source_family_counts": dict(sorted(by_family.items())),
        "blind_genesis_vgm_set_count": sum(
            bool(item["blind_genesis_vgm_eligible"]) for item in public
        ),
        "spc_future_lane_set_count": sum(
            bool(item["spc_future_lane_eligible"]) for item in public
        ),
        "target": target[0],
        "controls": controls,
        "capability_lanes": {
            "blind_vgm_trajectory_realization": "executable",
            "persistent_musical_parts": "in_progress",
            "smps_hidden_oracle": "planned",
            "rom_forensics": "planned_separate_from_musical_blind_mode",
            "spc_creator_facing_relations": "planned",
            "phrase_motif_harmony_form": "planned",
            "cross_soundtrack_composer_grammar": "gated_on_deeper_musical_model",
            "blind_composer_attribution": "gated_on_confound_controls",
        },
    }


def _load_cross_soundtrack_audit():
    path = Path(__file__).with_name("cross_soundtrack_vgm_audit.py")
    spec = importlib.util.spec_from_file_location("cross_soundtrack_vgm_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def run_vgm_baseline(
    manifest_path: Path = MANIFEST_PATH,
    neighbor_count: int = 5,
) -> dict[str, Any]:
    manifest = _load_json(manifest_path)
    selected = _selected_sets(manifest)
    eligible = [item for item in selected if _blind_genesis_vgm_eligible(item)]

    target = [item for item in eligible if item.get("corpus_id") == TARGET_CORPUS_ID]
    if len(target) != 1:
        raise ValueError("Sonic 3 target is not uniquely eligible for Genesis VGM audit")

    # Target first makes the experiment contract obvious. Control order is
    # deterministic and contains no creator labels.
    controls = sorted(
        (item for item in eligible if item.get("corpus_id") != TARGET_CORPUS_ID),
        key=lambda item: str(item.get("corpus_id")),
    )
    ordered = target + controls

    corpus_paths: list[Path] = []
    for item in ordered:
        relative = item.get("path")
        if not isinstance(relative, str):
            raise ValueError(f"missing corpus path for {item.get('corpus_id')!r}")
        path = ROOT / relative
        if not path.is_dir():
            raise FileNotFoundError(path)
        corpus_paths.append(path)

    audit = _load_cross_soundtrack_audit()
    result = audit.audit_soundtracks(
        corpus_paths,
        neighbor_count=neighbor_count,
        cross_soundtrack_only=True,
    )
    result["testbed"] = "sonic3-primary-integration-testbed"
    result["stage"] = "blind-vgm-baseline"
    result["target_corpus_id"] = TARGET_CORPUS_ID
    result["eligible_corpus_ids"] = [str(item.get("corpus_id")) for item in ordered]
    result["label_firewall"] = (
        "No curated Sonic 3 attribution labels or external target_control_people "
        "are read by this stage. Unblind only after this JSON is frozen."
    )
    return result


def _write_result(result: dict[str, Any], output: Path | None) -> None:
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8")
    print(text, end="")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--manifest",
        type=Path,
        default=MANIFEST_PATH,
        help="corpus manifest (defaults to tests/corpus/manifest.json)",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    inventory_parser = subparsers.add_parser("inventory")
    inventory_parser.add_argument("--json", type=Path)

    vgm_parser = subparsers.add_parser("vgm-baseline")
    vgm_parser.add_argument("--json", type=Path)
    vgm_parser.add_argument("--neighbors", type=int, default=5)

    args = parser.parse_args()

    if args.command == "inventory":
        _write_result(build_inventory(args.manifest), args.json)
        return 0

    if args.command == "vgm-baseline":
        if args.neighbors < 0:
            parser.error("--neighbors must be >= 0")
        _write_result(
            run_vgm_baseline(args.manifest, neighbor_count=args.neighbors),
            args.json,
        )
        return 0

    parser.error(f"unknown command {args.command!r}")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
