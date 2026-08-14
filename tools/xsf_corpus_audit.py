#!/usr/bin/env python3
"""Validate xSF containers, dependency closure, and platform-specific effective objects."""

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from components.psf.psf1 import build_psf1_effective_image
from components.twosf.twosf import build_twosf_effective_state
from components.usf.usf import build_usf_effective_state
from components.xsf.envelope import parse_xsf, resolve_xsf


SUFFIX_VERSION = {
    ".psf": 0x01,
    ".minipsf": 0x01,
    ".psflib": 0x01,
    ".psf1": 0x01,
    ".minipsf1": 0x01,
    ".psf1lib": 0x01,
    ".usf": 0x21,
    ".miniusf": 0x21,
    ".usflib": 0x21,
    ".2sf": 0x24,
    ".mini2sf": 0x24,
    ".2sflib": 0x24,
}
LIBRARY_SUFFIXES = {".psflib", ".psf1lib", ".usflib", ".2sflib"}


def audit_directory(directory: Path) -> dict:
    directory = directory.resolve()
    paths = sorted(
        (path for path in directory.rglob("*") if path.is_file() and path.suffix.lower() in SUFFIX_VERSION),
        key=lambda path: path.relative_to(directory).as_posix().casefold(),
    )
    if not paths:
        raise ValueError(f"no xSF objects under {directory}")
    versions: Counter[str] = Counter()
    tag_keys: Counter[str] = Counter()
    for path in paths:
        expected = SUFFIX_VERSION[path.suffix.lower()]
        obj = parse_xsf(path.read_bytes(), source_id=path.name, expected_version=expected)
        versions[f"0x{obj.version:02X}"] += 1
        tag_keys.update(tag.name for tag in obj.tags)

    roots = [path for path in paths if path.suffix.lower() not in LIBRARY_SUFFIXES]
    effective = []
    dependency_edges = 0
    for path in roots:
        expected = SUFFIX_VERSION[path.suffix.lower()]
        resolved = resolve_xsf(path, expected_version=expected)
        dependency_edges += len(resolved.edges)
        if expected == 0x01:
            state = build_psf1_effective_image(resolved)
            detail = {
                "kind": "ps-x-exe-memory",
                "bytes": len(state.memory),
                "memory_base": state.memory_base,
            }
        elif expected == 0x21:
            state = build_usf_effective_state(resolved)
            detail = {"kind": "n64-rom-project64-state", "rom_bytes": len(state.rom), "save_bytes": len(state.save_state)}
        elif expected == 0x24:
            state = build_twosf_effective_state(resolved)
            detail = {"kind": "nds-rom-save-map", "rom_bytes": len(state.rom), "save_bytes": len(state.save_state)}
        else:
            raise AssertionError(expected)
        if state.runtime_available:
            raise AssertionError("audit must not report an unimplemented machine runtime")
        effective.append({"root": path.relative_to(directory).as_posix(), **detail, "runtime_available": False})
    return {
        "directory": str(directory),
        "object_count": len(paths),
        "root_count": len(roots),
        "library_count": len(paths) - len(roots),
        "versions": dict(sorted(versions.items())),
        "dependency_edge_count": dependency_edges,
        "tag_keys": dict(sorted(tag_keys.items(), key=lambda item: item[0].casefold())),
        "effective_objects": effective,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directories", nargs="+", type=Path)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    reports = [audit_directory(directory) for directory in args.directories]
    if not args.quiet:
        print(json.dumps({"schema": "xsf-corpus-audit-1", "reports": reports}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
