#!/usr/bin/env python3
"""Require an exact committed source state before private binary construction.

The final bundle records one Retro VGM Compiler commit as provenance. Building
from modified tracked files would make that statement false even if every binary
compiled successfully. This preflight therefore requires a real 40-hex HEAD and
no staged or unstaged tracked changes. Untracked build/output directories are
intentionally ignored because the private builder creates them before CTest runs.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess


HEX40 = re.compile(r"^[0-9a-fA-F]{40}$")


def run_git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True,
        capture_output=True,
        check=False,
    )


def verify_repository(repo: Path) -> str:
    head = run_git(repo, "rev-parse", "HEAD")
    commit = head.stdout.strip()
    if head.returncode != 0 or not HEX40.fullmatch(commit):
        detail = (head.stderr or head.stdout).strip()
        raise AssertionError(
            "private build requires a Git checkout with an exact 40-hex HEAD"
            + (f": {detail}" if detail else "")
        )

    unstaged = run_git(repo, "diff", "--quiet", "--ignore-submodules=dirty", "--")
    if unstaged.returncode not in (0, 1):
        raise RuntimeError(
            "could not inspect unstaged tracked changes: "
            + (unstaged.stderr or unstaged.stdout).strip()
        )
    if unstaged.returncode == 1:
        raise AssertionError(
            "private build source has unstaged tracked modifications; commit or revert them first"
        )

    staged = run_git(repo, "diff", "--cached", "--quiet", "--ignore-submodules=dirty", "--")
    if staged.returncode not in (0, 1):
        raise RuntimeError(
            "could not inspect staged tracked changes: "
            + (staged.stderr or staged.stdout).strip()
        )
    if staged.returncode == 1:
        raise AssertionError(
            "private build source has staged modifications not represented by HEAD"
        )

    return commit.lower()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("repo", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    commit = verify_repository(args.repo.resolve())
    print(f"private build source provenance verified: {commit}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
