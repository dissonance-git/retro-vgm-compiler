#!/usr/bin/env python3
"""Reject reintroduction of deprecated live project identities.

VGM Compiler is the current project identity. Historical prose under docs/history/
may retain older names when doing so preserves lineage; active repository text may
not silently reintroduce them.
"""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DEPRECATED = (
    "Game" + " Music" + " Interpreter",
    "Retro" + " VGM" + " Compiler",
    "retro-vgm" + "-compiler-roadmap.md",
)
TEXT_SUFFIXES = {
    ".md", ".txt", ".rst", ".py", ".h", ".hpp", ".c", ".cc", ".cpp",
    ".json", ".jsonl", ".toml", ".yml", ".yaml", ".cmake", ".ps1", ".sh",
}
TEXT_NAMES = {"CMakeLists.txt", "LICENSE", "Makefile"}
SKIP_DIRS = {".git", "imports", "references"}
HISTORICAL_PREFIXES = {(Path("docs") / "history").parts}


def candidate(path: Path) -> bool:
    return path.name in TEXT_NAMES or path.suffix.lower() in TEXT_SUFFIXES


def historical(path: Path) -> bool:
    parts = path.parts
    return any(parts[: len(prefix)] == prefix for prefix in HISTORICAL_PREFIXES)


def scan_repository(root: Path = ROOT) -> list[tuple[Path, int, str]]:
    findings: list[tuple[Path, int, str]] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or not candidate(path):
            continue
        relative = path.relative_to(root)
        if any(part in SKIP_DIRS for part in relative.parts[:-1]) or historical(relative):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        for line_number, line in enumerate(text.splitlines(), start=1):
            if any(name in line for name in DEPRECATED):
                findings.append((relative, line_number, line.strip()))
    return findings


def main() -> int:
    findings = scan_repository()
    if not findings:
        print("project identity check passed: VGM Compiler")
        return 0

    print(f"deprecated project identity found in {len(findings)} location(s):", file=sys.stderr)
    for path, line_number, line in findings:
        print(f"  {path}:{line_number}: {line}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
