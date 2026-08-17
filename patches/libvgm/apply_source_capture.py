#!/usr/bin/env python3
"""Apply guarded libvgm changes required by source-aware VGM rendering."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, libvgm_root: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(libvgm_root)],
        cwd=str(libvgm_root),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{script.name} failed with exit code {completed.returncode}")


def apply_numbered_patch(patch: Path, libvgm_root: Path) -> None:
    # Numbered observer patches are pinned source contracts. Be idempotent for a
    # checkout that already contains them, but never fuzz/guess around upstream
    # drift: either the patch applies exactly or its exact reverse does.
    check = subprocess.run(
        ["git", "apply", "--check", str(patch)],
        cwd=str(libvgm_root),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if check.returncode == 0:
        applied = subprocess.run(
            ["git", "apply", str(patch)],
            cwd=str(libvgm_root),
            check=False,
        )
        if applied.returncode != 0:
            raise RuntimeError(f"{patch.name} passed --check but failed to apply")
        return

    reverse = subprocess.run(
        ["git", "apply", "--reverse", "--check", str(patch)],
        cwd=str(libvgm_root),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if reverse.returncode == 0:
        return

    raise RuntimeError(
        f"{patch.name} neither applies cleanly nor matches an already-applied tree: "
        f"{check.stderr.strip()}"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    root = args.libvgm_root.resolve()
    here = Path(__file__).resolve().parent

    # These observational ABIs are also prerequisites for source-native DAC
    # enhancement. In particular, 0003 exposes original PCM-bank identity and
    # authored stream frequency instead of forcing inference from resolved $2A
    # writes. 0004/0005 preserve refreshed-bank and length semantics.
    for name in (
        "0001-realtime-command-observer.patch",
        "0002-resolved-ym2612-dac-observer.patch",
        "0003-dac-stream-source-observer.patch",
        "0004-refresh-dac-stream-pcm-bank.patch",
        "0005-fix-dac-stream-millisecond-length.patch",
    ):
        apply_numbered_patch(here / name, root)

    run(here / "apply_source_resampler_hooks.py", root)
    run(here / "apply_sn76496_source_tap.py", root)
    run(here / "apply_playera_gain_view.py", root)
    run(here / "apply_playera_postrender_hook.py", root)
    run(here / "apply_playera_deferred_postrender.py", root)
    print("libvgm source-aware VGM capture/replacement patches applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
