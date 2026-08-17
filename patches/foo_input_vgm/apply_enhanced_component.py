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
    # Install the source-bank observer while the base seek/decode anchors are
    # still intact. Later deferred FM/PSG patches extend those same lifecycle
    # seams, while the final DAC mix patch composes their resulting frame.
    run(here / "apply_enhanced_dac_stream_observer.py", source)
    run(here / "apply_hq_nuked_fm_lift.py", source)
    # Legacy filenames below are all part of the one enhanced option.
    run(here / "apply_studio_hq_fm_observer.py", source)
    run(here / "apply_enhanced_runtime.py", source)
    run(here / "apply_enhanced_family_independence.py", source)
    run(here / "apply_studio_hq_fm_runtime.py", source)
    run(here / "apply_studio_hq_fm_session_reset.py", source)
    # The early PCM observer reset now sits inside the expanded seek lifecycle.
    # Relocate it before the PSG patch consumes its stable seek anchor.
    run(here / "apply_enhanced_dac_stream_seek_order_bridge.py", source)
    run(here / "apply_studio_deferred_psg.py", source)
    run(here / "apply_studio_deferred_family_independence.py", source)
    run(here / "apply_studio_deferred_psg_fail_closed.py", source)
    run(here / "apply_studio_deferred_psg_session_reset.py", source)
    run(here / "apply_enhanced_dac_runtime.py", source)
    run(here / "apply_enhanced_dac_stream_session_reset.py", source)
    run(here / "apply_enhanced_dac_stream_mix.py", source)
    # Spatial consumes only the already-finalized source choices. Keep this
    # after every source-quality patch so presentation cannot affect admission.
    run(here / "apply_spatial_selected_source_transport.py", source)
    run(here / "apply_spatial_omniphony_runtime.py", source)
    print("foo_input_vgm enhanced + Spatial component patch set applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
