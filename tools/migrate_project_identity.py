#!/usr/bin/env python3
"""Apply the bounded live-surface rename to VGM Compiler.

This is a migration helper, not a historical rewriter. It changes exact retired
project labels and the retired live roadmap path in active repository text while
leaving docs/history, imported upstreams, and reference material untouched.
"""

from __future__ import annotations

import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPLACEMENTS = (
    (("Retro" + " VGM" + " Compiler").encode(), b"VGM Compiler"),
    (("Game" + " Music" + " Interpreter").encode(), b"VGM Compiler"),
    (("retro-vgm" + "-compiler-roadmap.md").encode(), b"vgm-compiler-roadmap.md"),
)
TEXT_SUFFIXES = {
    ".md", ".txt", ".rst", ".py", ".h", ".hpp", ".c", ".cc", ".cpp",
    ".json", ".jsonl", ".toml", ".yml", ".yaml", ".cmake", ".ps1", ".sh",
}
TEXT_NAMES = {"CMakeLists.txt", "LICENSE", "Makefile"}
SKIP_DIRS = {".git", "imports", "references"}
HISTORICAL_PREFIX = (Path("docs") / "history").parts


def candidate(path: Path) -> bool:
    return path.name in TEXT_NAMES or path.suffix.lower() in TEXT_SUFFIXES


def excluded(relative: Path) -> bool:
    if relative.parts[: len(HISTORICAL_PREFIX)] == HISTORICAL_PREFIX:
        return True
    return any(part in SKIP_DIRS for part in relative.parts[:-1])


def migrate(root: Path = ROOT, *, apply: bool = False) -> list[Path]:
    changed: list[Path] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or not candidate(path):
            continue
        relative = path.relative_to(root)
        if excluded(relative):
            continue
        try:
            original = path.read_bytes()
        except OSError:
            continue
        migrated = original
        for old, new in REPLACEMENTS:
            migrated = migrated.replace(old, new)
        if migrated == original:
            continue
        changed.append(relative)
        if apply:
            path.write_bytes(migrated)
    return changed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--apply",
        action="store_true",
        help="write the bounded rename; otherwise only report affected files",
    )
    args = parser.parse_args()

    changed = migrate(apply=args.apply)
    action = "updated" if args.apply else "would update"
    print(f"{action} {len(changed)} active file(s)")
    for path in changed:
        print(path.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
