#!/usr/bin/env python3
"""Carry source-bank YM2612 DAC reconstruction on PlayerA's engine clock.

This patch runs after the ordinary enhanced DAC block patch. libvgm's
DAC-control observer is the source of truth for bank bytes, authored stream
frequency, start/stop, stepping, reverse, and loop semantics. The component
materializes that stream into one ordinal queue shared by ordinary post-render
and deferred FM render-ahead.

Only one active stream may own primary YM2612 register $2A. Ambiguous ownership,
ordinal disagreement, missing exact subtraction authority, or arithmetic failure
falls back to the protected reference DAC family without disturbing FM or PSG.
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
    args = parser.parse_args()
    root = args.source_dir.resolve()
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
        "PCM stream bank and queue state",
    )
    replace_once(
        header,
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};\n",
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};\n"
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_pcm_source_scratch{};\n",
        "PCM source candidate scratch",
    )
    replace_once(
        header,
        "\tvoid advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;\n",
        "\tbool advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;\n",
        "PCM stream advance result",
    )
    replace_once(
        header,
        "\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;\n",
        "\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "ordinary post-render ordinal ABI",
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
        """\tm_shadow_configured = true;
\tm_shadow_replay_sample = 0;
""",
        """\tm_shadow_configured = true;
\tm_shadow_replay_sample = 0;
\treset_pcm_streams();
""",
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
	case VGMDSE_SETUP:
		mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::setup;
		break;
	case VGMDSE_SET_DATA:
		mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::set_data;
		break;
	case VGMDSE_SET_FREQUENCY:
		mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::set_frequency;
		break;
	case VGMDSE_START:
		mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::start;
		break;
	case VGMDSE_STOP:
		mapped.kind = gameaudio::vgm::dac_stream_source_event_kind::stop;
		break;
	default:
		return;
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

	// Materialize [previous,event) before changing the stream state. The event
	// itself therefore takes effect exactly at mapped.sample.
	self->advance_pcm_streams_to(static_cast<uint_fast64_t>(mapped.sample));
	self->apply_pcm_stream_event(mapped);
}
#endif

'''
    replace_once(
        shadow,
        "#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER\n",
        observer + "#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER\n",
        "DAC stream observer implementation",
    )

    replace_once(
        shadow,
        """\t\tself->m_qsound_state.reset();
""",
        """\t\tself->reset_pcm_streams();
\t\tself->m_qsound_state.reset();
""",
        "PCM stream command-reset lifecycle",
    )

    replace_once(
        shadow,
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\tif (self->m_source_capture_active)
""",
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\t// genesis_state invokes this tap before mutating YM controls. Advancing here
\t// gives the preceding interval the old DAC-enable/pan state and the next
\t// interval the new state at exactly this command ordinal.
\tself->advance_pcm_streams_to(absolute_sample);

\tif (self->m_source_capture_active)
""",
        "PCM source command-boundary advance",
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
	const std::uint64_t start = static_cast<std::uint64_t>(m_pcm_stream_replay_sample);
	const std::uint64_t delta = end - start;

	if (!m_pcm_stream_queue_capture_active || m_pcm_stream_queue_failed)
	{
		std::uint64_t remaining = delta;
		while (remaining != 0)
		{
			const std::size_t chunk = static_cast<std::size_t>(
				remaining > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
					? static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
					: remaining);
			(void)m_pcm_streams.render(nullptr, chunk);
			remaining -= static_cast<std::uint64_t>(chunk);
		}
		m_pcm_stream_replay_sample = absolute_sample;
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
		(void)m_pcm_streams.render(nullptr, static_cast<std::size_t>(delta));
		m_pcm_stream_replay_sample = absolute_sample;
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
		std::uint64_t remaining = delta;
		while (remaining != 0)
		{
			const std::size_t chunk = static_cast<std::size_t>(
				remaining > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
					? static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
					: remaining);
			(void)m_pcm_streams.render(nullptr, chunk);
			remaining -= static_cast<std::uint64_t>(chunk);
		}
		m_pcm_stream_replay_sample = absolute_sample;
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
        "PCM stream engine-clock helpers",
    )

    replace_once(
        shadow,
        """void input_vgm::enhanced_post_render_callback(
\tvoid* user_param,
\tWAVE_32BS* samples,
\tUINT32 sample_count,
\tUINT32)
{
\tif (user_param == nullptr || samples == nullptr || sample_count == 0)
\t\treturn;
\tstatic_cast<input_vgm*>(user_param)->apply_enhanced_post_render(samples, sample_count);
}

void input_vgm::apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept
""",
        """void input_vgm::enhanced_post_render_callback(
\tvoid* user_param,
\tWAVE_32BS* samples,
\tUINT32 sample_count,
\tUINT32 base_playback_sample)
{
\tif (user_param == nullptr || samples == nullptr || sample_count == 0)
\t\treturn;
\tstatic_cast<input_vgm*>(user_param)->apply_enhanced_post_render(
\t\tsamples, sample_count, base_playback_sample);
}

void input_vgm::apply_enhanced_post_render(
\tWAVE_32BS* samples,
\tUINT32 sample_count,
\tUINT32 base_playback_sample) noexcept
""",
        "ordinary post-render absolute ordinal",
    )

    ordinary = r'''	// ---- source-bank YM2612 DAC ------------------------------------------
	// The dac_control observer owns source timing. Replace only frames carrying
	// unambiguous stream ownership; all other DAC frames remain protected here.
	bool pcm_stream_changed = false;
	bool pcm_stream_block = !m_pcm_stream_queue_failed
		&& sample_count <= m_enhanced_pcm_source_scratch.size();
	const std::uint64_t pcm_rendered_end =
		static_cast<std::uint64_t>(base_playback_sample)
		+ static_cast<std::uint64_t>(sample_count);
	if (pcm_stream_block && !advance_pcm_streams_to(static_cast<uint_fast64_t>(pcm_rendered_end)))
		pcm_stream_block = false;

	if (pcm_stream_block)
	{
		const auto* exact_dac = source_player->source_output(SourceAwareVGMPlayer::source_lane::ym2612_dac);
		for (UINT32 frame = 0; frame < sample_count; ++frame)
			m_enhanced_pcm_source_scratch[frame] = m_enhanced_candidate_mix[frame];

		for (UINT32 frame = 0; frame < sample_count && pcm_stream_block; ++frame)
		{
			const std::uint64_t ordinal = static_cast<std::uint64_t>(base_playback_sample)
				+ static_cast<std::uint64_t>(frame);
			gameaudio::vgm::ym2612_pcm_source_frame enhanced{};
			if (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
			{
				pcm_stream_block = false;
				break;
			}
			if (!enhanced.replace_reference)
				continue;
			if (exact_dac == nullptr || !source_player->ym_source_expected()
				|| !source_player->ym_source_block_valid())
			{
				pcm_stream_block = false;
				break;
			}
			const std::int64_t left =
				static_cast<std::int64_t>(m_enhanced_pcm_source_scratch[frame].L)
				+ enhanced.left - static_cast<std::int64_t>(exact_dac[frame].left);
			const std::int64_t right =
				static_cast<std::int64_t>(m_enhanced_pcm_source_scratch[frame].R)
				+ enhanced.right - static_cast<std::int64_t>(exact_dac[frame].right);
			if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
				|| left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
				|| right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
				|| right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
			{
				pcm_stream_block = false;
				break;
			}
			m_enhanced_pcm_source_scratch[frame].L = static_cast<INT32>(left);
			m_enhanced_pcm_source_scratch[frame].R = static_cast<INT32>(right);
			pcm_stream_changed = true;
		}

		if (pcm_stream_block && pcm_stream_changed)
		{
			for (UINT32 frame = 0; frame < sample_count; ++frame)
				m_enhanced_candidate_mix[frame] = m_enhanced_pcm_source_scratch[frame];
			changed = true;
		}
		else if (!pcm_stream_block)
		{
			m_pcm_stream_queue.fail_closed();
			m_pcm_stream_queue_capture_active = false;
			m_pcm_stream_queue_failed = true;
			pcm_stream_changed = false;
		}
	}

'''
    replace_once(
        shadow,
        "\t// ---- YM2612 FM family -------------------------------------------------\n",
        ordinary + "\t// ---- YM2612 FM family -------------------------------------------------\n",
        "ordinary source-bank DAC replacement",
    )
    replace_once(
        shadow,
        """\tconst bool dac_ready = !m_studio_deferred_engaged && m_dac_present[0]
""",
        """\tconst bool dac_ready = !m_studio_deferred_engaged
\t\t&& !m_pcm_stream_queue_failed && !pcm_stream_changed && m_dac_present[0]
""",
        "source-bank ownership precedes direct DAC descendant",
    )

    deferred = r'''		bool deferred_pcm_block = !m_pcm_stream_queue_failed
			&& rendered_count <= m_enhanced_pcm_source_scratch.size()
			&& source_player != nullptr
			&& source_player->source_topology_supported()
			&& source_player->source_block_complete()
			&& source_player->source_output_count() == rendered_count;
		if (deferred_pcm_block && !advance_pcm_streams_to(static_cast<uint_fast64_t>(rendered_end)))
			deferred_pcm_block = false;

		bool deferred_pcm_changed = false;
		if (deferred_pcm_block)
		{
			const auto* exact_dac = source_player->source_output(SourceAwareVGMPlayer::source_lane::ym2612_dac);
			for (UINT32 frame = 0; frame < rendered_count; ++frame)
			{
				m_enhanced_pcm_source_scratch[frame] = deferred_psg_block
					? m_enhanced_family_scratch[frame] : rendered_samples[frame];
			}

			for (UINT32 frame = 0; frame < rendered_count && deferred_pcm_block; ++frame)
			{
				const std::uint64_t ordinal =
					static_cast<std::uint64_t>(rendered_base_playback_sample)
					+ static_cast<std::uint64_t>(frame);
				gameaudio::vgm::ym2612_pcm_source_frame enhanced{};
				if (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
				{
					deferred_pcm_block = false;
					break;
				}
				if (!enhanced.replace_reference)
					continue;
				if (exact_dac == nullptr || !source_player->ym_source_expected()
					|| !source_player->ym_source_block_valid())
				{
					deferred_pcm_block = false;
					break;
				}
				const std::int64_t left =
					static_cast<std::int64_t>(m_enhanced_pcm_source_scratch[frame].L)
					+ enhanced.left - static_cast<std::int64_t>(exact_dac[frame].left);
				const std::int64_t right =
					static_cast<std::int64_t>(m_enhanced_pcm_source_scratch[frame].R)
					+ enhanced.right - static_cast<std::int64_t>(exact_dac[frame].right);
				if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
					|| right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
				{
					deferred_pcm_block = false;
					break;
				}
				m_enhanced_pcm_source_scratch[frame].L = static_cast<INT32>(left);
				m_enhanced_pcm_source_scratch[frame].R = static_cast<INT32>(right);
				deferred_pcm_changed = true;
			}

			if (!deferred_pcm_block)
			{
				m_pcm_stream_queue.fail_closed();
				m_pcm_stream_queue_capture_active = false;
				m_pcm_stream_queue_failed = true;
				deferred_pcm_changed = false;
			}
		}

'''
    replace_once(
        shadow,
        """\t\t\tif (!deferred_psg_block)
\t\t\t\tfail_studio_deferred_psg();
\t\t}

\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        """\t\t\tif (!deferred_psg_block)
\t\t\t\tfail_studio_deferred_psg();
\t\t}

""" + deferred + """\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        "deferred source-bank DAC candidate block",
    )
    replace_once(
        shadow,
        """\t\t\tinput.protected_left = deferred_psg_block
\t\t\t\t? m_enhanced_family_scratch[frame].L : rendered_samples[frame].L;
\t\t\tinput.protected_right = deferred_psg_block
\t\t\t\t? m_enhanced_family_scratch[frame].R : rendered_samples[frame].R;
""",
        """\t\t\tinput.protected_left = deferred_pcm_changed
\t\t\t\t? m_enhanced_pcm_source_scratch[frame].L
\t\t\t\t: (deferred_psg_block ? m_enhanced_family_scratch[frame].L : rendered_samples[frame].L);
\t\t\tinput.protected_right = deferred_pcm_changed
\t\t\t\t? m_enhanced_pcm_source_scratch[frame].R
\t\t\t\t: (deferred_psg_block ? m_enhanced_family_scratch[frame].R : rendered_samples[frame].R);
""",
        "compose DAC before deferred FM transport",
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
        "PCM source decode-block capture start",
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
        "PCM source exception capture stop",
    )
    replace_once(
        shadow,
        """\tm_source_capture_active = false;
\tm_qsound_audio_capture_active = false;
\tm_qsound_mix_capture_active = false;
""",
        """\tm_pcm_stream_queue_capture_active = false;
\tm_source_capture_active = false;
\tm_qsound_audio_capture_active = false;
\tm_qsound_mix_capture_active = false;
""",
        "PCM source normal capture stop",
    )
    replace_once(
        shadow,
        """\tif (!result)
\t\treturn false;
""",
        """\tif (m_pcm_stream_queue.size() != 0)
\t{
\t\t// A registered enhanced callback should consume every captured engine
\t\t// ordinal. Leftovers mean the source topology/callback contract was not
\t\t// available for this block, so keep this DAC family on reference.
\t\tm_pcm_stream_queue.fail_closed();
\t\tm_pcm_stream_queue_failed = true;
\t}

\tif (!result)
\t\treturn false;
""",
        "PCM source unconsumed-block fail close",
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
        "PCM source seek reset",
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
        "PCM source seek rebase",
    )

    print("foo_input_vgm source-bank enhanced DAC runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
