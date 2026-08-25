#!/usr/bin/env python3
"""Verify the current VGM Compiler identity and canonical owner surface."""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_TEXT = {
    Path("README.md"): (
        "# VGM Compiler\n",
        "docs/architecture.md",
        "docs/vgm-compiler-roadmap.md",
    ),
    Path("AGENTS.md"): (
        "# AGENTS.md\n",
        "docs/architecture.md",
        "docs/vgm-compiler-roadmap.md",
        "dense current/future contracts",
    ),
    Path("docs/architecture.md"): (
        "# VGM Compiler architecture\n",
        "This is the canonical contract for VGM Compiler's shared semantics",
        "## Objective\n",
    ),
    Path("docs/vgm-compiler-roadmap.md"): (
        "# VGM Compiler roadmap\n",
        "## Active frontier: positive phrase-role evidence",
    ),
    Path("docs/musical-understanding.md"): (
        "# Holistic musical understanding\n",
        "canonical north-star contract",
        "VGM Compiler should understand a cue and a soundtrack as coherent music",
    ),
}


def verify(root: Path = ROOT) -> list[str]:
    failures: list[str] = []
    for relative, markers in REQUIRED_TEXT.items():
        path = root / relative
        if not path.is_file():
            failures.append(f"missing canonical owner: {relative.as_posix()}")
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except OSError as exc:
            failures.append(f"cannot read {relative.as_posix()}: {exc}")
            continue
        for marker in markers:
            if marker not in text:
                failures.append(
                    f"{relative.as_posix()} missing current identity marker: {marker!r}"
                )
    return failures


def main() -> int:
    failures = verify()
    if not failures:
        print("project identity check passed: VGM Compiler")
        return 0

    print("VGM Compiler identity contract failed:", file=sys.stderr)
    for failure in failures:
        print(f"  {failure}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
