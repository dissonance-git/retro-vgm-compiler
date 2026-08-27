#!/usr/bin/env python3
"""Apply pinned SPC runtime, ordering, route, and native-source hooks.

The upstream SPC_MORE_ACCURACY switch changes more than host callback ordering,
including a probabilistic timer-glitch model. The forensic build does not enable
that broad switch. This coordinator applies the existing runtime evidence hooks,
exact-sentinel shared-memory ordering classes, exact live VOLL/VOLR/EON state
observations, and the read-only ten-plane native spatial observer used by the
governor experiment.
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

SPATIAL_REGISTER_PATH = Path(__file__).with_name(
    "patch_snes_spc_spatial_register_observer.py"
)
SPATIAL_REGISTER_SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_spatial_register_observer_forensic",
    SPATIAL_REGISTER_PATH,
)
if SPATIAL_REGISTER_SPEC is None or SPATIAL_REGISTER_SPEC.loader is None:
    raise RuntimeError(f"could not load {SPATIAL_REGISTER_PATH}")
spatial_register = importlib.util.module_from_spec(SPATIAL_REGISTER_SPEC)
sys.modules[SPATIAL_REGISTER_SPEC.name] = spatial_register
SPATIAL_REGISTER_SPEC.loader.exec_module(spatial_register)

NATIVE_SPATIAL_PATH = Path(__file__).with_name(
    "patch_snes_spc_native_spatial_observer.py"
)
NATIVE_SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_native_spatial_observer_forensic",
    NATIVE_SPATIAL_PATH,
)
if NATIVE_SPEC is None or NATIVE_SPEC.loader is None:
    raise RuntimeError(f"could not load {NATIVE_SPATIAL_PATH}")
native_spatial = importlib.util.module_from_spec(NATIVE_SPEC)
sys.modules[NATIVE_SPEC.name] = native_spatial
NATIVE_SPEC.loader.exec_module(native_spatial)


ORDERING_OLD = """#if SPC_MORE_ACCURACY
\t#define MEM_ACCESS( time, addr ) \\
\t{\\
\t\tif ( time >= m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, max_reg_time );\\
\t\t}\\
\t}
#elif !defined (NDEBUG)
\t// Debug-only check for read/write within echo buffer, since this might result in
\t// inaccurate emulation due to the DSP not being caught up to the present.
\t
\tbool SNES_SPC::check_echo_access( int addr )
\t{
\t\tif ( !(dsp.read( SPC_DSP::r_flg ) & 0x20) )
\t\t{
\t\t\tint start = 0x100 * dsp.read( SPC_DSP::r_esa );
\t\t\tint size  = 0x800 * (dsp.read( SPC_DSP::r_edl ) & 0x0F);
\t\t\tint end   = start + (size ? size : 4);
\t\t\tif ( start <= addr && addr < end )
\t\t\t{
\t\t\t\tif ( !m.echo_accessed )
\t\t\t\t{
\t\t\t\t\tm.echo_accessed = 1;
\t\t\t\t\treturn true;
\t\t\t\t}
\t\t\t}
\t\t}
\t\treturn false;
\t}
\t
\t#define MEM_ACCESS( time, addr ) check( !check_echo_access( (uint16_t) addr ) );
#else
\t#define MEM_ACCESS( time, addr )
#endif
"""

ORDERING_NEW = """#if defined(RETRO_VGM_SPC_FORENSIC_ORDERING) && RETRO_VGM_SPC_FORENSIC_ORDERING
\t// Forensic ordering v2 separates the two real APURAM hazards from native
\t// DSP-register synchronization. CPU writes to shared RAM must not overtake
\t// DSP reads. CPU reads only need a catch-up when they can observe the DSP's
\t// echo writes. $F0-$FF are SPC700 registers rather than ordinary APURAM;
\t// $F1 is the one write-side exception because changing CONTROL can replace
\t// the visible $FFC0-$FFFF IPL window.
\t#define MEM_WRITE_ACCESS( time, addr ) \\
\t{\\
\t\tunsigned const forensic_addr = (uint16_t) (addr);\\
\t\tbool const forensic_shared_ram =\\
\t\t\tforensic_addr < 0xF0 || forensic_addr >= 0x100 || forensic_addr == 0xF1;\\
\t\tif ( forensic_shared_ram && time > m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, 0 );\\
\t\t}\\
\t}
\t#define MEM_READ_ACCESS( time, addr ) \\
\t{\\
\t\tunsigned const forensic_addr = (uint16_t) (addr);\\
\t\tbool const forensic_shared_ram = forensic_addr < 0xF0 || forensic_addr >= 0x100;\\
\t\tif ( forensic_shared_ram && !(dsp.read( SPC_DSP::r_flg ) & 0x20) )\\
\t\t{\\
\t\t\tint const forensic_start = 0x100 * dsp.read( SPC_DSP::r_esa );\\
\t\t\tint const forensic_size = 0x800 * (dsp.read( SPC_DSP::r_edl ) & 0x0F);\\
\t\t\tint const forensic_end = forensic_start + (forensic_size ? forensic_size : 4);\\
\t\t\tif ( forensic_start <= (int) forensic_addr &&\\
\t\t\t\t\t(int) forensic_addr < forensic_end && time > m.dsp_time )\\
\t\t\t{\\
\t\t\t\tRUN_DSP( time, 0 );\\
\t\t\t}\\
\t\t}\\
\t}
#elif SPC_MORE_ACCURACY
\t#define MEM_WRITE_ACCESS( time, addr ) \\
\t{\\
\t\tif ( time >= m.dsp_time )\\
\t\t{\\
\t\t\tRUN_DSP( time, max_reg_time );\\
\t\t}\\
\t}
\t#define MEM_READ_ACCESS( time, addr ) MEM_WRITE_ACCESS( time, addr )
#elif !defined (NDEBUG)
\t// Debug-only check for read/write within echo buffer, since this might result in
\t// inaccurate emulation due to the DSP not being caught up to the present.
\t
\tbool SNES_SPC::check_echo_access( int addr )
\t{
\t\tif ( !(dsp.read( SPC_DSP::r_flg ) & 0x20) )
\t\t{
\t\t\tint start = 0x100 * dsp.read( SPC_DSP::r_esa );
\t\t\tint size  = 0x800 * (dsp.read( SPC_DSP::r_edl ) & 0x0F);
\t\t\tint end   = start + (size ? size : 4);
\t\t\tif ( start <= addr && addr < end )
\t\t\t{
\t\t\t\tif ( !m.echo_accessed )
\t\t\t\t{
\t\t\t\t\tm.echo_accessed = 1;
\t\t\t\t\treturn true;
\t\t\t\t}
\t\t\t}
\t\t}
\t\treturn false;
\t}
\t
\t#define MEM_WRITE_ACCESS( time, addr ) check( !check_echo_access( (uint16_t) addr ) );
\t#define MEM_READ_ACCESS( time, addr ) MEM_WRITE_ACCESS( time, addr )
#else
\t#define MEM_WRITE_ACCESS( time, addr )
\t#define MEM_READ_ACCESS( time, addr )
#endif
"""

CPU_WRITE_ACCESS_OLD = """void SNES_SPC::cpu_write( int data, int addr, rel_time_t time )
{
\tMEM_ACCESS( time, addr )
"""

CPU_WRITE_ACCESS_NEW = """void SNES_SPC::cpu_write( int data, int addr, rel_time_t time )
{
\tMEM_WRITE_ACCESS( time, addr )
"""

CPU_READ_ACCESS_OLD = """int SNES_SPC::cpu_read( int addr, rel_time_t time )
{
\tMEM_ACCESS( time, addr )
"""

CPU_READ_ACCESS_NEW = """int SNES_SPC::cpu_read( int addr, rel_time_t time )
{
\tMEM_READ_ACCESS( time, addr )
"""


def patch(root: Path) -> None:
    # The native-spatial observer must claim its pristine SPC_DSP state sentinel
    # before runtime instrumentation inserts adjacent host-only state. This is a
    # composition-order contract, not a relaxation of either strict patcher.
    native_spatial.patch(root)

    # Apply every runtime hook through the per-edit strict transformer after the
    # native observer has preserved its own exact source-shape obligation.
    strict.patcher.replace_once = strict.strict_replace_once
    strict.patcher.patch(root)

    # Add only the host-order synchronization needed by the experiment.
    # Ordinary APURAM writes, echo-overlap reads, and native DSP-register
    # accesses have distinct timing obligations. Keeping them distinct also
    # prevents a generic pre-catch from turning a later exact DSP-register
    # synchronization into a zero-clock DSP run.
    source = root / "snes_spc" / "SNES_SPC.cpp"
    strict.strict_replace_once(
        source,
        ORDERING_OLD,
        ORDERING_NEW,
        "forensic shared-memory DSP synchronization",
    )
    strict.strict_replace_once(
        source,
        CPU_WRITE_ACCESS_OLD,
        CPU_WRITE_ACCESS_NEW,
        "forensic CPU write ordering class",
    )
    strict.strict_replace_once(
        source,
        CPU_READ_ACCESS_OLD,
        CPU_READ_ACCESS_NEW,
        "forensic CPU read ordering class",
    )

    # Observe exact live route/effect-send register state after real DSP writes.
    spatial_register.patch(root)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    patch(args.source_root.resolve())


if __name__ == "__main__":
    main()
