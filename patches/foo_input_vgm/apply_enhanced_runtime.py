#!/usr/bin/env python3
"""Wire source-native enhanced VGM replacement into the foobar shell.

Prerequisites are composed by apply_enhanced_component.py:
  * guarded libvgm source capture + PlayerA pre-volume hook;
  * the independent enhanced preference;
  * SourceAwareVGMPlayer selection;
  * exact-state Nuked OPN2 HQ FM lift.

Admitted audible families:
  * six YM2612 FM channels: exact reference source -> exact-state HQ carrier lift;
  * primary default-MAME SN76489/96: exact source -> enhanced PSG descendant.

DAC and all unrelated chips remain the protected libvgm reference. Each source
family is transactional: a failed candidate leaves that family unchanged while a
separately valid family may still be enhanced in the same block.
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
    shadow = root / "input_vgm_shadow.cpp"
    source_player = root / "source_aware_vgm_player.h"
    cfg_external = root / "my_cfg_external.h"

    replace_once(
        cfg_external,
        """extern cfg_int cfg_vgm_enhanced_enabled;
""",
        """extern cfg_int cfg_vgm_enhanced_enabled;
#define FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI 1
""",
        "enhanced UI ABI tag",
    )

    # The HQ-lift patch has already inserted hq_fm_source_output between the
    # exact source view and protected section. Add only the PSG device-volume
    # coordinate needed by the independently synthesized PSG descendant.
    replace_once(
        source_player,
        """    const stereo_sample* hq_fm_source_output(std::size_t channel) const noexcept
    {
        return channel < kHqFmLaneCount ? m_hq_fm_output[channel].data() : nullptr;
    }

protected:
""",
        """    const stereo_sample* hq_fm_source_output(std::size_t channel) const noexcept
    {
        return channel < kHqFmLaneCount ? m_hq_fm_output[channel].data() : nullptr;
    }

    bool psg_source_volume(INT16& left, INT16& right) const noexcept
    {
        if (!m_psg.attached || m_psg.resampler == nullptr)
            return false;
        left = m_psg.resampler->volumeL;
        right = m_psg.resampler->volumeR;
        return true;
    }

protected:
""",
        "source-aware PSG device-volume view",
    )

    replace_once(
        header,
        """#include "../../enhancement/sn76489_enhanced.h"
""",
        """#include "../../enhancement/sn76489_enhanced.h"
#include "../../enhancement/sn76489_enhanced_source_block.h"
""",
        "enhanced PSG source-block include",
    )

    replace_once(
        header,
        """\tstd::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        """\tstd::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
\tgameaudio::vgm::sn76489_enhanced_source_block_storage<8192> m_enhanced_psg_source_block;
\tstd::array<WAVE_32BS, 8192> m_enhanced_candidate_mix{};
\tstd::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};
\tbool m_enhanced_psg_block_rendered = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        "enhanced source replacement storage",
    )

    replace_once(
        header,
        """#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
\tstatic void qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event);
#endif
\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
""",
        """#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
\tstatic void qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event);
#endif
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tstatic void enhanced_post_render_callback(void* user_param, WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample);
\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;
#endif
\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
""",
        "enhanced PlayerA callback declarations",
    )

    replace_once(
        shadow,
        """#include <emu/cores/sn764intf.h>
#include <limits>
""",
        """#include <emu/cores/sn764intf.h>
#include <cmath>
#include <cstdint>
#include <limits>
""",
        "enhanced arithmetic includes",
    )

    callback = r'''#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
void input_vgm::enhanced_post_render_callback(
	void* user_param,
	WAVE_32BS* samples,
	UINT32 sample_count,
	UINT32)
{
	if (user_param == nullptr || samples == nullptr || sample_count == 0)
		return;
	static_cast<input_vgm*>(user_param)->apply_enhanced_post_render(samples, sample_count);
}

void input_vgm::apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept
{
#if defined(FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI)
	if (!cfg_vgm_enhanced_enabled || samples == nullptr || sample_count == 0
		|| sample_count > m_enhanced_candidate_mix.size() || !m_shadow_configured)
		return;

	auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
	if (source_player == nullptr || !source_player->source_topology_supported()
		|| !source_player->source_block_complete()
		|| source_player->source_output_count() != sample_count)
		return;

	for (UINT32 frame = 0; frame < sample_count; ++frame)
		m_enhanced_candidate_mix[frame] = samples[frame];
	bool changed = false;

	// ---- YM2612 FM family -------------------------------------------------
	// Exact reference and HQ lift are generated from the same authoritative
	// Nuked state and pass through the same outer libvgm timing/volume domain.
	// Replace all six FM identities together or none of them. DAC is not part of
	// this loop; when DAC owns channel 6's bus slot, both FM6 lanes are zero.
	const bool fm_ready = source_player->ym_source_expected()
		&& source_player->ym_source_block_valid()
		&& source_player->hq_fm_source_block_valid();
	if (fm_ready)
	{
		for (UINT32 frame = 0; frame < sample_count; ++frame)
			m_enhanced_family_scratch[frame] = m_enhanced_candidate_mix[frame];

		bool fm_valid = true;
		for (UINT32 frame = 0; frame < sample_count && fm_valid; ++frame)
		{
			std::int64_t left = m_enhanced_family_scratch[frame].L;
			std::int64_t right = m_enhanced_family_scratch[frame].R;
			for (std::size_t channel = 0; channel < 6; ++channel)
			{
				const auto exact_lane = static_cast<SourceAwareVGMPlayer::source_lane>(
					static_cast<std::uint8_t>(SourceAwareVGMPlayer::source_lane::ym2612_fm1)
					+ static_cast<std::uint8_t>(channel));
				const auto* exact = source_player->source_output(exact_lane);
				const auto* hq = source_player->hq_fm_source_output(channel);
				if (exact == nullptr || hq == nullptr)
				{
					fm_valid = false;
					break;
				}
				left += static_cast<std::int64_t>(hq[frame].left)
					- static_cast<std::int64_t>(exact[frame].left);
				right += static_cast<std::int64_t>(hq[frame].right)
					- static_cast<std::int64_t>(exact[frame].right);
			}

			if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
				|| left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
				|| right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
				|| right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
			{
				fm_valid = false;
				break;
			}
			m_enhanced_family_scratch[frame].L = static_cast<INT32>(left);
			m_enhanced_family_scratch[frame].R = static_cast<INT32>(right);
		}

		if (fm_valid)
		{
			for (UINT32 frame = 0; frame < sample_count; ++frame)
				m_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
			changed = true;
		}
	}

	// ---- SN76489/96 family ------------------------------------------------
	// The PSG has its own higher-quality descendant rather than a Nuked state
	// lift. Render on a copy so a failed block cannot advance live shadow state.
	const bool psg_ready = m_psg_present[0] && m_psg_shadow_valid[0]
		&& source_player->psg_source_expected()
		&& source_player->psg_source_block_valid();
	if (psg_ready)
	{
		INT16 device_volume_left = 0;
		INT16 device_volume_right = 0;
		auto candidate_psg = m_enhanced_psg[0];
		if (source_player->psg_source_volume(device_volume_left, device_volume_right)
			&& m_enhanced_psg_source_block.render(
				candidate_psg,
				m_psg_capture,
				0,
				static_cast<std::size_t>(sample_count),
				device_volume_left,
				device_volume_right))
		{
			for (UINT32 frame = 0; frame < sample_count; ++frame)
				m_enhanced_family_scratch[frame] = m_enhanced_candidate_mix[frame];

			bool psg_valid = true;
			for (UINT32 frame = 0; frame < sample_count && psg_valid; ++frame)
			{
				std::int64_t left = m_enhanced_family_scratch[frame].L;
				std::int64_t right = m_enhanced_family_scratch[frame].R;
				for (std::size_t channel = 0;
					channel < gameaudio::vgm::sn76489_enhanced::stem_count;
					++channel)
				{
					const auto exact_lane = static_cast<SourceAwareVGMPlayer::source_lane>(
						static_cast<std::uint8_t>(SourceAwareVGMPlayer::source_lane::sn76489_tone0)
						+ static_cast<std::uint8_t>(channel));
					const auto* exact = source_player->source_output(exact_lane);
					const auto enhanced = m_enhanced_psg_source_block.source(channel);
					if (exact == nullptr || enhanced.left == nullptr || enhanced.right == nullptr)
					{
						psg_valid = false;
						break;
					}
					const double enhanced_left = enhanced.left[frame];
					const double enhanced_right = enhanced.right[frame];
					if (!std::isfinite(enhanced_left) || !std::isfinite(enhanced_right))
					{
						psg_valid = false;
						break;
					}
					left += static_cast<std::int64_t>(std::llround(enhanced_left))
						- static_cast<std::int64_t>(exact[frame].left);
					right += static_cast<std::int64_t>(std::llround(enhanced_right))
						- static_cast<std::int64_t>(exact[frame].right);
				}
				if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
					|| right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
				{
					psg_valid = false;
					break;
				}
				m_enhanced_family_scratch[frame].L = static_cast<INT32>(left);
				m_enhanced_family_scratch[frame].R = static_cast<INT32>(right);
			}

			if (psg_valid)
			{
				for (UINT32 frame = 0; frame < sample_count; ++frame)
					m_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
				m_enhanced_psg[0] = candidate_psg;
				m_enhanced_psg_block_rendered = true;
				changed = true;
			}
		}
	}

	if (changed)
	{
		for (UINT32 frame = 0; frame < sample_count; ++frame)
			samples[frame] = m_enhanced_candidate_mix[frame];
	}
#else
	(void)samples;
	(void)sample_count;
#endif
}
#endif

'''

    replace_once(
        shadow,
        """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
""",
        callback + """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
""",
        "enhanced PlayerA source replacement",
    )

    replace_once(
        shadow,
        """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
""",
        """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
\tm_enhanced_psg_block_rendered = false;
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tm_main_player.SetPostRenderProcessor(&input_vgm::enhanced_post_render_callback, this);
#endif
""",
        "PlayerA enhanced callback activation",
    )

    replace_once(
        shadow,
        """void input_vgm::replay_captured_sources(uint_fast32_t rendered_samples) noexcept
{
\tfor (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
\t{
\t\tif (m_psg_present[instance] && m_psg_shadow_valid[instance])
""",
        """void input_vgm::replay_captured_sources(uint_fast32_t rendered_samples) noexcept
{
\tfor (size_t instance = 0; instance < m_enhanced_psg.size(); ++instance)
\t{
\t\tif (m_psg_present[instance] && m_psg_shadow_valid[instance]
\t\t\t&& !(instance == 0 && m_enhanced_psg_block_rendered))
""",
        "avoid double-advancing enhanced PSG state",
    )

    print("foo_input_vgm enhanced six-channel FM + PSG runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())