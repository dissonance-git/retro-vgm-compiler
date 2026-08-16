#!/usr/bin/env python3
"""Sonic 3 testbed wrapper for creator-blind structural grammar observations."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "tests" / "corpus" / "manifest.json"
TARGET_CORPUS_ID = "sonic-3-knuckles"
CONTROL_SELECTION_MARKER = "Sonic 3 attribution-control candidate"


def _load_structural_audit():
    path = Path(__file__).with_name("structural_grammar_audit.py")
    spec = importlib.util.spec_from_file_location("structural_grammar_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def selected_sonic3_corpus_ids(manifest_path: Path = MANIFEST_PATH) -> set[str]:
    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or not isinstance(payload.get("sets"), list):
        raise ValueError("corpus manifest must contain a sets array")

    selected: set[str] = set()
    found_target = False
    for raw in payload["sets"]:
        if not isinstance(raw, dict):
            raise ValueError("corpus manifest contains a non-object set")
        corpus_id = raw.get("corpus_id")
        if not isinstance(corpus_id, str):
            continue
        if corpus_id == TARGET_CORPUS_ID:
            selected.add(corpus_id)
            found_target = True
            continue
        reason = raw.get("selection_reason")
        if isinstance(reason, str) and CONTROL_SELECTION_MARKER.lower() in reason.lower():
            selected.add(corpus_id)

    if not found_target:
        raise ValueError(f"target corpus {TARGET_CORPUS_ID!r} missing from manifest")
    return selected


def run_sonic3_structural_audit(
    inputs: list[Path],
    manifest_path: Path = MANIFEST_PATH,
) -> dict[str, Any]:
    audit = _load_structural_audit()
    selected = selected_sonic3_corpus_ids(manifest_path)
    observations = audit.load_observations(inputs)

    foreign = sorted({
        str(observation["soundtrack_id"])
        for observation in observations
        if str(observation["soundtrack_id"]) not in selected
    })
    if foreign:
        raise ValueError(
            "structural observations contain soundtrack ids outside the predeclared "
            f"Sonic 3 target/control set: {foreign}"
        )

    result = audit.audit_observations(observations)
    result["testbed"] = "sonic3-primary-integration-testbed"
    result["target_corpus_id"] = TARGET_CORPUS_ID
    result["stage"] = "blind-structural-grammar-cross-soundtrack"
    result["testbed_corpus_ids"] = sorted(selected)
    result["testbed_firewall"] = (
        "Only predeclared Sonic 3 target/control corpus ids are admitted. Identity-bearing "
        "fields remain forbidden by structural_grammar_audit. Freeze this output before "
        "joining curated track or creator labels."
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+", type=Path)
    parser.add_argument("--manifest", type=Path, default=MANIFEST_PATH)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    result = run_sonic3_structural_audit(args.inputs, args.manifest)
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
