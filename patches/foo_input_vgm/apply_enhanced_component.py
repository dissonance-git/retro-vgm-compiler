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


def diagnose_generated_host(source_dir: Path) -> None:
    """Print narrow contexts for the identifiers from the current MSVC failure.

    The historical host is reconstructed only inside the private Windows build,
    so these diagnostics make the generated patch boundary observable without
    checking that private source snapshot into Git. Keep the context deliberately
    small: enough to identify a bad insertion scope, not a source dump.
    """
    tokens = (
        "enhancedDacStream",
        "CurLoop",
        "dacState",
        "rawSample",
        "subtick",
        "TempByt",
        "TempSht",
        "sampleIndex",
        "playbackClock",
        "hostSampleRate",
        "frameIndex",
        "outputScratch",
        "lastObservedSourceState",
        "enhancedPsgSource",
        "enhancedFmSource",
        "routeEnhancedGenesisFrame",
        "clearEnhancedGenesisRoute",
        "setEnhancedGenesisRouteSampleRate",
    )
    suffixes = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
    print("== generated VGM host diagnostic ==")
    for path in sorted(source_dir.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        raw = path.read_bytes()
        try:
            text = raw.decode("utf-8-sig")
        except UnicodeDecodeError:
            text = raw.decode("cp932")
        lines = text.splitlines()
        for index, line in enumerate(lines):
            hits = [token for token in tokens if token in line]
            if not hits:
                continue
            rel = path.relative_to(source_dir)
            print(f"VGM_GENERATED_DIAG {rel}:{index + 1} tokens={','.join(hits)}")
            lo = max(0, index - 2)
            hi = min(len(lines), index + 3)
            for context_index in range(lo, hi):
                print(f"  {context_index + 1:05d}: {lines[context_index]}")
    print("== end generated VGM host diagnostic ==")


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
    # The preserved 0.31 host uses libvgm's newer Game Boy interface header
    # name. Keep the verified libvgm pin and translate that one include to the
    # equivalent public header owned by the pinned revision.
    run(here / "apply_pinned_libvgm_compat.py", source)
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
    diagnose_generated_host(source)
    print("foo_input_vgm Genesis enhanced + Surround/Omniphony component patch set applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
