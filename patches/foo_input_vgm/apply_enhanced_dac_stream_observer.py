#!/usr/bin/env python3
"""Bridge libvgm DAC-control source events onto one engine-clock PCM queue."""

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
    parser.add_argument("source_dir", type=Path)
    root = parser.parse_args().source_dir.resolve()
    header = root / "input_vgm.h"
    player_cpp = root / "input_vgm.cpp"
    shadow = root / "input_vgm_shadow.cpp"

    replace_once(
        header,
        '#include "../../enhancement/ym2612_pcm_stream.h"\n',
        '#include "../../enhancement/ym2612_pcm_source_queue.h"\n',
        "PCM source queue include",
    )
    replace_once(
        header,
        "\tstd::array<gameaudio::vgm::ym2612_pcm_stream, 256> m_pcm_streams;\n",
        "\tgameaudio::vgm::ym2612_pcm_stream_bank m_pcm_streams{};\n"
        "\tusing pcm_source_queue_type = gameaudio::vgm::ym2612_pcm_source_queue<16640>;\n"
        "\tpcm_source_queue_type m_pcm_stream_queue{};\n"
        "\tbool m_pcm_stream_queue_capture_active = false;\n"
        "\tbool m_pcm_stream_queue_failed = false;\n",
        "PCM stream queue state",
    )
    replace_once(
        header,
        "\tvoid advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;\n",
        "\tbool advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;\n",
        "PCM stream advance result",
    )

    replace_once(
        player_cpp,
        """#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tif (m_vgm_player != nullptr)
\t\tm_vgm_player->SetCommandObserver(nullptr, nullptr);
#endif
""",
        """#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tif (m_vgm_player != nullptr)
\t\tm_vgm_player->SetCommandObserver(nullptr, nullptr);
#endif
#ifdef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
\tif (m_vgm_player != nullptr)
\t\tm_vgm_player->SetDACStreamSourceObserver(nullptr, nullptr);
#endif
""",
        "DAC stream observer teardown",
    )
    replace_once(
        player_cpp,
        """#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tm_vgm_player->SetCommandObserver(&input_vgm::command_observer_callback, this);
#endif
\tm_main_player.RegisterPlayerEngine(m_vgm_player);
""",
        """#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
\tm_vgm_player->SetCommandObserver(&input_vgm::command_observer_callback, this);
#endif
#ifdef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
\tm_vgm_player->SetDACStreamSourceObserver(&input_vgm::dac_stream_source_callback, this);
#endif
\tm_main_player.RegisterPlayerEngine(m_vgm_player);
""",
        "DAC stream observer registration",
    )

    replace_once(
        shadow,
        "\tm_shadow_configured = true;\n\tm_shadow_replay_sample = 0;\n",
        "\tm_shadow_configured = true;\n\tm_shadow_replay_sample = 0;\n\treset_pcm_streams();\n",
        "PCM stream initial reset",
    )

    observer = r'''#ifdef LIBVGM_GAMEAUDIO_DAC_STREAM_OBSERVER
void input_vgm::dac_stream_source_callback(void* user_param, const VGM_DAC_STREAM_SOURCE_EVENT* event)
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (self == nullptr || event == nullptr || self->m_vgm_player == nullptr)
		return;

	gameaudio::vgm::dac_stream_source_event mapped{};
	switch (event->type)
	{
	case VGMDSE_SETUP: mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::setup; break;
	case VGMDSE_SET_DATA: mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::set_data; break;
	case VGMDSE_SET_FREQUENCY: mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::set_frequency; break;
	case VGMDSE_START: mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::start; break;
	case VGMDSE_STOP: mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::stop; break;
	default: return;
	}

	mapped.sample = static_cast<std::uint64_t>(
		self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event->tick)));
	mapped.stream_id = event->streamID;
	mapped.chip_type = event->chipType;
	mapped.chip_id = event->chipID;
	mapped.chip_command = event->chipCommand;
	mapped.bank_id = event->bankID;
	mapped.step_size = event->stepSize;
	mapped.step_base = event->stepBase;
	mapped.play_mode = event->playMode;
	mapped.frequency = event->frequency;
	mapped.start_offset = event->startOffset;
	mapped.length = event->length;
	mapped.data = event->data;
	mapped.data_length = static_cast<std::size_t>(event->dataLen);

	// Advance [previous,event) first. The observer event itself takes effect at
	// mapped.sample and therefore cannot leak backward by one host frame.
	(void)self->advance_pcm_streams_to(static_cast<uint_fast64_t>(mapped.sample));
	self->apply_pcm_stream_event(mapped);
}
#endif

'''
    replace_once(
        shadow,
        """#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
void input_vgm::qsound_source_callback""",
        observer + """#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
void input_vgm::qsound_source_callback""",
        "DAC stream observer callback",
    )

    replace_once(
        shadow,
        "\t\tself->m_qsound_state.reset();\n",
        "\t\tself->reset_pcm_streams();\n\t\tself->m_qsound_state.reset();\n",
        "PCM command-reset lifecycle",
    )
    replace_once(
        shadow,
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\tif (self->m_source_capture_active)
""",
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\t// genesis_state calls this tap before mutating YM controls, so the preceding
\t// interval sees the old DAC-enable/pan state and the next interval sees the
\t// new state at this exact command ordinal.
\t(void)self->advance_pcm_streams_to(absolute_sample);

\tif (self->m_source_capture_active)
""",
        "PCM command-boundary advance",
    )

    helpers = r'''void input_vgm::reset_pcm_streams() noexcept
{
	m_pcm_streams.configure_output_rate(static_cast<double>(m_sample_rate));
	m_pcm_streams.reset();
	m_pcm_stream_replay_sample = 0;
	m_pcm_stream_queue.reset(0);
	m_pcm_stream_queue_capture_active = false;
	m_pcm_stream_queue_failed = false;
}

void input_vgm::apply_pcm_stream_event(const gameaudio::vgm::dac_stream_source_event& event) noexcept
{
	m_pcm_streams.apply(event);
}

bool input_vgm::advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept
{
	if (absolute_sample < m_pcm_stream_replay_sample)
	{
		m_pcm_stream_queue.fail_closed();
		m_pcm_stream_queue_capture_active = false;
		m_pcm_stream_queue_failed = true;
		return false;
	}
	if (absolute_sample == m_pcm_stream_replay_sample)
		return !m_pcm_stream_queue_failed;

	const std::uint64_t end = static_cast<std::uint64_t>(absolute_sample);
	const std::uint64_t delta = end - static_cast<std::uint64_t>(m_pcm_stream_replay_sample);
	auto discard_interval = [&]() noexcept
	{
		std::uint64_t remaining = delta;
		while (remaining != 0)
		{
			const std::uint64_t ceiling = static_cast<std::uint64_t>(
				std::numeric_limits<std::size_t>::max());
			const std::size_t chunk = static_cast<std::size_t>(remaining > ceiling ? ceiling : remaining);
			(void)m_pcm_streams.render(nullptr, chunk);
			remaining -= static_cast<std::uint64_t>(chunk);
		}
		m_pcm_stream_replay_sample = absolute_sample;
	};

	if (!m_pcm_stream_queue_capture_active || m_pcm_stream_queue_failed)
	{
		discard_interval();
		return !m_pcm_stream_queue_failed;
	}

	auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
	INT16 volume_left = 0;
	INT16 volume_right = 0;
	const auto& ym = m_genesis_state.ym2612(0);
	const auto& dac_pan = ym.channels[5];
	const bool needs_volume = ym.dac_enabled && m_pcm_streams.active_target_count() != 0;
	if (needs_volume && (source_player == nullptr
		|| !source_player->ym_source_volume(volume_left, volume_right)))
	{
		m_pcm_stream_queue.fail_closed();
		m_pcm_stream_queue_capture_active = false;
		m_pcm_stream_queue_failed = true;
		discard_interval();
		return false;
	}

	if (!m_pcm_stream_queue.render_until(
		m_pcm_streams,
		end,
		ym.dac_enabled,
		dac_pan.pan_left,
		dac_pan.pan_right,
		volume_left,
		volume_right))
	{
		m_pcm_stream_queue_capture_active = false;
		m_pcm_stream_queue_failed = true;
		discard_interval();
		return false;
	}

	m_pcm_stream_replay_sample = absolute_sample;
	return true;
}

'''
    replace_once(
        shadow,
        "void input_vgm::advance_shadow_to(uint_fast64_t absolute_sample) noexcept\n",
        helpers + "void input_vgm::advance_shadow_to(uint_fast64_t absolute_sample) noexcept\n",
        "PCM stream clock helpers",
    )

    replace_once(
        shadow,
        """\tconst uint_fast64_t block_start = m_vgm_player != nullptr
\t\t? static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE))
\t\t: m_played_sample;
\tadvance_shadow_to(block_start);
""",
        """\tconst uint_fast64_t block_start = m_vgm_player != nullptr
\t\t? static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE))
\t\t: m_played_sample;
\tadvance_shadow_to(block_start);
\t(void)advance_pcm_streams_to(block_start);
\tif (!m_pcm_stream_queue_failed)
\t{
\t\tif (m_pcm_stream_queue.size() != 0)
\t\t{
\t\t\tm_pcm_stream_queue.fail_closed();
\t\t\tm_pcm_stream_queue_failed = true;
\t\t}
\t\telse
\t\t{
\t\t\tm_pcm_stream_queue.reset(static_cast<std::uint64_t>(block_start));
#if defined(FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI)
\t\t\tm_pcm_stream_queue_capture_active = cfg_vgm_enhanced_enabled != 0;
#else
\t\t\tm_pcm_stream_queue_capture_active = false;
#endif
\t\t}
\t}
""",
        "PCM decode-block capture start",
    )
    replace_once(
        shadow,
        """\tcatch (...)
\t{
\t\tm_source_capture_active = false;
""",
        """\tcatch (...)
\t{
\t\tm_pcm_stream_queue_capture_active = false;
\t\tm_source_capture_active = false;
""",
        "PCM exception capture stop",
    )
    replace_once(
        shadow,
        """\t\tthrow;
\t}
\tm_source_capture_active = false;
\tm_qsound_audio_capture_active = false;
""",
        """\t\tthrow;
\t}
\tm_pcm_stream_queue_capture_active = false;
\tm_source_capture_active = false;
\tm_qsound_audio_capture_active = false;
""",
        "PCM normal capture stop",
    )
    replace_once(
        shadow,
        """\tif (!result)
\t\treturn false;
""",
        """\tif (m_pcm_stream_queue.size() != 0)
\t{
\t\t// The post-render consumer must account for every captured engine frame.
\t\t// Leftovers mean this DAC family lacks subtraction/timing authority.
\t\tm_pcm_stream_queue.fail_closed();
\t\tm_pcm_stream_queue_failed = true;
\t}

\tif (!result)
\t\treturn false;
""",
        "PCM unconsumed-block fail close",
    )
    replace_once(
        shadow,
        """void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
\tm_source_capture_active = false;
""",
        """void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
\treset_pcm_streams();
\tm_source_capture_active = false;
""",
        "PCM seek reset",
    )
    replace_once(
        shadow,
        """\tif (m_vgm_player != nullptr)
\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
}
""",
        """\tif (m_vgm_player != nullptr)
\t{
\t\tconst uint_fast64_t seek_sample =
\t\t\tstatic_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE));
\t\tadvance_shadow_to(seek_sample);
\t\t(void)advance_pcm_streams_to(seek_sample);
\t\tm_pcm_stream_queue.reset(static_cast<std::uint64_t>(seek_sample));
\t}
}
""",
        "PCM seek rebase",
    )

    print("foo_input_vgm source-bank DAC observer bridge applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
