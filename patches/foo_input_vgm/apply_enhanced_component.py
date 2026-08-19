#!/usr/bin/env python3
"""Apply the independent enhanced + source-aware VGM foobar shell patches.

The enhanced option is the sole source-native quality option. The existing
foobar Surround preference is the sole user-facing spatial switch. A few patch
filenames still carry a legacy ``studio_*`` development prefix; they are
implementation names, not a separate mode, tier, or proper name.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, source_dir: Path, *extra: str) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(source_dir), *extra],
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
    # Private package output is one 48 kHz host timeline in every source-quality
    # and surround combination. This does not select enhanced or presentation.
    run(here / "apply_private_48khz_output.py", source)
    # Reconstruct the small source-aware host seam that the later guarded
    # transformations historically consumed, without copying the old patched
    # host snapshot over the exact foo_input_vgm 0.31 source tree.
    run(here / "apply_source_aware_host_foundation.py", source)
    run(here / "apply_source_aware_player.py", source)

    # This first installable private VGM runtime is intentionally Genesis-first:
    # YM2612 + DAC + PSG are the promoted chip-specific implementation. The
    # independent YM2151/OPM reference-capture experiment remains in the repo,
    # but it is not a prerequisite for shipping this Genesis + SPC milestone.
    # Ordinary 0.31 playback for non-Genesis VGM families remains untouched.
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
    # Spatial presentation consumes only already-finalized source choices. Keep
    # it after every source-quality patch so presentation cannot affect admission.
    run(here / "apply_spatial_selected_source_transport.py", source)
    # The DAC observer owns an exact sample-boundary advance that historically
    # occupied the old spatial route patch anchor. Expose that anchor only while
    # applying the spatial patch, then restore the PCM advance immediately after
    # the new route observation. The compiled source always contains both.
    run(here / "apply_spatial_route_order_bridge.py", source, "prepare")
    run(here / "apply_spatial_omniphony_runtime.py", source)
    run(here / "apply_spatial_route_order_bridge.py", source, "restore")
    # Reuse the historical Surround preference and neutralize libvgm's old
    # channel-inversion surround effect before any audio reaches the user.
    run(here / "apply_surround_omniphony_bridge.py", source)
    run(here / "apply_spatial_omniphony_rate_lifecycle.py", source)
    print("foo_input_vgm Genesis enhanced + Surround/Omniphony component patch set applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
