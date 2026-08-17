#!/usr/bin/env python3
"""Keep vgmspc as historical provenance only, never a live build input."""

from __future__ import annotations

from pathlib import Path
import re


REPO = Path(__file__).resolve().parents[1]
SCAN_ROOTS = (
    REPO / "tools",
    REPO / "patches",
    REPO / "components",
    REPO / ".github",
)

# Historical prose may name the retired source repository. Executable dependency
# shapes are forbidden: clone/fetch URLs, the old scaffold variable, or its old
# working-directory name.
FORBIDDEN = (
    re.compile(r"https://github\.com/dissonance-git/vgmspc(?:\.git)?", re.I),
    re.compile(r"git@github\.com:dissonance-git/vgmspc(?:\.git)?", re.I),
    re.compile(r"\bVgmSpcCommit\b"),
    re.compile(r"\bvgmspc-scaffold\b", re.I),
)


def main() -> int:
    failures: list[str] = []
    for root in SCAN_ROOTS:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {
                ".py", ".ps1", ".yml", ".yaml", ".cmake", ".txt", ".md"
            }:
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for pattern in FORBIDDEN:
                if pattern.search(text):
                    failures.append(f"{path.relative_to(REPO)} matches {pattern.pattern}")
    if failures:
        raise AssertionError("live vgmspc dependency survived:\n" + "\n".join(failures))
    print("no live vgmspc build dependency found")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
