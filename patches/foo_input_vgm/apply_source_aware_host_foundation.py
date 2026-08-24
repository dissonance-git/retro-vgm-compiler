#!/usr/bin/env python3
"""Recreate the guarded source-aware host foundation on pristine foo_input_vgm 0.31.

The early Genesis enhancement work originally accumulated a small amount of host
state directly in input_vgm.{h,cpp}. Later guarded patches legitimately use that
state as their input contract. The 0.31 materializer must not copy the old host
snapshot, so this patch reconstructs only that additive foundation over the exact
0.31 files before the higher-level transformations run.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    has_utf8_bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", has_utf8_bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, has_utf8_bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if has_utf8_bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    root = parser.parse_args().source_dir.resolve()
    header = root / "input_vgm.h"
    player_cpp = root / "input_vgm.cpp"

    replace_once(
        header,
        '#include "input_base.h"\n',
        '''#include "input_base.h"
#include "../../enhancement/dac_stream_source.h"
#include "../../enhancement/genesis_state.h"
#include "../../enhancement/psg_block_capture.h"
#include "../../enhancement/qsound_block_capture.h"
#include "../../enhancement/qsound_consumer_source_storage.h"
#include "../../enhancement/qsound_control_state.h"
#include "../../enhancement/qsound_native_mix_capture.h"
#include "../../enhancement/qsound_native_source_capture.h"
#include "../../enhancement/qsound_native_source_window.h"
#include "../../enhancement/qsound_native_time_map.h"
#include "../../enhancement/qsound_spatial_source_bus.h"
#include "../../enhancement/sn76489_enhanced.h"
#include "../../enhancement/ym2612_dac_block_capture.h"
#include "../../enhancement/ym2612_dac_enhanced.h"
#include "../../enhancement/ym2612_pcm_stream.h"
''',
        "source-aware host includes",
    )

    replace_once(
        header,
        '''\tbool\tm_vgz;

protected:
''',
        '''\tbool\tm_vgz;

\tgameaudio::vgm::genesis_state m_genesis_state;
\tgameaudio::vgm::psg_block_capture m_psg_capture;
\tgameaudio::vgm::ym2612_dac_block_capture m_dac_capture;
\tgameaudio::vgm::qsound_block_capture m_qsound_capture;
\tgameaudio::vgm::qsound_control_state m_qsound_state;
\tgameaudio::vgm::qsound_native_source_capture m_qsound_audio_capture;
\tgameaudio::vgm::qsound_native_mix_capture m_qsound_mix_capture;
\tgameaudio::vgm::qsound_native_time_map m_qsound_consumer_time_map;
\tgameaudio::vgm::qsound_native_source_window m_qsound_consumer_source_window;
\tgameaudio::vgm::qsound_consumer_source_storage m_qsound_consumer_source_storage;
\tgameaudio::vgm::qsound_spatial_source_bus_storage m_qsound_spatial_source_bus;
\tstd::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
\tstd::array<gameaudio::vgm::ym2612_pcm_stream, 256> m_pcm_streams;

\tstd::array<bool, 2> m_psg_present{{false, false}};
\tstd::array<bool, 2> m_psg_config_supported{{false, false}};
\tstd::array<bool, 2> m_psg_shadow_valid{{false, false}};
\tstd::array<bool, 2> m_dac_present{{false, false}};
\tstd::array<bool, 2> m_dac_shadow_valid{{false, false}};
\tbool m_qsound_present = false;
\tbool m_qsound_shadow_valid = false;
\tbool m_qsound_audio_shadow_valid = false;
\tbool m_qsound_audio_capture_active = false;
\tbool m_qsound_mix_shadow_valid = false;
\tbool m_qsound_mix_capture_active = false;
\tbool m_qsound_consumer_source_configured = false;
\tbool m_qsound_consumer_source_shadow_valid = false;

\tbool m_source_capture_active = false;
\tbool m_shadow_configured = false;
\tuint_fast64_t m_shadow_replay_sample = 0;
\tuint_fast64_t m_pcm_stream_replay_sample = 0;

protected:
''',
        "source-aware host state",
    )

    replace_once(
        header,
        '''\tstatic bool g_is_our_path(const char *p_path, const char *p_extension);

private:
\tvoid guess_track_number_tag_from_file_name(file_info& p_info);
''',
        '''\tstatic bool g_is_our_path(const char *p_path, const char *p_extension);
\tbool decode_run(audio_chunk &p_chunk, abort_callback &p_abort);
\tvoid decode_seek(double p_seconds, abort_callback &p_abort);

private:
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tstatic void command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event);
#endif
#ifdef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
\tstatic void dac_stream_source_callback(void* user_param, const VGM_DAC_STREAM_SOURCE_EVENT* event);
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
\tstatic void qsound_source_callback(void* user_param, const VGM_QSOUND_SOURCE_FRAME* event);
#endif
#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
\tstatic void qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event);
#endif
\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
\tvoid configure_enhancement_shadow();
\tvoid apply_source_event_outside_render(const gameaudio::vgm::command_event& event) noexcept;
\tvoid apply_pcm_stream_event(const gameaudio::vgm::dac_stream_source_event& event) noexcept;
\tvoid advance_shadow_to(uint_fast64_t absolute_sample) noexcept;
\tvoid advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;
\tvoid reset_pcm_streams() noexcept;
\tvoid replay_captured_sources(uint_fast32_t rendered_samples) noexcept;
\tvoid reset_qsound_consumer_source_path(bool source_observer_available) noexcept;
\tvoid project_qsound_consumer_sources(uint_fast64_t block_start, uint_fast32_t rendered_samples) noexcept;
#ifndef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
\tvoid invalidate_unobserved_dac_stream(const gameaudio::vgm::command_event& event) noexcept;
#endif
\tvoid guess_track_number_tag_from_file_name(file_info& p_info);
''',
        "source-aware host declarations",
    )

    replace_once(
        header,
        "class input_vgm : public input_base\n",
        "class SourceAwareVGMPlayer;\n\nclass input_vgm : public input_base\n",
        "source-aware player forward declaration",
    )

    replace_once(
        player_cpp,
        '''input_vgm::~input_vgm()
{
}
''',
        '''input_vgm::~input_vgm()
{
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tif (m_vgm_player != nullptr)
\t\tm_vgm_player->SetCommandObserver(nullptr, nullptr);
#endif
}
''',
        "command observer teardown",
    )

    callback = '''#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
void input_vgm::command_observer_callback(void* user_param, const VGM_COMMAND_OBSERVER_EVENT* event)
{
\tif (user_param == nullptr || event == nullptr)
\t\treturn;

\tinput_vgm* self = static_cast<input_vgm*>(user_param);
\tgameaudio::vgm::command_event mapped;
\tmapped.tick = event->tick;
\tmapped.file_offset = event->filePos;
\tmapped.command = event->command;
\tmapped.payload = event->payload;
\tmapped.payload_size = event->payloadLen;

\tswitch (event->type)
\t{
\tcase VGMCOE_RESET:
\t\tmapped.kind = gameaudio::vgm::command_event_kind::reset;
\t\tbreak;
\tcase VGMCOE_YM2612_DAC:
\t\tmapped.kind = gameaudio::vgm::command_event_kind::ym2612_dac;
\t\tbreak;
\tcase VGMCOE_COMMAND:
\tdefault:
\t\tmapped.kind = gameaudio::vgm::command_event_kind::command;
\t\tbreak;
\t}

\tself->m_genesis_state.observe(mapped);
}
#endif

'''
    replace_once(
        player_cpp,
        'void input_vgm::register_player()\n',
        callback + 'void input_vgm::register_player()\n',
        "command observer callback",
    )

    replace_once(
        player_cpp,
        '''void input_vgm::register_player()
{
\tm_vgm_player = new VGMPlayer;
\tm_main_player.RegisterPlayerEngine(m_vgm_player);
}
''',
        '''void input_vgm::register_player()
{
\tm_vgm_player = new VGMPlayer;
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tm_vgm_player->SetCommandObserver(&input_vgm::command_observer_callback, this);
#endif
\tm_main_player.RegisterPlayerEngine(m_vgm_player);
}
''',
        "command observer registration",
    )

    print("foo_input_vgm 0.31 source-aware host foundation applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
