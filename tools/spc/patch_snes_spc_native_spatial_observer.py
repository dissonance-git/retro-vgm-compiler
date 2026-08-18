#!/usr/bin/env python3
"""Add a read-only ten-plane spatial observer to the pinned snes_spc DSP.

This patch is forensic/research instrumentation only. It exposes the accurate
DSP's eight enveloped mono voice values before VOLL/VOLR routing together with
the shared echo return after EVOL has been applied. It must not change synthesis,
register state, echo feedback, mute state, or output arithmetic.

The caller owns timeline ordinals. The dependency callback therefore carries
only the ten simultaneous source amplitudes for one native 32 kHz DSP frame.
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys


STRICT_PATH = Path(__file__).with_name("patch_snes_spc_runtime_strict.py")
SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_runtime_strict_native_spatial",
    STRICT_PATH,
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {STRICT_PATH}")
strict = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = strict
SPEC.loader.exec_module(strict)


DSP_API_OLD = """\ttypedef short sample_t;
\tvoid set_output( sample_t* out, int out_size );
"""

DSP_API_NEW = """\ttypedef short sample_t;
\tvoid set_output( sample_t* out, int out_size );
\n\t// RETRO_VGM_COMPILER_SNES_SPC_NATIVE_SPATIAL_OBSERVER
\t// Observation only: eight pre-route dry voices plus the shared post-EVOL
\t// return for the same native DSP frame. No source identity is inferred here.
\ttypedef void (*native_spatial_observer_t)(
\t\tvoid* user,
\t\tsample_t const* dry_voice,
\t\tint dry_voice_count,
\t\tsample_t echo_left,
\t\tsample_t echo_right );
\tvoid set_native_spatial_observer( native_spatial_observer_t observer, void* user );
"""

DSP_STATE_OLD = """\tvoid init_counter();
"""

DSP_STATE_NEW = """\t// Host-only observation state. These values are outside state_t and therefore
\t// cannot enter emulator save/copy state or alter causal DSP state.
\tnative_spatial_observer_t native_spatial_observer = 0;
\tvoid* native_spatial_user = 0;
\tsample_t native_dry_source [voice_count] = {};
\tsample_t native_echo_left = 0;
\n\tvoid init_counter();
"""

DSP_INLINE_OLD = """inline void SPC_DSP::mute_voices( int mask ) { m.mute_mask = mask; }
"""

DSP_INLINE_NEW = """inline void SPC_DSP::set_native_spatial_observer(
\t\tnative_spatial_observer_t observer, void* user )
{
\tnative_spatial_observer = observer;
\tnative_spatial_user = user;
\tfor ( int voice = 0; voice < voice_count; ++voice )
\t\tnative_dry_source [voice] = 0;
\tnative_echo_left = 0;
}

inline void SPC_DSP::mute_voices( int mask ) { m.mute_mask = mask; }
"""

SNES_API_OLD = """\ttypedef short sample_t;
\tvoid set_output( sample_t* out, int out_size );
"""

SNES_API_NEW = """\ttypedef short sample_t;
\tvoid set_output( sample_t* out, int out_size );
\n\ttypedef SPC_DSP::native_spatial_observer_t native_spatial_observer_t;
\tvoid set_native_spatial_observer( native_spatial_observer_t observer, void* user );
"""

SNES_INLINE_OLD = """inline void SNES_SPC::mute_voices( int mask ) { dsp.mute_voices( mask ); }
"""

SNES_INLINE_NEW = """inline void SNES_SPC::set_native_spatial_observer(
\t\tnative_spatial_observer_t observer, void* user )
{
\tdsp.set_native_spatial_observer( observer, user );
}

inline void SNES_SPC::mute_voices( int mask ) { dsp.mute_voices( mask ); }
"""

# V3c owns the causal mono source value after interpolation/noise, PMON
# consequences and envelope, before voice_output() applies VOLL/VOLR.
DRY_TAP_OLD = """\t\tm.t_output = (output * v->env) >> 11 & ~1;
\t\tv->t_envx_out = (uint8_t) (v->env >> 4);
\t}
"""

DRY_TAP_NEW = """\t\tm.t_output = (output * v->env) >> 11 & ~1;
\t\tv->t_envx_out = (uint8_t) (v->env >> 4);
\t\tnative_dry_source [v - m.voices] = (sample_t) m.t_output;
\t}
"""

# ECHO 26/27 are the exact point where the shared FIR return is multiplied by
# signed EVOL for left/right. Capture that contribution separately from MVOL dry
# arithmetic; the normal echo_output() and WRITE_SAMPLES path remains untouched.
ECHO_LEFT_OLD = """\tm.t_main_out [0] = echo_output( 0 );
"""

ECHO_LEFT_NEW = """\tnative_echo_left = (sample_t) (int16_t)
\t\t((m.t_echo_in [0] * (int8_t) REG(evoll)) >> 7);
\tm.t_main_out [0] = echo_output( 0 );
"""

ECHO_RIGHT_OLD = """\tint l = m.t_main_out [0];
\tint r = echo_output( 1 );
\tm.t_main_out [0] = 0;
"""

ECHO_RIGHT_NEW = """\tint l = m.t_main_out [0];
\tint r = echo_output( 1 );
\tsample_t const native_echo_right = (sample_t) (int16_t)
\t\t((m.t_echo_in [1] * (int8_t) REG(evoll + 0x10)) >> 7);
\tif ( native_spatial_observer )
\t{
\t\tnative_spatial_observer(
\t\t\tnative_spatial_user,
\t\t\tnative_dry_source,
\t\t\tvoice_count,
\t\t\tnative_echo_left,
\t\t\tnative_echo_right );
\t}
\tm.t_main_out [0] = 0;
"""


def patch(root: Path) -> None:
    snes_header = root / "snes_spc" / "SNES_SPC.h"
    dsp_header = root / "snes_spc" / "SPC_DSP.h"
    dsp_cpp = root / "snes_spc" / "SPC_DSP.cpp"

    strict.strict_replace_once(dsp_header, DSP_API_OLD, DSP_API_NEW, "native spatial DSP API")
    strict.strict_replace_once(dsp_header, DSP_STATE_OLD, DSP_STATE_NEW, "native spatial DSP state")
    strict.strict_replace_once(dsp_header, DSP_INLINE_OLD, DSP_INLINE_NEW, "native spatial DSP setter")
    strict.strict_replace_once(snes_header, SNES_API_OLD, SNES_API_NEW, "native spatial SNES API")
    strict.strict_replace_once(snes_header, SNES_INLINE_OLD, SNES_INLINE_NEW, "native spatial SNES setter")
    strict.strict_replace_once(dsp_cpp, DRY_TAP_OLD, DRY_TAP_NEW, "pre-route dry source tap")
    strict.strict_replace_once(dsp_cpp, ECHO_LEFT_OLD, ECHO_LEFT_NEW, "post-EVOL left echo tap")
    strict.strict_replace_once(dsp_cpp, ECHO_RIGHT_OLD, ECHO_RIGHT_NEW, "post-EVOL right echo tap")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    patch(args.source_root.resolve())


if __name__ == "__main__":
    main()
