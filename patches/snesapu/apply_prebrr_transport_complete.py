#!/usr/bin/env python3
"""Apply the complete current sampled-source parent/child transport stack."""

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
    parser.add_argument("foo_snesapu_root", type=Path)
    args = parser.parse_args()
    root = args.foo_snesapu_root.resolve()
    here = Path(__file__).resolve().parent

    run(here / "apply_prebrr_transport.py", root)
    run(here / "fix_prebrr_pointer_callback.py", root)
    run(here / "apply_studio_source_transport.py", root)
    print("complete verified sampled-source parent/child transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
