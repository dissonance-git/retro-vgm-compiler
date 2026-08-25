#!/usr/bin/env python3
"""Require an exact committed source state for private binary construction.

The final bundle records one VGM Compiler commit as provenance. Building
from modified tracked files would make that statement false even if every binary
compiled successfully. The verifier therefore requires a real 40-hex HEAD and
no staged or unstaged tracked changes. An optional expected commit lets the
builder prove that the checkout stayed on the same source snapshot for the whole
build. Untracked build/output directories are intentionally ignored.
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


def verify_repository(repo: Path, expected_commit: str | None = None) -> str:
    head = run_git(repo, "rev-parse", "HEAD")
    commit = head.stdout.strip()
    if head.returncode != 0 or not HEX40.fullmatch(commit):
        detail = (head.stderr or head.stdout).strip()
        raise AssertionError(
            "private build requires a Git checkout with an exact 40-hex HEAD"
            + (f": {detail}" if detail else "")
        )
    commit = commit.lower()

    if expected_commit is not None:
        expected = expected_commit.strip().lower()
        if not HEX40.fullmatch(expected):
            raise AssertionError(
                f"expected source commit must be exactly 40 hexadecimal characters: {expected_commit!r}"
            )
        if commit != expected:
            raise AssertionError(
                f"private build source commit changed during build: started {expected}, now {commit}"
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

    return commit


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("repo", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument(
        "--expected-commit",
        default=None,
        help="require HEAD to still equal this 40-hex source commit",
    )
    args = parser.parse_args()

    commit = verify_repository(args.repo.resolve(), args.expected_commit)
    print(f"private build source provenance verified: {commit}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
