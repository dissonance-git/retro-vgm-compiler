#!/usr/bin/env python3
"""Capture and freeze an opaque mixed-source motif panel without creator labels.

The panel contract intentionally permits only opaque cue ids and fixture paths for
individual cues. Creator, candidate, role, soundtrack-label, and attribution data
belong to the later reveal/evaluation stage.

Expensive source-specific extraction is song-centered and persistent. Panel-specific
cue ids are attached only after a creator-blind cached object has been produced or
reused. SPC fixtures route through controlled forensic execution. Genesis VGM/VGZ
fixtures route through the existing creator-blind song cache and conservative
persistent-part motif projection. Both meet only at the representation-neutral
freezer.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass

THIS_DIR = pathlib.Path(__file__).resolve().parent
TOOLS_DIR = THIS_DIR.parent
for module_dir in (THIS_DIR, TOOLS_DIR):
    if str(module_dir) not in sys.path:
        sys.path.insert(0, str(module_dir))

import creator_blind_spc_cache as spc_cache
import creator_blind_song_cache as genesis_cache
import genesis_cached_part_evidence as genesis_parts


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
SPC_SUFFIXES = {".spc"}
GENESIS_SUFFIXES = {".vgm", ".vgz"}
SUPPORTED_SUFFIXES = SPC_SUFFIXES | GENESIS_SUFFIXES


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
        if pure.suffix.lower() not in SUPPORTED_SUFFIXES:
            raise ValueError("blind motif panel accepts only .spc, .vgm, or .vgz fixtures")
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
    extractor: pathlib.Path | None,
    output_dir: pathlib.Path,
    seconds: int,
    freeze_tool: pathlib.Path,
    freeze_output: pathlib.Path,
    cache_root: pathlib.Path = spc_cache.DEFAULT_CACHE_ROOT,
    genesis_cache_root: pathlib.Path = genesis_cache.DEFAULT_CACHE_ROOT,
    genesis_max_gap_ticks: int | None = None,
    genesis_max_pitch_interval_octaves: float | None = None,
    genesis_strand_min_confidence: float = genesis_parts.DEFAULT_STRAND_MIN_CONFIDENCE,
    refresh_cache: bool = False,
) -> None:
    if seconds <= 0:
        raise ValueError("seconds must be positive")
    repo_root = repo_root.resolve()
    freeze_tool = freeze_tool.resolve()
    cache_root = cache_root if cache_root.is_absolute() else repo_root / cache_root
    genesis_cache_root = (
        genesis_cache_root if genesis_cache_root.is_absolute()
        else repo_root / genesis_cache_root
    )
    if not freeze_tool.is_file():
        raise FileNotFoundError(f"freeze tool not found: {freeze_tool}")

    has_spc = any(cue.fixture_path.suffix.lower() in SPC_SUFFIXES for cue in cues)
    has_genesis = any(cue.fixture_path.suffix.lower() in GENESIS_SUFFIXES for cue in cues)
    resolved_extractor: pathlib.Path | None = None
    if has_spc:
        if extractor is None:
            raise ValueError("SPC cues require --extractor")
        resolved_extractor = extractor.resolve()
        if not resolved_extractor.is_file():
            raise FileNotFoundError(f"SPC forensic extractor not found: {resolved_extractor}")
    if has_genesis:
        if genesis_max_gap_ticks is None or genesis_max_gap_ticks < 0:
            raise ValueError("Genesis cues require nonnegative --genesis-max-gap-ticks")
        if (
            genesis_max_pitch_interval_octaves is None
            or genesis_max_pitch_interval_octaves < 0.0
        ):
            raise ValueError(
                "Genesis cues require nonnegative --genesis-max-pitch-interval-octaves"
            )

    output_dir.mkdir(parents=True, exist_ok=True)
    freeze_output.parent.mkdir(parents=True, exist_ok=True)
    spc_args: list[str] = []
    profile_args: list[str] = []
    for cue in cues:
        fixture = (repo_root / pathlib.Path(*cue.fixture_path.parts)).resolve()
        try:
            fixture.relative_to(repo_root)
        except ValueError as exc:
            raise ValueError("fixture escaped repository root") from exc
        if not fixture.is_file():
            raise FileNotFoundError(f"fixture not found: {cue.fixture_path}")

        corpus_id = cue.fixture_path.parts[2]
        suffix = cue.fixture_path.suffix.lower()
        sidecar = (output_dir / f"{cue.cue_id}.json").resolve()
        if suffix in SPC_SUFFIXES:
            assert resolved_extractor is not None
            cached_sidecar, _changed = spc_cache.build_one(
                fixture,
                corpus_id=corpus_id,
                extractor=resolved_extractor,
                cache_root=cache_root,
                seconds=seconds,
                refresh=refresh_cache,
            )
            if sidecar != cached_sidecar.resolve():
                shutil.copyfile(cached_sidecar, sidecar)
            spc_args.extend(["--cue", f"{cue.cue_id}={sidecar}"])
            continue

        if suffix in GENESIS_SUFFIXES:
            assert genesis_max_gap_ticks is not None
            assert genesis_max_pitch_interval_octaves is not None
            _cache_path, _changed, capsule = genesis_cache.build_one(
                fixture,
                corpus_id=corpus_id,
                cache_root=genesis_cache_root,
                refresh=refresh_cache,
            )
            projection = genesis_parts.project(
                capsule,
                max_gap_ticks=genesis_max_gap_ticks,
                max_pitch_interval_octaves=genesis_max_pitch_interval_octaves,
                strand_min_confidence=genesis_strand_min_confidence,
            )
            bundle = genesis_parts.make_motif_profile_bundle(projection)
            sidecar.write_text(
                json.dumps(bundle, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            profile_args.extend(["--profile-bundle", f"{cue.cue_id}={sidecar}"])
            continue

        raise AssertionError(f"unreachable panel suffix: {suffix}")

    subprocess.run(
        [
            sys.executable,
            str(freeze_tool),
            *spc_args,
            *profile_args,
            "--output",
            str(freeze_output.resolve()),
        ],
        cwd=repo_root,
        check=True,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--panel", type=pathlib.Path, required=True)
    parser.add_argument(
        "--extractor",
        type=pathlib.Path,
        help="SPC forensic extractor; required only when the panel contains SPC cues.",
    )
    parser.add_argument("--output-dir", type=pathlib.Path, required=True)
    parser.add_argument("--freeze-output", type=pathlib.Path, required=True)
    parser.add_argument("--seconds", type=int, default=5)
    parser.add_argument("--repo-root", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument(
        "--cache-root",
        type=pathlib.Path,
        default=spc_cache.DEFAULT_CACHE_ROOT,
        help="Persistent creator-blind SPC sidecar cache.",
    )
    parser.add_argument(
        "--genesis-cache-root",
        type=pathlib.Path,
        default=genesis_cache.DEFAULT_CACHE_ROOT,
        help="Persistent creator-blind Genesis song capsule cache.",
    )
    parser.add_argument(
        "--genesis-max-gap-ticks",
        type=int,
        help="Explicit Genesis continuity window; required when VGM/VGZ cues are present.",
    )
    parser.add_argument(
        "--genesis-max-pitch-interval-octaves",
        type=float,
        help="Explicit Genesis pitch-continuity bound; required when VGM/VGZ cues are present.",
    )
    parser.add_argument(
        "--genesis-strand-min-confidence",
        type=float,
        default=genesis_parts.DEFAULT_STRAND_MIN_CONFIDENCE,
        help="Conservative persistent-part strand threshold for Genesis motif projection.",
    )
    parser.add_argument(
        "--refresh-cache",
        action="store_true",
        help="Force source-specific extraction even when a compatible song cache exists.",
    )
    parser.add_argument(
        "--freeze-tool",
        type=pathlib.Path,
        default=pathlib.Path("tools/spc/freeze_forensic_sidecars.py"),
    )
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    panel_path = args.panel if args.panel.is_absolute() else repo_root / args.panel
    extractor = None
    if args.extractor is not None:
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
        cache_root=args.cache_root,
        genesis_cache_root=args.genesis_cache_root,
        genesis_max_gap_ticks=args.genesis_max_gap_ticks,
        genesis_max_pitch_interval_octaves=args.genesis_max_pitch_interval_octaves,
        genesis_strand_min_confidence=args.genesis_strand_min_confidence,
        refresh_cache=args.refresh_cache,
    )


if __name__ == "__main__":
    main()
