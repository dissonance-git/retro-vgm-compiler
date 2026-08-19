#!/usr/bin/env python3
"""Materialize the private foo_snesapu source tree without external migration input.

The historical x64 parent host comes from imports/foo_snesapu/parent. The
source-aware child comes from components/spc/spcplayer and shares the current
SRCE-v2 wire ABI from components/spc/snesapu_source_wire_v2.h. Current guarded
patches are then applied exactly once unless --no-patches is requested.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


REPO = Path(__file__).resolve().parents[1]
BOOTSTRAP = REPO / "imports" / "foo_snesapu" / "parent"
SPCPLAYER = REPO / "components" / "spc" / "spcplayer"
WIRE = REPO / "components" / "spc" / "snesapu_source_wire_v2.h"
PATCHER = REPO / "patches" / "snesapu" / "apply_private_component.py"

# Files consumed by guarded patchers must remain identical to the audited
# SRCE-v2 Git objects. Resolve the repository object itself rather than hashing
# checkout bytes, because Windows may perform a reversible CRLF worktree
# conversion even when the checked-out Git object is exact.
EXPECTED_BOOTSTRAP_BLOBS = {
    "input_snesapu.cpp": "e25b123bf71e64e14bfa89169d2f2d8caf8c38c9",
    "input_snesapu.hpp": "f1c3dad16bf6a0abd7eeccc13cde28ba472f70c7",
    "preferences_snesapu.cpp": "c2664d0eeba34eb3226dc141b5e20429194e9bfd",
    "resource.h": "37b84caec9c008e12aa6056c62d649872a222a8b",
    "resource.rc": "7f21b4b05423ad28992bab359edc2603c0e480e7",
    "spc_source_block.h": "1da3554a2498e30437cef394c724fb9a06f57163",
    "spcplayer_controller.cpp": "78c3e765faa893c7dff24665fe52a09fbf004c25",
    "spcplayer_controller.h": "5b65bcf43e6be92a6aedbf2929bfb5a712d69ce9",
    "foo_snesapu.vcxproj": "d64b1d482e6ad665ab5fe0edc68f52d95fd99a24",
}


def repository_blob_sha(relative: str) -> str:
    repo_path = (BOOTSTRAP / relative).relative_to(REPO).as_posix()
    completed = subprocess.run(
        ["git", "rev-parse", f"HEAD:{repo_path}"],
        cwd=REPO,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"could not resolve audited Git object for {repo_path}: "
            f"{completed.stderr.strip()}"
        )
    return completed.stdout.strip().lower()


def worktree_matches_head(path: Path) -> bool:
    relative = path.relative_to(REPO).as_posix()
    completed = subprocess.run(
        ["git", "diff", "--quiet", "HEAD", "--", relative],
        cwd=REPO,
        check=False,
    )
    return completed.returncode == 0


def verify_bootstrap() -> None:
    if not BOOTSTRAP.is_dir():
        raise RuntimeError(f"missing foo_snesapu bootstrap: {BOOTSTRAP}")
    for relative, expected in EXPECTED_BOOTSTRAP_BLOBS.items():
        path = BOOTSTRAP / relative
        if not path.is_file():
            raise RuntimeError(f"missing audited bootstrap file: {path}")
        actual = repository_blob_sha(relative)
        if actual != expected:
            raise RuntimeError(
                f"bootstrap Git-object drift for {relative}: expected {expected}, got {actual}"
            )
        if not worktree_matches_head(path):
            raise RuntimeError(f"bootstrap worktree differs from audited HEAD object: {path}")
    for required in (SPCPLAYER / "main.cpp", SPCPLAYER / "spcplayer.h", WIRE, PATCHER):
        if not required.is_file():
            raise RuntimeError(f"missing canonical SPC input: {required}")


def run_patcher(root: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(PATCHER), str(root)],
        cwd=str(root),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{PATCHER.name} failed with exit code {completed.returncode}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path, help="destination foo_snesapu workspace")
    parser.add_argument(
        "--no-patches",
        action="store_true",
        help="materialize the audited source geometry without applying current patches",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="replace an existing destination",
    )
    args = parser.parse_args()

    verify_bootstrap()
    output = args.output.resolve()
    if output.exists():
        if not args.force:
            raise RuntimeError(f"destination already exists: {output}; pass --force to replace it")
        shutil.rmtree(output)

    parent = output / "foobar2000" / "foo_snesapu"
    parent.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(BOOTSTRAP, parent)
    shutil.copytree(SPCPLAYER, output / "spcplayer")
    shutil.copy2(WIRE, output / "snesapu_source_wire_v2.h")

    if not args.no_patches:
        run_patcher(output)

    print(f"materialized foo_snesapu workspace: {output}")
    print("migration-source repository was not consulted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
