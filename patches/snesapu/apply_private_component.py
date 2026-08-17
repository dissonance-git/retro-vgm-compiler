#!/usr/bin/env python3
"""Apply the complete private foo_snesapu parent/component patch stack.

The source tree must be the internal SRCE-v2-capable foo_snesapu bootstrap
materialized by tools/materialize_foo_snesapu.py. SNESAPU itself is
patched/built separately because it is a distinct 32-bit dependency tree.
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


def remove_obsolete_enhancer_seek_reset(source: Path) -> None:
    """Remove the final legacy SemanticStereoEnhancer state touch.

    The current Spatial patch removes m_Enhancer from the class and replaces its
    decode/allocation paths, but the historical backward-seek branch still
    carried one reset call. Spatial identity is reset at the seek destination by
    ResetSpatialRuntime, so retaining this dead member access is both unnecessary
    and a compile error.
    """
    raw = source.read_bytes()
    newline = b"\r\n" if b"\r\n" in raw else b"\n"
    old = b"\t\tm_PlayedTime = 0;" + newline + b"\t\tm_Enhancer.reset();"
    new = b"\t\tm_PlayedTime = 0;"
    count = raw.count(old)
    if count != 1:
        raise RuntimeError(
            f"obsolete enhancer seek reset: expected exactly one match in {source}, found {count}"
        )
    source.write_bytes(raw.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    here = Path(__file__).resolve().parent
    parent = root / "foobar2000" / "foo_snesapu"

    run(here / "apply_enhanced_component.py", parent)
    run(here / "apply_prebrr_transport_complete.py", root)
    run(here / "apply_spatial_omniphony_private_runtime.py", root)
    run(here / "apply_spatial_omniphony_private_rate_lifecycle.py", root)
    remove_obsolete_enhancer_seek_reset(parent / "input_snesapu.cpp")
    add_algorithm_include(parent / "stdafx.h")

    print("private foo_snesapu enhanced + Spatial patch stack applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
