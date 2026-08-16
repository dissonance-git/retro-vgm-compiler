#!/usr/bin/env python3
"""Apply the pinned runtime hooks plus deterministic forensic ordering.

The upstream SPC_MORE_ACCURACY switch changes more than host callback ordering,
including a probabilistic timer-glitch model. The forensic build does not enable
that broad switch. Instead this coordinator adds one exact-sentinel MEM_ACCESS
branch that only catches the accurate DSP up before SPC700 memory accesses.
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys


STRICT_PATH = Path(__file__).with_name("patch_snes_spc_runtime_strict.py")
SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_runtime_strict_forensic",
    STRICT_PATH,
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {STRICT_PATH}")
strict = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = strict
SPEC.loader.exec_module(strict)


ORDERING_OLD = """#if SPC_MORE_ACCURACY
\t#define MEM_ACCESS( time, addr ) \\
\t{\\
\t\tif ( time >= m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, max_reg_time );\\
\t\t}\\
\t}
#elif !defined (NDEBUG)
"""

ORDERING_NEW = """#if defined(RETRO_VGM_SPC_FORENSIC_ORDERING) && RETRO_VGM_SPC_FORENSIC_ORDERING
\t// Forensic-only host ordering: catch the accurate DSP up before a CPU
\t// memory access without enabling SPC_MORE_ACCURACY's unrelated timer/glitch
\t// behavior. Use a strict inequality because the normal accurate RUN_DSP
\t// path requires a positive clock count.
\t#define MEM_ACCESS( time, addr ) \\
\t{\\
\t\tif ( time > m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, 0 );\\
\t\t}\\
\t}
#elif SPC_MORE_ACCURACY
\t#define MEM_ACCESS( time, addr ) \\
\t{\\
\t\tif ( time >= m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, max_reg_time );\\
\t\t}\\
\t}
#elif !defined (NDEBUG)
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()

    # First apply every runtime hook through the per-edit strict transformer.
    strict.patcher.replace_once = strict.strict_replace_once
    strict.patcher.patch(root)

    # Then add only the host-order synchronization needed by the experiment.
    strict.strict_replace_once(
        root / "snes_spc" / "SNES_SPC.cpp",
        ORDERING_OLD,
        ORDERING_NEW,
        "forensic MEM_ACCESS DSP synchronization",
    )


if __name__ == "__main__":
    main()
