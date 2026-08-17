#!/usr/bin/env python3
"""Apply the independent Enhanced preference and first audible SNESAPU path."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, source_dir: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(source_dir)],
        cwd=str(source_dir),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{script.name} failed with exit code {completed.returncode}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source_dir",
        type=Path,
        help="foo_snesapu/foobar2000/foo_snesapu source directory",
    )
    args = parser.parse_args()
    source = args.source_dir.resolve()
    here = Path(__file__).resolve().parent

    run(here / "apply_enhanced_ui.py", source)
    run(here / "apply_enhanced_runtime.py", source)
    print("SNESAPU Enhanced component patch set applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
