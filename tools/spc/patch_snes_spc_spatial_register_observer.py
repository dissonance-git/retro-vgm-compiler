#!/usr/bin/env python3
"""Expose exact live S-DSP VOLL/VOLR/EON state changes to the spatial adapter.

This is a strict, read-only extension applied after the base runtime patch. It
observes only real DSP register writes after they have taken effect. It does not
change register timing, DSP arithmetic, CPU state, or source generation.
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys


STRICT_PATH = Path(__file__).with_name("patch_snes_spc_runtime_strict.py")
SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_runtime_strict_spatial_register",
    STRICT_PATH,
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {STRICT_PATH}")
strict = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = strict
SPEC.loader.exec_module(strict)


INCLUDE_OLD = """#include "SNES_SPC.h"
#include "components/spc/snes_spc_runtime_hook_bridge.h"

#include <string.h>
"""

INCLUDE_NEW = """#include "SNES_SPC.h"
#include "components/spc/snes_spc_runtime_hook_bridge.h"
#include "components/spc/snes_spc_spatial_register_hook_bridge.h"

#include <string.h>
"""

DSP_WRITE_OLD = """\tif ( REGS [r_dspaddr] <= 0x7F )
\t\tdsp.write( REGS [r_dspaddr], data );
\telse if ( !SPC_MORE_ACCURACY )
\t\tdprintf( "SPC wrote to DSP register > $7F\\n" );
"""

DSP_WRITE_NEW = """\tint const spatial_reg = REGS [r_dspaddr];
\tif ( spatial_reg <= 0x7F )
\t{
\t\tdsp.write( spatial_reg, data );
\n\t\t// RETRO_VGM_COMPILER_SNES_SPC_SPATIAL_REGISTER_OBSERVER
\t\t// VOLL/VOLR are signed /128 hardware coefficients. Emit the complete
\t\t// current route for that voice only after the real write has landed.
\t\tif ( instrumentation_sink && spatial_reg <= 0x71 &&
\t\t\t\t(spatial_reg & 0x0F) <= 1 )
\t\t{
\t\t\tint const voice = spatial_reg >> 4;
\t\t\tint const base = voice << 4;
\t\t\tgameaudio::spc::observe_snes_spc_spatial_route_state(
\t\t\t\tinstrumentation_sink,
\t\t\t\t(long long) instrumentation_tick( time ),
\t\t\t\tclock_rate,
\t\t\t\t(uint8_t) voice,
\t\t\t\t(uint8_t) dsp.read( base + SPC_DSP::v_voll ),
\t\t\t\t(uint8_t) dsp.read( base + SPC_DSP::v_volr ),
\t\t\t\t(dsp.read( SPC_DSP::r_eon ) & (1 << voice)) != 0 );
\t\t}
\t\telse if ( instrumentation_sink && spatial_reg == SPC_DSP::r_eon )
\t\t{
\t\t\t// EON is one shared bitfield write. Publish all eight complete voice
\t\t\t// states at this exact tick so effect-send changes never inherit a
\t\t\t// stale per-voice route snapshot.
\t\t\tfor ( int voice = 0; voice < SPC_DSP::voice_count; ++voice )
\t\t\t{
\t\t\t\tint const base = voice << 4;
\t\t\t\tgameaudio::spc::observe_snes_spc_spatial_route_state(
\t\t\t\t\tinstrumentation_sink,
\t\t\t\t\t(long long) instrumentation_tick( time ),
\t\t\t\t\tclock_rate,
\t\t\t\t\t(uint8_t) voice,
\t\t\t\t\t(uint8_t) dsp.read( base + SPC_DSP::v_voll ),
\t\t\t\t\t(uint8_t) dsp.read( base + SPC_DSP::v_volr ),
\t\t\t\t\t(dsp.read( SPC_DSP::r_eon ) & (1 << voice)) != 0 );
\t\t\t}
\t\t}
\t}
\telse if ( !SPC_MORE_ACCURACY )
\t\tdprintf( "SPC wrote to DSP register > $7F\\n" );
"""


def patch(root: Path) -> None:
    path = root / "snes_spc" / "SNES_SPC.cpp"
    strict.strict_replace_once(path, INCLUDE_OLD, INCLUDE_NEW, "spatial register bridge include")
    strict.strict_replace_once(path, DSP_WRITE_OLD, DSP_WRITE_NEW, "spatial DSP register observation")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    patch(args.source_root.resolve())


if __name__ == "__main__":
    main()
