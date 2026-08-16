#!/usr/bin/env python3
"""Primary Sonic 3 integration-testbed entry point.

The runner discovers the Sonic 3 target and predeclared attribution-control
soundtracks from the corpus manifest without reading creator names or curated
track-attribution labels during blind stages.

Current executable lanes:
  * inventory: report testbed coverage and lane eligibility.
  * vgm-baseline: run the label-blind Genesis VGM trajectory/realization audit.
  * vgm-motif-probe: run an exploratory physical-channel local motif audit that
    remains explicitly below persistent-part and phrase truth.
  * vgm-harmonic-probe: run a mixed YM2612+SN76489 surface-harmony pressure test
    that preserves PSG tone/noise role ambiguity and shared-model promotion gates.
  * rom-forensics: run derived-only ROM provenance analysis in a deliberately
    separate forensic mode that must not leak into musical blind attribution.

The shared C++ model contains the stronger persistent-part, motif, harmony,
phrase, tonal-region, function, cadence, and creator-grammar primitives. An
executable exploratory lane must never be advertised as having crossed a shared
model gate that its real-corpus evidence has not earned.
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


def _load_tool(module_name: str, file_name: str):
    path = Path(__file__).with_name(file_name)
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


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


def _ordered_genesis_vgm_sets(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    eligible = [
        item for item in _selected_sets(manifest)
        if _blind_genesis_vgm_eligible(item)
    ]
    target = [item for item in eligible if item.get("corpus_id") == TARGET_CORPUS_ID]
    if len(target) != 1:
        raise ValueError("Sonic 3 target is not uniquely eligible for Genesis VGM audit")
    controls = sorted(
        (item for item in eligible if item.get("corpus_id") != TARGET_CORPUS_ID),
        key=lambda item: str(item.get("corpus_id")),
    )
    return target + controls


def _corpus_paths(sets: list[dict[str, Any]]) -> list[Path]:
    paths: list[Path] = []
    for item in sets:
        relative = item.get("path")
        if not isinstance(relative, str):
            raise ValueError(f"missing corpus path for {item.get('corpus_id')!r}")
        path = ROOT / relative
        if not path.is_dir():
            raise FileNotFoundError(path)
        paths.append(path)
    return paths


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
        "spc_lane_eligible": _spc_eligible(item),
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
        "spc_lane_set_count": sum(bool(item["spc_lane_eligible"]) for item in public),
        "target": target[0],
        "controls": controls,
        "capability_lanes": {
            "blind_vgm_trajectory_realization": "executable",
            "blind_vgm_physical_channel_motif_probe": "executable_exploratory",
            "blind_vgm_surface_harmony_probe": "executable_exploratory_mixed_ym2612_sn76489_with_shared_model_promotion_gates",
            "rom_forensics": "executable_separate_from_musical_blind_mode",
            "persistent_musical_parts": "implemented_shared_model_pending_real_corpus_execution",
            "cross_architecture_motif_profiles": "implemented_shared_model_pending_real_corpus_execution",
            "spc_creator_facing_relations": "implemented_part_motif_phrase_adapters_pending_real_corpus_execution",
            "phrase_motif_harmony_form": "mixed_surface_harmony_executable; persistent_part_phrase_grounded_harmony_form_pending",
            "smps_hidden_oracle": "planned_real_corpus_supervision_lane",
            "cross_soundtrack_composer_grammar": "implemented_evidence_kernel_pending_real_observations",
            "blind_composer_attribution": "gated_on_frozen_blind_outputs_and_confound_controls",
        },
    }


def run_vgm_baseline(
    manifest_path: Path = MANIFEST_PATH,
    neighbor_count: int = 5,
) -> dict[str, Any]:
    manifest = _load_json(manifest_path)
    ordered = _ordered_genesis_vgm_sets(manifest)
    corpus_paths = _corpus_paths(ordered)

    audit = _load_tool("cross_soundtrack_vgm_audit", "cross_soundtrack_vgm_audit.py")
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


def run_vgm_motif_probe(
    manifest_path: Path = MANIFEST_PATH,
    *,
    neighbor_count: int = 5,
    window_events: int = 4,
) -> dict[str, Any]:
    manifest = _load_json(manifest_path)
    ordered = _ordered_genesis_vgm_sets(manifest)
    corpus_paths = _corpus_paths(ordered)

    probe = _load_tool("vgm_motif_probe", "vgm_motif_probe.py")
    result = probe.audit_soundtracks(
        corpus_paths,
        window_events=window_events,
        neighbor_count=neighbor_count,
        cross_soundtrack_only=True,
    )
    result["testbed"] = "sonic3-primary-integration-testbed"
    result["stage"] = "blind-vgm-physical-channel-motif-probe"
    result["target_corpus_id"] = TARGET_CORPUS_ID
    result["eligible_corpus_ids"] = [str(item.get("corpus_id")) for item in ordered]
    result["label_firewall"] = (
        "No curated Sonic 3 attribution labels or external target_control_people are read. "
        "Freeze this output before unblinding."
    )
    result["model_firewall"] = (
        "This probe is below persistent-part recovery. A high motif-probe similarity may "
        "motivate deeper analysis but is not composer evidence."
    )
    return result


def run_vgm_harmonic_probe(
    manifest_path: Path = MANIFEST_PATH,
    *,
    neighbor_count: int = 5,
    pitch_tolerance_cents: float = 35.0,
    presence_floor_ratio: float = 0.01,
) -> dict[str, Any]:
    manifest = _load_json(manifest_path)
    ordered = _ordered_genesis_vgm_sets(manifest)
    corpus_paths = _corpus_paths(ordered)

    probe = _load_tool("sonic3_mixed_harmonic_probe", "sonic3_mixed_harmonic_probe.py")
    result = probe.audit_soundtracks(
        corpus_paths,
        neighbor_count=neighbor_count,
        cross_soundtrack_only=True,
        pitch_tolerance_cents=pitch_tolerance_cents,
        presence_floor_ratio=presence_floor_ratio,
    )
    result["testbed"] = "sonic3-primary-integration-testbed"
    result["stage"] = "blind-vgm-mixed-surface-harmony-pressure-test"
    result["target_corpus_id"] = TARGET_CORPUS_ID
    result["eligible_corpus_ids"] = [str(item.get("corpus_id")) for item in ordered]
    result["label_firewall"] = (
        "No curated Sonic 3 attribution labels or external target_control_people are read. "
        "Freeze this output before any attribution unblind."
    )
    result["model_firewall"] = (
        "Mixed YM2612+SN76489 surface pitch may rank harmonic candidates, but PSG tone can "
        "be an independent part or an FM doubling/shadow and PSG noise can be percussion or "
        "texture. The shared model still requires persistent musical parts, structural pitch "
        "collections, cross-origin tonal-center support, voice leading, role grounding, and "
        "phrase arrival before key, function, cadence, tonicization, or modulation may be promoted."
    )
    return result


def run_rom_forensics(
    rom: Path,
    *,
    compare: Path | None = None,
    needles: list[str] | None = None,
    needle_file: Path | None = None,
    min_string: int = 5,
    min_padding: int = 32,
    min_equal_run: int = 64,
    block_size: int = 4096,
    max_items: int = 2000,
) -> dict[str, Any]:
    forensic = _load_tool("sonic3_rom_forensics", "sonic3_rom_forensics.py")
    needle_values = forensic._read_needles(needles or [], needle_file)
    left = rom.read_bytes()
    result: dict[str, Any] = {
        "testbed": "sonic3-primary-integration-testbed",
        "stage": "rom-forensics",
        "mode": "forensic-only",
        "musical_blind_firewall": (
            "This stage may expose provenance shortcuts. Freeze musical-blind "
            "outputs before admitting any result from this object."
        ),
        "scan": forensic.scan_bytes(
            left,
            source_name=str(rom),
            needles=needle_values,
            min_string=min_string,
            min_padding=min_padding,
            max_items=max_items,
        ),
    }
    if compare is not None:
        right = compare.read_bytes()
        result["comparison_source"] = str(compare)
        result["comparison"] = forensic.compare_bytes(
            left,
            right,
            min_equal_run=min_equal_run,
            block_size=block_size,
            max_items=max_items,
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

    motif_parser = subparsers.add_parser("vgm-motif-probe")
    motif_parser.add_argument("--json", type=Path)
    motif_parser.add_argument("--neighbors", type=int, default=5)
    motif_parser.add_argument("--window-events", type=int, default=4)

    harmonic_parser = subparsers.add_parser("vgm-harmonic-probe")
    harmonic_parser.add_argument("--json", type=Path)
    harmonic_parser.add_argument("--neighbors", type=int, default=5)
    harmonic_parser.add_argument("--pitch-tolerance-cents", type=float, default=35.0)
    harmonic_parser.add_argument("--presence-floor-ratio", type=float, default=0.01)

    rom_parser = subparsers.add_parser("rom-forensics")
    rom_parser.add_argument("rom", type=Path)
    rom_parser.add_argument("--compare", type=Path)
    rom_parser.add_argument("--json", type=Path)
    rom_parser.add_argument("--needle", action="append", default=[])
    rom_parser.add_argument("--needle-file", type=Path)
    rom_parser.add_argument("--min-string", type=int, default=5)
    rom_parser.add_argument("--min-padding", type=int, default=32)
    rom_parser.add_argument("--min-equal-run", type=int, default=64)
    rom_parser.add_argument("--block-size", type=int, default=4096)
    rom_parser.add_argument("--max-items", type=int, default=2000)

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

    if args.command == "vgm-motif-probe":
        if args.neighbors < 0:
            parser.error("--neighbors must be >= 0")
        if args.window_events < 3:
            parser.error("--window-events must be >= 3")
        _write_result(
            run_vgm_motif_probe(
                args.manifest,
                neighbor_count=args.neighbors,
                window_events=args.window_events,
            ),
            args.json,
        )
        return 0

    if args.command == "vgm-harmonic-probe":
        if args.neighbors < 0:
            parser.error("--neighbors must be >= 0")
        if args.pitch_tolerance_cents <= 0.0:
            parser.error("--pitch-tolerance-cents must be > 0")
        if not 0.0 < args.presence_floor_ratio < 1.0:
            parser.error("--presence-floor-ratio must lie in (0, 1)")
        _write_result(
            run_vgm_harmonic_probe(
                args.manifest,
                neighbor_count=args.neighbors,
                pitch_tolerance_cents=args.pitch_tolerance_cents,
                presence_floor_ratio=args.presence_floor_ratio,
            ),
            args.json,
        )
        return 0

    if args.command == "rom-forensics":
        if args.max_items < 0:
            parser.error("--max-items must be >= 0")
        _write_result(
            run_rom_forensics(
                args.rom,
                compare=args.compare,
                needles=args.needle,
                needle_file=args.needle_file,
                min_string=args.min_string,
                min_padding=args.min_padding,
                min_equal_run=args.min_equal_run,
                block_size=args.block_size,
                max_items=args.max_items,
            ),
            args.json,
        )
        return 0

    parser.error(f"unknown command {args.command!r}")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())