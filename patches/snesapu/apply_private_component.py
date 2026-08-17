#!/usr/bin/env python3
"""Apply the complete private foo_snesapu parent/component patch stack.

The source tree must be the SRCE-v2-capable vgmspc base pinned by
apply_spatial_omniphony_private_runtime.py. SNESAPU itself is patched/built
separately because it is a distinct 32-bit dependency tree.
"""

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


def add_algorithm_include(stdafx: Path) -> None:
    raw = stdafx.read_bytes()
    newline = b"\r\n" if b"\r\n" in raw else b"\n"
    marker = b"#include <pfc/pfc.h>"
    if b"#include <algorithm>" in raw:
        return
    if raw.count(marker) != 1:
        raise RuntimeError(f"expected one pfc include in {stdafx}")
    stdafx.write_bytes(raw.replace(
        marker,
        b"#include <algorithm>" + newline + marker,
        1,
    ))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    here = Path(__file__).resolve().parent
    parent = root / "foobar2000" / "foo_snesapu"

    run(here / "apply_enhanced_component.py", parent)
    run(here / "apply_prebrr_transport_complete.py", root)
    run(here / "apply_spatial_omniphony_private_runtime.py", root)
    add_algorithm_include(parent / "stdafx.h")

    print("private foo_snesapu Enhanced + Spatial patch stack applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
