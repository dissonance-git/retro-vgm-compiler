#!/usr/bin/env python3
"""Compute creator-blind similarity matrices from cached song capsules only.

This tool never opens VGM/VGZ files.  It is the cheap research path after
`vgm_creator_feature_cache.py` has ingested each song once.
"""

from __future__ import annotations

import argparse
import json
import pathlib
from typing import Callable

import vgm_creator_feature_audit as gen1
import vgm_creator_part_matcher as gen2


ScoreFn = Callable[[dict[str, object], dict[str, object]], float]


def _capsule_paths(inputs: list[str]) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for raw in inputs:
        path = pathlib.Path(raw)
        if path.is_dir():
            found.extend(
                candidate
                for candidate in path.rglob("*.json")
                if candidate.name != "manifest.json"
            )
        elif path.is_file() and path.suffix.lower() == ".json":
            if path.name != "manifest.json":
                found.append(path)
        else:
            raise SystemExit(f"not a feature-cache JSON file or directory: {path}")
    return sorted(dict.fromkeys(found), key=lambda item: str(item).lower())


def _load(path: pathlib.Path) -> dict[str, object]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("model") != "creator-blind Genesis VGM reusable feature capsule":
        raise ValueError(f"not a creator feature capsule: {path}")
    return payload


def _track_id(capsule: dict[str, object]) -> str:
    source = capsule["source"]
    assert isinstance(source, dict)
    return f"{source['soundtrack_id']}::{source['file']}"


def _view(capsule: dict[str, object], name: str) -> dict[str, object]:
    views = capsule["views"]
    assert isinstance(views, dict)
    value = views[name]
    assert isinstance(value, dict)
    return value


def _score_fn(view: str) -> ScoreFn:
    if view == "gen1-structural":
        return lambda lhs, rhs: gen1.structural_similarity(
            _view(lhs, "gen1"), _view(rhs, "gen1")
        )
    if view == "gen1-realization":
        return lambda lhs, rhs: gen1.realization_similarity(
            _view(lhs, "gen1"), _view(rhs, "gen1")
        )
    if view == "gen2-parts":
        return lambda lhs, rhs: float(
            gen2.track_similarity(_view(lhs, "gen2_parts"), _view(rhs, "gen2_parts"))
        )
    if view == "gen3-motion-parts":
        return lambda lhs, rhs: float(
            gen2.track_similarity(
                _view(lhs, "gen3_motion_parts"),
                _view(rhs, "gen3_motion_parts"),
            )
        )
    raise ValueError(f"unsupported view: {view}")


def build_matrix(capsules: list[dict[str, object]], view: str) -> dict[str, object]:
    scorer = _score_fn(view)
    ids = [_track_id(capsule) for capsule in capsules]
    matrix = [
        [1.0 if i == j else scorer(left, right) for j, right in enumerate(capsules)]
        for i, left in enumerate(capsules)
    ]
    return {
        "schema_version": 1,
        "model": "creator-blind cached VGM similarity matrix",
        "view": view,
        "track_count": len(ids),
        "track_ids": ids,
        "similarity_matrix": matrix,
        "label_policy": "No composer/artist labels are read or inferred by this tool.",
        "source_policy": "All scores were computed from cached JSON capsules; no VGM/VGZ was opened.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="Feature-cache JSON files/directories")
    parser.add_argument(
        "--view",
        choices=(
            "gen1-structural",
            "gen1-realization",
            "gen2-parts",
            "gen3-motion-parts",
        ),
        default="gen3-motion-parts",
    )
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    paths = _capsule_paths(args.inputs)
    if not paths:
        raise SystemExit("no feature capsules found")
    payload = build_matrix([_load(path) for path in paths], args.view)
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
