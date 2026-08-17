#!/usr/bin/env python3
"""Apply the complete current sampled-source parent/child transport stack."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, target: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(target)],
        cwd=str(target),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{script.name} failed with exit code {completed.returncode}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    here = Path(__file__).resolve().parent

    # The parent is the audited historical SRCE-v2 host. The child is already
    # the canonical current SRCE-v2 implementation. Patch each from its actual
    # baseline instead of replaying historical child migrations.
    run(here / "apply_current_parent_source_transport.py", root)
    run(here / "apply_current_child_source_transport.py", root / "spcplayer")
    print("complete current verified sampled-source parent/child transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
