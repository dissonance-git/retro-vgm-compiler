#!/usr/bin/env python3
"""Verify VGM Compiler's current canonical owner surface."""

from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]

# Pin semantic ownership, not historical heading wording. These markers should
# change only when the corresponding canonical responsibility changes.
REQUIRED_TEXT = {
    Path("README.md"): (
        "# VGM Compiler\n",
        "## Repository landmarks\n",
        "docs/architecture.md",
        "docs/musical-understanding.md",
        "docs/vgm-compiler-roadmap.md",
        "tools/github_agent_bootstrap.py",
    ),
    Path("AGENTS.md"): (
        "# VGM Compiler development contract\n",
        "## 1. First-connection skill preflight\n",
        "## 3. One owner per concept\n",
        "## 4. Semantic collapse and cleanup\n",
        "## 8. Repository mutation and concurrency\n",
        "tools/github_agent_bootstrap.py",
        "docs/architecture.md",
        "docs/vgm-compiler-roadmap.md",
    ),
    Path("docs/architecture.md"): (
        "# VGM Compiler architecture\n",
        "owns the shared semantic, provenance, evidence, identity, time, and abstraction laws",
        "## 1. Linked representations\n",
        "## 12. Architectural test\n",
    ),
    Path("docs/musical-understanding.md"): (
        "# Holistic musical understanding\n",
        "canonical north-star contract",
        "VGM Compiler should understand a cue and a soundtrack as coherent music",
    ),
    Path("research/validation/music-representation-systems.md"): (
        "# Music representation systems observatory\n",
        "## Linked representations\n",
        "openmusic-libraries.md",
    ),
    Path("docs/vgm-compiler-roadmap.md"): (
        "# VGM Compiler roadmap\n",
        "## Active frontier: real-corpus pressure for phrase syntax",
    ),
    Path("research/README.md"): (
        "# Research\n",
        "## Evidence, policy, and derived state\n",
    ),
    Path("tests/README.md"): (
        "# Tests\n",
        "under the corpus owner",
        "## Test law\n",
    ),
    Path("tools/github_agent_bootstrap.py"): (
        'SCHEMA_VERSION = "vgm-github-agent-bootstrap-001.0"',
        '"skill_preflight_path": ".agents/skills/skill-preflight/SKILL.md"',
        '"freeze-github-head"',
        '"run-skill-preflight"',
    ),
}

FORBIDDEN_PATHS = (
    Path("tests/CORPUS.md"),
    Path("docs/audio-programming-languages.md"),
    Path("docs/openmusic-libraries.md"),
    Path(".agents/github-connector.json"),
)

FORBIDDEN_TEXT = {
    Path("research/validation/music-representation-systems.md"): (
        "docs/composer-level-understanding.md",
        "research/cases/human-musical-discourse.md",
        "docs/openmusic-libraries.md",
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
                    f"{relative.as_posix()} missing current owner marker: {marker!r}"
                )

    for relative in FORBIDDEN_PATHS:
        if (root / relative).exists():
            failures.append(f"superseded owner still present: {relative.as_posix()}")

    for relative, markers in FORBIDDEN_TEXT.items():
        path = root / relative
        if not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            continue
        for marker in markers:
            if marker in text:
                failures.append(
                    f"{relative.as_posix()} retains superseded route: {marker!r}"
                )

    return failures


def main() -> int:
    failures = verify()
    if not failures:
        print("project identity check passed: VGM Compiler canonical owners")
        return 0

    print("VGM Compiler owner contract failed:", file=sys.stderr)
    for failure in failures:
        print(f"  {failure}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
