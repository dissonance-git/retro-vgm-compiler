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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    root = args.libvgm_root.resolve()
    here = Path(__file__).resolve().parent

    run(here / "apply_source_resampler_hooks.py", root)
    run(here / "apply_sn76496_source_tap.py", root)
    run(here / "apply_playera_gain_view.py", root)
    run(here / "apply_playera_postrender_hook.py", root)
    run(here / "apply_playera_deferred_postrender.py", root)
    print("libvgm source-aware VGM capture/replacement patches applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
