#!/usr/bin/env python3
"""Capture and freeze an opaque SPC panel without creator labels entering extraction.

The panel contract intentionally permits only opaque cue ids and fixture paths for
individual cues. Creator, candidate, role, soundtrack-label, and attribution data
belong to the later reveal/evaluation stage.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass


CUE_ID_RE = re.compile(r"^cue-\d{3}$")
FORBIDDEN_CUE_KEYS = {
    "artist",
    "composer",
    "candidate",
    "role",
    "attribution",
    "expected_candidate",
    "external_artist",
    "gd3_artist",
    "id666_artist",
}


@dataclass(frozen=True)
class PanelCue:
    cue_id: str
    fixture_path: pathlib.PurePosixPath


def load_panel(path: pathlib.Path) -> list[PanelCue]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("blind panel must be a JSON object")
    cues = value.get("cues")
    if not isinstance(cues, list) or len(cues) < 2:
        raise ValueError("blind panel requires at least two cues")

    result: list[PanelCue] = []
    seen_ids: set[str] = set()
    seen_paths: set[str] = set()
    for entry in cues:
        if not isinstance(entry, dict):
            raise ValueError("blind panel cue must be a JSON object")
        lowered = {str(key).lower() for key in entry}
        leaked = lowered & FORBIDDEN_CUE_KEYS
        if leaked:
            raise ValueError(f"creator-bearing cue key forbidden in blind panel: {sorted(leaked)[0]}")
        if set(entry) != {"cue_id", "fixture_path"}:
            raise ValueError("blind panel cue entries may contain only cue_id and fixture_path")

        cue_id = entry.get("cue_id")
        fixture = entry.get("fixture_path")
        if not isinstance(cue_id, str) or CUE_ID_RE.fullmatch(cue_id) is None:
            raise ValueError("cue_id must match cue-NNN")
        if not isinstance(fixture, str) or not fixture:
            raise ValueError("fixture_path must be a non-empty string")
        pure = pathlib.PurePosixPath(fixture)
        if pure.is_absolute() or ".." in pure.parts:
            raise ValueError("fixture_path must remain repository-relative")
        if pure.suffix.lower() != ".spc":
            raise ValueError("blind SPC panel accepts only .spc fixtures")
        if tuple(pure.parts[:2]) != ("tests", "corpus"):
            raise ValueError("fixture_path must live below tests/corpus")
        if cue_id in seen_ids:
            raise ValueError(f"duplicate cue id: {cue_id}")
        if fixture in seen_paths:
            raise ValueError(f"duplicate fixture path: {fixture}")
        seen_ids.add(cue_id)
        seen_paths.add(fixture)
        result.append(PanelCue(cue_id=cue_id, fixture_path=pure))

    return result


def capture_panel(
    cues: list[PanelCue],
    *,
    repo_root: pathlib.Path,
    extractor: pathlib.Path,
    output_dir: pathlib.Path,
    seconds: int,
    freeze_tool: pathlib.Path,
    freeze_output: pathlib.Path,
) -> None:
    if seconds <= 0:
        raise ValueError("seconds must be positive")
    extractor = extractor.resolve()
    freeze_tool = freeze_tool.resolve()
    if not extractor.is_file():
        raise FileNotFoundError(f"SPC forensic extractor not found: {extractor}")
    if not freeze_tool.is_file():
        raise FileNotFoundError(f"freeze tool not found: {freeze_tool}")

    output_dir.mkdir(parents=True, exist_ok=True)
    freeze_output.parent.mkdir(parents=True, exist_ok=True)
    cue_args: list[str] = []
    for cue in cues:
        fixture = (repo_root / pathlib.Path(*cue.fixture_path.parts)).resolve()
        try:
            fixture.relative_to(repo_root.resolve())
        except ValueError as exc:
            raise ValueError("fixture escaped repository root") from exc
        if not fixture.is_file():
            raise FileNotFoundError(f"SPC fixture not found: {cue.fixture_path}")

        sidecar = (output_dir / f"{cue.cue_id}.json").resolve()
        subprocess.run(
            [str(extractor), str(fixture), str(sidecar), str(seconds)],
            cwd=repo_root,
            check=True,
        )
        cue_args.extend(["--cue", f"{cue.cue_id}={sidecar}"])

    subprocess.run(
        [sys.executable, str(freeze_tool), *cue_args, "--output", str(freeze_output.resolve())],
        cwd=repo_root,
        check=True,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--panel", type=pathlib.Path, required=True)
    parser.add_argument("--extractor", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", type=pathlib.Path, required=True)
    parser.add_argument("--freeze-output", type=pathlib.Path, required=True)
    parser.add_argument("--seconds", type=int, default=5)
    parser.add_argument("--repo-root", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument(
        "--freeze-tool",
        type=pathlib.Path,
        default=pathlib.Path("tools/spc/freeze_forensic_sidecars.py"),
    )
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    panel_path = args.panel if args.panel.is_absolute() else repo_root / args.panel
    extractor = args.extractor if args.extractor.is_absolute() else repo_root / args.extractor
    output_dir = args.output_dir if args.output_dir.is_absolute() else repo_root / args.output_dir
    freeze_output = args.freeze_output if args.freeze_output.is_absolute() else repo_root / args.freeze_output
    freeze_tool = args.freeze_tool if args.freeze_tool.is_absolute() else repo_root / args.freeze_tool

    cues = load_panel(panel_path)
    capture_panel(
        cues,
        repo_root=repo_root,
        extractor=extractor,
        output_dir=output_dir,
        seconds=args.seconds,
        freeze_tool=freeze_tool,
        freeze_output=freeze_output,
    )


if __name__ == "__main__":
    main()
