#!/usr/bin/env python3
"""Verify the local libvgm patch series against its pinned upstream commit."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PATCH_DIR = ROOT / "patches" / "libvgm"
PINNED_COMMIT = "61fc6725644886abc3168e240e4e51588d74bdf7"


def run(*args: str, cwd: Path, capture: bool = False) -> str:
    result = subprocess.run(
        list(args),
        cwd=cwd,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )
    return result.stdout.strip() if capture else ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm", type=Path, help="path to a libvgm git checkout")
    parser.add_argument("--keep", action="store_true", help="keep temporary verification worktree")
    args = parser.parse_args()

    source = args.libvgm.resolve()
    if not (source / ".git").exists():
        print(f"not a git checkout: {source}", file=sys.stderr)
        return 2

    patches = sorted(PATCH_DIR.glob("*.patch"))
    if not patches:
        print("no libvgm patches found", file=sys.stderr)
        return 2

    try:
        run("git", "cat-file", "-e", f"{PINNED_COMMIT}^{{commit}}", cwd=source)
    except subprocess.CalledProcessError:
        print(f"pinned libvgm commit is missing: {PINNED_COMMIT}", file=sys.stderr)
        return 2

    temp_root = Path(tempfile.mkdtemp(prefix="gameaudio-libvgm-patches-"))
    worktree = temp_root / "libvgm"
    try:
        run("git", "worktree", "add", "--detach", str(worktree), PINNED_COMMIT, cwd=source)

        for patch in patches:
            print(f"[check] {patch.name}")
            run("git", "apply", "--check", str(patch), cwd=worktree)
            run("git", "apply", str(patch), cwd=worktree)

        status = run("git", "status", "--short", cwd=worktree, capture=True)
        if not status:
            print("patches produced no changes", file=sys.stderr)
            return 1

        print(f"\nAll {len(patches)} patches apply cleanly to {PINNED_COMMIT}.")
        return 0
    except subprocess.CalledProcessError as exc:
        print(f"patch verification failed with exit {exc.returncode}", file=sys.stderr)
        return 1
    finally:
        try:
            if worktree.exists():
                run("git", "worktree", "remove", "--force", str(worktree), cwd=source)
        except subprocess.CalledProcessError:
            pass
        if args.keep:
            print(f"kept temporary directory: {temp_root}")
        else:
            shutil.rmtree(temp_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
