#!/usr/bin/env python3
"""Patch pinned snes_spc 0.9.0 with the minimal runtime-observation hooks.

This is intentionally an exact-sentinel transformer, not a fuzzy patcher. The
forensic build pins ec8ee2bbe30451614c1d02a83f7af1c97d497d45; if any expected
upstream fragment is absent or duplicated, configuration fails closed.
"""

from __future__ import annotations

import argparse
from pathlib import Path


PINNED_COMMIT = "ec8ee2bbe30451614c1d02a83f7af1c97d497d45"
PATCH_MARKER = "RETRO_VGM_COMPILER_SNES_SPC_RUNTIME_HOOKS"


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if PATCH_MARKER in text and old not in text:
        return
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{path}: expected exactly one {label!r} sentinel, found {count}; "
            "refusing to patch an unknown snes_spc source"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def patch_snes_spc_header(root: Path) -> None:
    path = root / "snes_spc" / "SNES_SPC.h"
    replace_once(
        path,
        '#include "SPC_DSP.h"\n#include "blargg_endian.h"\n',
        '#include "SPC_DSP.h"\n#include "blargg_endian.h"\n\n'
        f'// {PATCH_MARKER}\n'
        'namespace gameaudio { namespace spc { class spc_runtime_instrumentation_sink; } }\n',
        "SNES_SPC forward declaration",
    )
    replace_once(
        path,
        '\tblargg_err_t init();\n',
        '\tblargg_err_t init();\n\tvoid set_runtime_instrumentation_sink(\n'
        '\t\tgameaudio::spc::spc_runtime_instrumentation_sink* sink );\n',
        "SNES_SPC sink setter",
    )
    replace_once(
        path,
        '\tSPC_DSP dsp;\n',
        '\tSPC_DSP dsp;\n'
        '\tgameaudio::spc::spc_runtime_instrumentation_sink* instrumentation_sink;\n'
        '\tunsigned long long instrumentation_time_base;\n'
        '\tunsigned long long instrumentation_tick( rel_time_t time ) const;\n',
        "SNES_SPC instrumentation state",
    )
    replace_once(
        path,
        '\tvoid enable_rom( int enable );\n',
        '\tvoid enable_rom( int enable, rel_time_t time );\n',
        "SNES_SPC enable_rom signature",
    )


def patch_dsp_header(root: Path) -> None:
    path = root / "snes_spc" / "SPC_DSP.h"
    replace_once(
        path,
        '#include "blargg_common.h"\n',
        '#include "blargg_common.h"\n\n'
        f'// {PATCH_MARKER}\n'
        'namespace gameaudio { namespace spc { class spc_runtime_instrumentation_sink; } }\n',
        "SPC_DSP forward declaration",
    )
    replace_once(
        path,
        '\tvoid init( void* ram_64k );\n',
        '\tvoid init( void* ram_64k );\n'
        '\tvoid set_runtime_instrumentation_sink(\n'
        '\t\tgameaudio::spc::spc_runtime_instrumentation_sink* sink,\n'
        '\t\tunsigned long long tick_rate );\n',
        "SPC_DSP sink setter",
    )
    replace_once(
        path,
        '\tstate_t m;\n',
        '\tstate_t m;\n\n'
        '\t// Observation-only state. It is deliberately outside state_t so\n'
        '\t// emulator copy_state/save-state behavior never serializes it.\n'
        '\tgameaudio::spc::spc_runtime_instrumentation_sink* instrumentation_sink;\n'
        '\tunsigned long long instrumentation_clock;\n'
        '\tunsigned long long instrumentation_tick_rate;\n'
        '\tint instrumentation_dir_srcn;\n'
        '\tint instrumentation_brr_srcn;\n'
        '\tint instrumentation_loop_address;\n'
        '\tbool instrumentation_voice_inactive [voice_count];\n',
        "SPC_DSP instrumentation state",
    )


def patch_snes_spc_cpp(root: Path) -> None:
    path = root / "snes_spc" / "SNES_SPC.cpp"
    replace_once(
        path,
        '#include "SNES_SPC.h"\n\n#include <string.h>\n',
        '#include "SNES_SPC.h"\n#include "components/spc/snes_spc_runtime_hook_bridge.h"\n\n#include <string.h>\n',
        "SNES_SPC bridge include",
    )
    replace_once(
        path,
        '#define REGS_IN     (m.smp_regs [1])\n\n',
        '#define REGS_IN     (m.smp_regs [1])\n\n'
        'void SNES_SPC::set_runtime_instrumentation_sink(\n'
        '\t\tgameaudio::spc::spc_runtime_instrumentation_sink* sink )\n'
        '{\n'
        '\tinstrumentation_sink = sink;\n'
        '\tinstrumentation_time_base = 0;\n'
        '\tdsp.set_runtime_instrumentation_sink( sink, clock_rate );\n'
        '}\n\n'
        'unsigned long long SNES_SPC::instrumentation_tick( rel_time_t time ) const\n'
        '{\n'
        '\tlong long relative = (long long) m.spc_time + (long long) time;\n'
        '\tif ( relative < 0 )\n'
        '\t\trelative = 0;\n'
        '\treturn instrumentation_time_base + (unsigned long long) relative;\n'
        '}\n\n',
        "SNES_SPC instrumentation helpers",
    )
    replace_once(
        path,
        '''void SNES_SPC::enable_rom( int enable )
{
\tif ( m.rom_enabled != enable )
\t{
\t\tm.rom_enabled = enable;
\t\tif ( enable )
\t\t\tmemcpy( m.hi_ram, &RAM [rom_addr], sizeof m.hi_ram );
\t\tmemcpy( &RAM [rom_addr], (enable ? m.rom : m.hi_ram), rom_size );
\t\t// TODO: ROM can still get overwritten when DSP writes to echo buffer
\t}
}
''',
        '''void SNES_SPC::enable_rom( int enable, rel_time_t time )
{
\tif ( m.rom_enabled != enable )
\t{
\t\tuint8_t old_visible [rom_size];
\t\tif ( instrumentation_sink )
\t\t\tmemcpy( old_visible, &RAM [rom_addr], rom_size );
\t\t
\t\tm.rom_enabled = enable;
\t\tif ( enable )
\t\t\tmemcpy( m.hi_ram, &RAM [rom_addr], sizeof m.hi_ram );
\t\tmemcpy( &RAM [rom_addr], (enable ? m.rom : m.hi_ram), rom_size );
\t\t// TODO: ROM can still get overwritten when DSP writes to echo buffer
\t\t
\t\tif ( instrumentation_sink && memcmp( old_visible, &RAM [rom_addr], rom_size ) )
\t\t{
\t\t\tgameaudio::spc::observe_snes_spc_apuram_mutation(
\t\t\t\tinstrumentation_sink,
\t\t\t\tgameaudio::spc::spc_runtime_ram_write_origin::ipl_rom_overlay,
\t\t\t\t(long long) instrumentation_tick( time ),
\t\t\t\tclock_rate,
\t\t\t\trom_addr,
\t\t\t\t&RAM [rom_addr],
\t\t\t\trom_size );
\t\t}
\t}
}
''',
        "enable_rom instrumentation",
    )
    replace_once(
        path,
        '\t\tenable_rom( data & 0x80 );\n',
        '\t\tenable_rom( data & 0x80, time );\n',
        "runtime enable_rom call",
    )
    replace_once(
        path,
        '''void SNES_SPC::cpu_write( int data, int addr, rel_time_t time )
{
\tMEM_ACCESS( time, addr )
\t
\t// RAM
\tRAM [addr] = (uint8_t) data;
\tint reg = addr - 0xF0;
''',
        '''void SNES_SPC::cpu_write( int data, int addr, rel_time_t time )
{
\tMEM_ACCESS( time, addr )
\t
\t// RAM. High IPL-covered writes are observed after cpu_write_high() restores
\t// the final DSP-visible byte; ordinary writes are observed immediately.
\tuint8_t const old_visible = RAM [addr];
\tbool const defer_high_observation =
\t\t(unsigned) addr < 0x10000 && addr >= rom_addr && m.rom_enabled;
\tRAM [addr] = (uint8_t) data;
\tif ( instrumentation_sink && !defer_high_observation &&
\t\t\t(unsigned) addr < 0x10000 && RAM [addr] != old_visible )
\t{
\t\tgameaudio::spc::observe_snes_spc_apuram_mutation(
\t\t\tinstrumentation_sink,
\t\t\tgameaudio::spc::spc_runtime_ram_write_origin::spc700_cpu,
\t\t\t(long long) instrumentation_tick( time ),
\t\t\tclock_rate,
\t\t\t(uint16_t) addr,
\t\t\t&RAM [addr],
\t\t\t1 );
\t}
\tint reg = addr - 0xF0;
''',
        "CPU RAM observation prologue",
    )
    replace_once(
        path,
        '''\t\t}
\t}
}


//// CPU read
''',
        '''\t\t}
\t}
\t
\tif ( instrumentation_sink && defer_high_observation &&
\t\t\t(unsigned) addr < 0x10000 && RAM [addr] != old_visible )
\t{
\t\tgameaudio::spc::observe_snes_spc_apuram_mutation(
\t\t\tinstrumentation_sink,
\t\t\tgameaudio::spc::spc_runtime_ram_write_origin::spc700_cpu,
\t\t\t(long long) instrumentation_tick( time ),
\t\t\tclock_rate,
\t\t\t(uint16_t) addr,
\t\t\t&RAM [addr],
\t\t\t1 );
\t}
}


//// CPU read
''',
        "CPU RAM deferred observation",
    )
    replace_once(
        path,
        '''\t// Save any extra samples beyond what should be generated
\tif ( m.buf_begin )
\t\tsave_extra();
}
''',
        '''\t// Save any extra samples beyond what should be generated
\tif ( m.buf_begin )
\t\tsave_extra();
\t
\tinstrumentation_time_base += (unsigned long long) end_time;
}
''',
        "monotonic frame clock",
    )


def patch_misc_cpp(root: Path) -> None:
    path = root / "snes_spc" / "SNES_SPC_misc.cpp"
    replace_once(
        path,
        '#include "SNES_SPC.h"\n\n#include <string.h>\n',
        '#include "SNES_SPC.h"\n#include "components/spc/snes_spc_runtime_hook_bridge.h"\n\n#include <string.h>\n',
        "SNES_SPC_misc bridge include",
    )
    replace_once(
        path,
        '''blargg_err_t SNES_SPC::init()
{
\tmemset( &m, 0, sizeof m );
\tdsp.init( RAM );
''',
        '''blargg_err_t SNES_SPC::init()
{
\tinstrumentation_sink = 0;
\tinstrumentation_time_base = 0;
\tmemset( &m, 0, sizeof m );
\tdsp.init( RAM );
''',
        "SNES_SPC initialization",
    )
    replace_once(
        path,
        '\tenable_rom( REGS [r_control] & 0x80 );\n',
        '\tenable_rom( REGS [r_control] & 0x80, 0 );\n',
        "loaded-state enable_rom call",
    )
    replace_once(
        path,
        '''void SNES_SPC::soft_reset()
{
\treset_common( 0 );
\tdsp.soft_reset();
}

void SNES_SPC::reset()
{
\tmemset( RAM, 0xFF, 0x10000 );
\tram_loaded();
\treset_common( 0x0F );
\tdsp.reset();
}
''',
        '''void SNES_SPC::soft_reset()
{
\tgameaudio::spc::observe_snes_spc_execution_reset(
\t\tinstrumentation_sink,
\t\t(long long) instrumentation_time_base,
\t\tclock_rate );
\treset_common( 0 );
\tdsp.soft_reset();
}

void SNES_SPC::reset()
{
\tgameaudio::spc::observe_snes_spc_execution_reset(
\t\tinstrumentation_sink,
\t\t(long long) instrumentation_time_base,
\t\tclock_rate );
\tmemset( RAM, 0xFF, 0x10000 );
\tram_loaded();
\treset_common( 0x0F );
\tdsp.reset();
}
''',
        "execution reset events",
    )


def patch_dsp_cpp(root: Path) -> None:
    path = root / "snes_spc" / "SPC_DSP.cpp"
    replace_once(
        path,
        '#include "SPC_DSP.h"\n\n#include "blargg_endian.h"\n',
        '#include "SPC_DSP.h"\n#include "components/spc/snes_spc_runtime_hook_bridge.h"\n\n#include "blargg_endian.h"\n',
        "SPC_DSP bridge include",
    )
    replace_once(
        path,
        '''inline VOICE_CLOCK( V1 )
{
\tm.t_dir_addr = m.t_dir * 0x100 + m.t_srcn * 4;
\tm.t_srcn = VREG(v->regs,srcn);
}
''',
        '''inline VOICE_CLOCK( V1 )
{
\tif ( instrumentation_sink )
\t\tinstrumentation_dir_srcn = m.t_srcn;
\tm.t_dir_addr = m.t_dir * 0x100 + m.t_srcn * 4;
\tm.t_srcn = VREG(v->regs,srcn);
}
''',
        "directory source pipeline observation",
    )
    replace_once(
        path,
        '''\tm.t_brr_next_addr = GET_LE16A( entry );
\t
\tm.t_adsr0 = VREG(v->regs,adsr0);
''',
        '''\tm.t_brr_next_addr = GET_LE16A( entry );
\tif ( instrumentation_sink && v->kon_delay )
\t{
\t\tinstrumentation_brr_srcn = instrumentation_dir_srcn;
\t\tinstrumentation_loop_address = GET_LE16A( entry + 2 );
\t}
\t
\tm.t_adsr0 = VREG(v->regs,adsr0);
''',
        "BRR source/loop pipeline observation",
    )
    replace_once(
        path,
        '''\t\tif ( v->kon_delay == 5 )
\t\t{
\t\t\tv->brr_addr    = m.t_brr_next_addr;
\t\t\tv->brr_offset  = 1;
\t\t\tv->buf_pos     = 0;
\t\t\tm.t_brr_header = 0; // header is ignored on this sample
\t\t\tm.kon_check    = true;
\t\t}
''',
        '''\t\tif ( v->kon_delay == 5 )
\t\t{
\t\t\tv->brr_addr    = m.t_brr_next_addr;
\t\t\tv->brr_offset  = 1;
\t\t\tv->buf_pos     = 0;
\t\t\tm.t_brr_header = 0; // header is ignored on this sample
\t\t\tm.kon_check    = true;
\t\t\tif ( instrumentation_sink )
\t\t\t{
\t\t\t\tint const voice = (int) (v - m.voices);
\t\t\t\tint const pitch = VREG(v->regs,pitchl) |
\t\t\t\t\t((VREG(v->regs,pitchh) & 0x3F) << 8);
\t\t\t\tgameaudio::spc::observe_snes_spc_sample_phase_started(
\t\t\t\t\tinstrumentation_sink,
\t\t\t\t\t(long long) instrumentation_clock,
\t\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t\t(uint8_t) voice,
\t\t\t\t\t(uint8_t) instrumentation_brr_srcn,
\t\t\t\t\t(uint16_t) v->brr_addr,
\t\t\t\t\t(uint16_t) instrumentation_loop_address,
\t\t\t\t\t(uint32_t) pitch,
\t\t\t\t\t(int8_t) VREG(v->regs,voll),
\t\t\t\t\t(int8_t) VREG(v->regs,volr),
\t\t\t\t\t(m.t_non & v->vbit) != 0,
\t\t\t\t\t(m.t_eon & v->vbit) != 0 );
\t\t\t}
\t\t}
''',
        "sample phase event",
    )
    replace_once(
        path,
        '''\t// Immediate silence due to end of sample or soft reset
\tif ( REG(flg) & 0x80 || (m.t_brr_header & 3) == 1 )
\t{
\t\tv->env_mode = env_release;
\t\tv->env      = 0;
\t}
''',
        '''\t// Immediate silence due to end of sample or soft reset
\tif ( REG(flg) & 0x80 || (m.t_brr_header & 3) == 1 )
\t{
\t\tif ( instrumentation_sink && v->env_mode != env_release )
\t\t{
\t\t\tgameaudio::spc::observe_snes_spc_release_entered(
\t\t\t\tinstrumentation_sink,
\t\t\t\t(long long) instrumentation_clock,
\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t(uint8_t) (v - m.voices),
\t\t\t\t(uint32_t) v->env );
\t\t}
\t\tv->env_mode = env_release;
\t\tv->env      = 0;
\t}
''',
        "immediate release event",
    )
    replace_once(
        path,
        '''\t\t// KOFF
\t\tif ( m.t_koff & v->vbit )
\t\t\tv->env_mode = env_release;
\t\t
\t\t// KON
\t\tif ( m.kon & v->vbit )
\t\t{
\t\t\tv->kon_delay = 5;
\t\t\tv->env_mode  = env_attack;
\t\t}
''',
        '''\t\t// KOFF
\t\tif ( m.t_koff & v->vbit )
\t\t{
\t\t\tif ( instrumentation_sink && v->env_mode != env_release )
\t\t\t{
\t\t\t\tgameaudio::spc::observe_snes_spc_release_entered(
\t\t\t\t\tinstrumentation_sink,
\t\t\t\t\t(long long) instrumentation_clock,
\t\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t\t(uint8_t) (v - m.voices),
\t\t\t\t\t(uint32_t) v->env );
\t\t\t}
\t\t\tv->env_mode = env_release;
\t\t}
\t\t
\t\t// KON
\t\tif ( m.kon & v->vbit )
\t\t{
\t\t\tv->kon_delay = 5;
\t\t\tv->env_mode  = env_attack;
\t\t\tif ( instrumentation_sink )
\t\t\t{
\t\t\t\tint const voice = (int) (v - m.voices);
\t\t\t\tint const pitch = VREG(v->regs,pitchl) |
\t\t\t\t\t((VREG(v->regs,pitchh) & 0x3F) << 8);
\t\t\t\tinstrumentation_voice_inactive [voice] = false;
\t\t\t\tgameaudio::spc::observe_snes_spc_key_on_accepted(
\t\t\t\t\tinstrumentation_sink,
\t\t\t\t\t(long long) instrumentation_clock,
\t\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t\t(uint8_t) voice,
\t\t\t\t\t(uint32_t) pitch,
\t\t\t\t\t(int8_t) VREG(v->regs,voll),
\t\t\t\t\t(int8_t) VREG(v->regs,volr),
\t\t\t\t\t(m.t_non & v->vbit) != 0,
\t\t\t\t\t(m.t_eon & v->vbit) != 0 );
\t\t\t}
\t\t}
''',
        "KOFF/KON lifecycle events",
    )
    replace_once(
        path,
        '''\t// Run envelope for next sample
\tif ( !v->kon_delay )
\t\trun_envelope( v );
}
''',
        '''\t// Run envelope for next sample
\tif ( !v->kon_delay )
\t\trun_envelope( v );
\t
\tif ( instrumentation_sink )
\t{
\t\tint const voice = (int) (v - m.voices);
\t\tif ( !instrumentation_voice_inactive [voice] &&
\t\t\t\tv->env_mode == env_release && v->env == 0 )
\t\t{
\t\t\tinstrumentation_voice_inactive [voice] = true;
\t\t\tgameaudio::spc::observe_snes_spc_became_inactive(
\t\t\t\tinstrumentation_sink,
\t\t\t\t(long long) instrumentation_clock,
\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t(uint8_t) voice );
\t\t}
\t}
}
''',
        "inactive lifecycle event",
    )
    replace_once(
        path,
        '''inline void SPC_DSP::echo_write( int ch )
{
\tif ( !(m.t_echo_enabled & 0x20) )
\t\tSET_LE16A( ECHO_PTR( ch ), m.t_echo_out [ch] );
\tm.t_echo_out [ch] = 0;
}
''',
        '''inline void SPC_DSP::echo_write( int ch )
{
\tif ( !(m.t_echo_enabled & 0x20) )
\t{
\t\tuint8_t* const echo_ptr = ECHO_PTR( ch );
\t\tuint8_t const old_lo = echo_ptr [0];
\t\tuint8_t const old_hi = echo_ptr [1];
\t\tSET_LE16A( echo_ptr, m.t_echo_out [ch] );
\t\tif ( instrumentation_sink &&
\t\t\t\t(echo_ptr [0] != old_lo || echo_ptr [1] != old_hi) )
\t\t{
\t\t\tgameaudio::spc::observe_snes_spc_apuram_mutation(
\t\t\t\tinstrumentation_sink,
\t\t\t\tgameaudio::spc::spc_runtime_ram_write_origin::dsp_echo,
\t\t\t\t(long long) instrumentation_clock,
\t\t\t\tinstrumentation_tick_rate,
\t\t\t\t(uint16_t) (m.t_echo_ptr + ch * 2),
\t\t\t\techo_ptr,
\t\t\t\t2 );
\t\t}
\t}
\tm.t_echo_out [ch] = 0;
}
''',
        "DSP echo RAM observation",
    )
    replace_once(
        path,
        '''\t\t#define PHASE( n ) if ( n && !--clocks_remain ) break; case n:
\t\tGEN_DSP_TIMING
\t\t#undef PHASE
\t
\t\tif ( --clocks_remain )
\t\t\tgoto loop;
''',
        '''\t\t#define PHASE( n ) if ( n ) { ++instrumentation_clock; if ( !--clocks_remain ) break; } case n:
\t\tGEN_DSP_TIMING
\t\t#undef PHASE
\t
\t\t++instrumentation_clock;
\t\tif ( --clocks_remain )
\t\t\tgoto loop;
''',
        "DSP observation clock",
    )
    replace_once(
        path,
        '''void SPC_DSP::init( void* ram_64k )
{
\tm.ram = (uint8_t*) ram_64k;
''',
        '''void SPC_DSP::init( void* ram_64k )
{
\tinstrumentation_sink = 0;
\tinstrumentation_clock = 0;
\tinstrumentation_tick_rate = 0;
\tinstrumentation_dir_srcn = 0;
\tinstrumentation_brr_srcn = 0;
\tinstrumentation_loop_address = 0;
\tfor ( int i = 0; i < voice_count; ++i )
\t\tinstrumentation_voice_inactive [i] = true;
\tm.ram = (uint8_t*) ram_64k;
''',
        "SPC_DSP instrumentation initialization",
    )
    replace_once(
        path,
        '''void SPC_DSP::soft_reset_common()
{
''',
        '''void SPC_DSP::set_runtime_instrumentation_sink(
\t\tgameaudio::spc::spc_runtime_instrumentation_sink* sink,
\t\tunsigned long long tick_rate )
{
\tinstrumentation_sink = sink;
\tinstrumentation_clock = 0;
\tinstrumentation_tick_rate = tick_rate;
\tinstrumentation_dir_srcn = 0;
\tinstrumentation_brr_srcn = 0;
\tinstrumentation_loop_address = 0;
\tfor ( int i = 0; i < voice_count; ++i )
\t\tinstrumentation_voice_inactive [i] = true;
}

void SPC_DSP::soft_reset_common()
{
''',
        "SPC_DSP sink setter implementation",
    )


def patch(root: Path) -> None:
    required = [
        root / "snes_spc" / "SNES_SPC.h",
        root / "snes_spc" / "SNES_SPC.cpp",
        root / "snes_spc" / "SNES_SPC_misc.cpp",
        root / "snes_spc" / "SPC_DSP.h",
        root / "snes_spc" / "SPC_DSP.cpp",
    ]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise RuntimeError("missing pinned snes_spc source files: " + ", ".join(missing))

    patch_snes_spc_header(root)
    patch_dsp_header(root)
    patch_snes_spc_cpp(root)
    patch_misc_cpp(root)
    patch_dsp_cpp(root)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    patch(args.source_root.resolve())
