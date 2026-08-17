#!/usr/bin/env python3
"""Apply the independent enhanced + source-aware VGM foobar shell patches.

The enhanced option is the sole source-native quality option. A few patch
filenames still carry a legacy ``studio_*`` development prefix; they are
implementation names, not a separate mode, tier, or proper name.
"""

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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    source = args.source_dir.resolve()
    here = Path(__file__).resolve().parent

    run(here / "apply_enhanced_ui.py", source)
    run(here / "apply_source_aware_player.py", source)
    run(here / "apply_source_aware_shadow_include.py", source)
    run(here / "apply_hq_nuked_fm_lift.py", source)
    # Legacy filenames below are all part of the one enhanced option.
    run(here / "apply_studio_hq_fm_observer.py", source)
    run(here / "apply_enhanced_runtime.py", source)
    run(here / "apply_studio_hq_fm_runtime.py", source)
    run(here / "apply_studio_hq_fm_session_reset.py", source)
    run(here / "apply_studio_deferred_psg.py", source)
    run(here / "apply_studio_deferred_family_independence.py", source)
    run(here / "apply_studio_deferred_psg_fail_closed.py", source)
    run(here / "apply_studio_deferred_psg_session_reset.py", source)
    run(here / "apply_enhanced_dac_runtime.py", source)
    print("foo_input_vgm enhanced component patch set applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
