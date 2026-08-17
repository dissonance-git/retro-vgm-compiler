#!/usr/bin/env python3
"""Apply the complete private source-aware SNESAPU patch stack."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, root: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(root)],
        cwd=str(root),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{script.name} failed with exit code {completed.returncode}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("spcplay_root", type=Path)
    root = parser.parse_args().spcplay_root.resolve()
    here = Path(__file__).resolve().parent

    run(here / "apply_source_capture.py", root)
    run(here / "upgrade_source_capture_v2.py", root)
    run(here / "apply_prebrr_provider.py", root)
    run(here / "apply_studio_source_provider.py", root)
    print("private source-aware SNESAPU patch stack applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
