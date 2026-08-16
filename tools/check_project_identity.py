#!/usr/bin/env python3
"""Reject reintroduction of deprecated human-facing project names.

The GitHub repository slug may temporarily remain `game-music-interpreter` while
project identity is Retro VGM Compiler. This check targets text content, not the
repository path or historical git metadata.
"""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DEPRECATED = "Game" + " Music" + " Interpreter"
TEXT_SUFFIXES = {
    ".md", ".txt", ".rst", ".py", ".h", ".hpp", ".c", ".cc", ".cpp",
    ".json", ".jsonl", ".toml", ".yml", ".yaml", ".cmake", ".ps1", ".sh",
}
TEXT_NAMES = {"CMakeLists.txt", "LICENSE", "Makefile"}
SKIP_DIRS = {".git", "imports", "references"}


def candidate(path: Path) -> bool:
    return path.name in TEXT_NAMES or path.suffix.lower() in TEXT_SUFFIXES


def scan_repository(root: Path = ROOT) -> list[tuple[Path, int, str]]:
    findings: list[tuple[Path, int, str]] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or not candidate(path):
            continue
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts[:-1]):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        for line_number, line in enumerate(text.splitlines(), start=1):
            if DEPRECATED in line:
                findings.append((path.relative_to(root), line_number, line.strip()))
    return findings


def main() -> int:
    findings = scan_repository()
    if not findings:
        print("project identity check passed: Retro VGM Compiler")
        return 0

    print(f"deprecated project identity found in {len(findings)} location(s):", file=sys.stderr)
    for path, line_number, line in findings:
        print(f"  {path}:{line_number}: {line}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
