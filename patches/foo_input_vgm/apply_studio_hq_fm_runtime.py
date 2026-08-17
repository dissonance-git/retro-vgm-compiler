#!/usr/bin/env python3
"""Promote exact-state Studio FM through PlayerA's deferred output seam.

This patch runs after apply_enhanced_runtime.py. It keeps PlayerA as the only
owner of song gain, fade, inversion, clipping, and PCM packing. While Studio is
engaged, PlayerA may render ahead by the FIR support horizon; this component
queues whole protected WAVE_32BS frames and replaces only the six exact FM
contributions when the corresponding ordinal-tagged Studio frame becomes ready.

DAC, PSG, QSound, unrelated chips, and metadata remain inside the protected
reference frame. Existing block-local enhanced families are deliberately held at
reference while deferred transport is engaged because their outer capture layer
still has one-render-per-decode semantics. Any Studio evidence failure becomes
protected reference output, never an FM-only delay or invented history.
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

    # SourceAware's Studio observer owns every source-ordinal diagnostic. This
    # runtime consumes that public contract rather than patching observer API a
    # second time.
    replace_once(
        header,
        """#include "input_base.h"
""",
        """#include "input_base.h"
#include "studio_frame_transport.h"
""",
        "Studio whole-frame transport include",
    )

    replace_once(
        header,
        """\tbool m_enhanced_psg_block_rendered = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        """\tbool m_enhanced_psg_block_rendered = false;
\tusing studio_fm_transport_type = foobar_vgm::source_audio::studio_frame_transport<16640>;
\tstudio_fm_transport_type m_studio_fm_transport{};
\tbool m_studio_deferred_engaged = false;
\tbool m_studio_deferred_active = false;
\tbool m_studio_deferred_failed = false;
\tbool m_studio_deferred_capture_bypass = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        "Studio deferred transport state",
    )

    replace_once(
        header,
        """#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tstatic void enhanced_post_render_callback(void* user_param, WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample);
\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;
#endif
""",
        """#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tstatic void enhanced_post_render_callback(void* user_param, WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample);
\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;
#endif
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
\tstatic UINT8 studio_deferred_post_render_callback(
\t\tvoid* user_param,
\t\tconst WAVE_32BS* rendered_samples,
\t\tUINT32 rendered_count,
\t\tUINT32 rendered_base_playback_sample,
\t\tWAVE_32BS* finalized_samples,
\t\tUINT32 finalized_capacity,
\t\tUINT32 expected_finalized_base,
\t\tUINT8 source_ended,
\t\tUINT32* finalized_count);
\tUINT8 apply_studio_deferred_post_render(
\t\tconst WAVE_32BS* rendered_samples,
\t\tUINT32 rendered_count,
\t\tUINT32 rendered_base_playback_sample,
\t\tWAVE_32BS* finalized_samples,
\t\tUINT32 finalized_capacity,
\t\tUINT32 expected_finalized_base,
\t\tUINT8 source_ended,
\t\tUINT32* finalized_count) noexcept;
\tvoid fail_studio_deferred_quality() noexcept;
#endif
""",
        "Studio deferred callback declarations",
    )

    # The historical linear HQ FM and enhanced PSG remain the fallback when
    # deferred transport was never engaged. Once PlayerA has rendered ahead,
    # however, its queue owns continuity; block-local descendants must not use
    # cumulative outer captures from a different delivered clock.
    replace_once(
        shadow,
        """\tconst bool fm_ready = source_player->ym_source_expected()
\t\t&& source_player->ym_source_block_valid()
\t\t&& source_player->hq_fm_source_block_valid();
""",
        """\tconst bool fm_ready = !m_studio_deferred_engaged
\t\t&& source_player->ym_source_expected()
\t\t&& source_player->ym_source_block_valid()
\t\t&& source_player->hq_fm_source_block_valid();
""",
        "deferred FM owns replacement while engaged",
    )

    replace_once(
        shadow,
        """\tconst bool psg_ready = m_psg_present[0] && m_psg_shadow_valid[0]
\t\t&& source_player->psg_source_expected()
\t\t&& source_player->psg_source_block_valid();
""",
        """\tconst bool psg_ready = !m_studio_deferred_engaged
\t\t&& m_psg_present[0] && m_psg_shadow_valid[0]
\t\t&& source_player->psg_source_expected()
\t\t&& source_player->psg_source_block_valid();
""",
        "deferred transport protects PSG reference timing",
    )

    callback = r'''#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
void input_vgm::fail_studio_deferred_quality() noexcept
{
	m_studio_deferred_active = false;
	m_studio_deferred_failed = true;
	if (m_studio_fm_transport.valid())
		m_studio_fm_transport.fail_closed_reference();
}

UINT8 input_vgm::studio_deferred_post_render_callback(
	void* user_param,
	const WAVE_32BS* rendered_samples,
	UINT32 rendered_count,
	UINT32 rendered_base_playback_sample,
	WAVE_32BS* finalized_samples,
	UINT32 finalized_capacity,
	UINT32 expected_finalized_base,
	UINT8 source_ended,
	UINT32* finalized_count)
{
	if (user_param == nullptr || finalized_count == nullptr)
		return 1;
	return static_cast<input_vgm*>(user_param)->apply_studio_deferred_post_render(
		rendered_samples,
		rendered_count,
		rendered_base_playback_sample,
		finalized_samples,
		finalized_capacity,
		expected_finalized_base,
		source_ended,
		finalized_count);
}

UINT8 input_vgm::apply_studio_deferred_post_render(
	const WAVE_32BS* rendered_samples,
	UINT32 rendered_count,
	UINT32 rendered_base_playback_sample,
	WAVE_32BS* finalized_samples,
	UINT32 finalized_capacity,
	UINT32 expected_finalized_base,
	UINT8 source_ended,
	UINT32* finalized_count) noexcept
{
	if (finalized_count == nullptr || (finalized_capacity != 0 && finalized_samples == nullptr))
		return 1;
	*finalized_count = 0;
	if (!m_studio_deferred_engaged || !m_studio_fm_transport.valid())
		return 1;

	auto drain_final = [&]() noexcept -> bool
	{
		while (*finalized_count < finalized_capacity)
		{
			foobar_vgm::source_audio::studio_transport_output_frame output{};
			if (!m_studio_fm_transport.pop_final(output))
				break;
			const std::uint64_t expected =
				static_cast<std::uint64_t>(expected_finalized_base)
				+ static_cast<std::uint64_t>(*finalized_count);
			if (output.destination_ordinal != expected)
				return false;
			finalized_samples[*finalized_count].L = output.left;
			finalized_samples[*finalized_count].R = output.right;
			++(*finalized_count);
		}
		return true;
	};

	// Free any already-finalized prefix before accepting a newly rendered block.
	// Waiting Studio frames stay at the head and therefore cannot be skipped.
	if (!drain_final())
		return 1;

	auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
	bool studio_block = m_studio_deferred_active;
	if (rendered_count != 0)
	{
		if (rendered_samples == nullptr)
			return 1;
		if (studio_block)
		{
			const std::uint64_t rendered_end =
				static_cast<std::uint64_t>(rendered_base_playback_sample)
				+ static_cast<std::uint64_t>(rendered_count);
			studio_block = source_player != nullptr
				&& source_player->source_topology_supported()
				&& source_player->source_block_complete()
				&& source_player->source_output_count() == rendered_count
				&& source_player->ym_source_expected()
				&& source_player->ym_source_block_valid()
				&& source_player->hq_fm_source_block_valid()
				&& source_player->studio_hq_fm_observer_valid()
				&& source_player->studio_hq_fm_next_destination_ordinal() == rendered_end;
			if (!studio_block)
				fail_studio_deferred_quality();
		}

		const bool domain_started = studio_block
			&& source_player->studio_hq_fm_domain_started();
		const std::uint64_t first_studio = domain_started
			? source_player->studio_hq_fm_first_destination_ordinal()
			: std::numeric_limits<std::uint64_t>::max();

		for (UINT32 frame = 0; frame < rendered_count; ++frame)
		{
			const std::uint64_t ordinal =
				static_cast<std::uint64_t>(rendered_base_playback_sample)
				+ static_cast<std::uint64_t>(frame);
			foobar_vgm::source_audio::studio_transport_input_frame input{};
			input.destination_ordinal = ordinal;
			input.protected_left = rendered_samples[frame].L;
			input.protected_right = rendered_samples[frame].R;
			input.await_studio_fm = studio_block && domain_started && ordinal >= first_studio;

			if (input.await_studio_fm)
			{
				for (std::size_t channel = 0; channel < 6; ++channel)
				{
					const auto exact_lane = static_cast<SourceAwareVGMPlayer::source_lane>(
						static_cast<std::uint8_t>(SourceAwareVGMPlayer::source_lane::ym2612_fm1)
						+ static_cast<std::uint8_t>(channel));
					const auto* exact = source_player->source_output(exact_lane);
					if (exact == nullptr)
					{
						fail_studio_deferred_quality();
						input.await_studio_fm = false;
						input.exact_fm_left = 0;
						input.exact_fm_right = 0;
						break;
					}
					input.exact_fm_left += static_cast<std::int64_t>(exact[frame].left);
					input.exact_fm_right += static_cast<std::int64_t>(exact[frame].right);
				}
			}

			if (!m_studio_fm_transport.push(input))
				return 1;
			if (!m_studio_deferred_active)
				studio_block = false;
		}
	}

	// SourceAware has already observed every native sample from rendered_samples
	// before PlayerA invokes this callback. Ready frames may therefore include a
	// prior block's held tail and the reconstructable prefix of this block.
	if (m_studio_deferred_active && source_player != nullptr)
	{
		SourceAwareVGMPlayer::studio_hq_fm_ready_frame ready{};
		while (source_player->pop_studio_hq_fm_ready_frame(ready))
		{
			long double left = 0.0L;
			long double right = 0.0L;
			bool finite = ready.valid;
			for (std::size_t channel = 0; channel < ready.lane.size(); ++channel)
			{
				finite = finite && std::isfinite(ready.lane[channel].left)
					&& std::isfinite(ready.lane[channel].right);
				left += static_cast<long double>(ready.lane[channel].left);
				right += static_cast<long double>(ready.lane[channel].right);
			}
			const long double rounded_left = std::round(left);
			const long double rounded_right = std::round(right);
			finite = finite
				&& rounded_left >= static_cast<long double>(std::numeric_limits<std::int64_t>::min())
				&& rounded_left <= static_cast<long double>(std::numeric_limits<std::int64_t>::max())
				&& rounded_right >= static_cast<long double>(std::numeric_limits<std::int64_t>::min())
				&& rounded_right <= static_cast<long double>(std::numeric_limits<std::int64_t>::max());
			if (!finite || !m_studio_fm_transport.apply_studio_fm(
					ready.destination_ordinal,
					static_cast<std::int64_t>(rounded_left),
					static_cast<std::int64_t>(rounded_right)))
			{
				fail_studio_deferred_quality();
				break;
			}
		}
	}

	if (source_ended && m_studio_deferred_active && source_player != nullptr)
	{
		const std::size_t tail = source_player->finish_studio_hq_fm_reference_tail();
		if (!m_studio_fm_transport.finish_reference_tail(tail))
			fail_studio_deferred_quality();
	}

	if (!drain_final())
		return 1;
	return 0;
}
#endif

'''

    replace_once(
        shadow,
        """#endif

bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
""",
        """#endif

""" + callback + """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
""",
        "Studio deferred runtime callback",
    )

    replace_once(
        shadow,
        """\tm_enhanced_psg_block_rendered = false;
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tm_main_player.SetPostRenderProcessor(&input_vgm::enhanced_post_render_callback, this);
#endif
""",
        """\tm_enhanced_psg_block_rendered = false;
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tm_main_player.SetPostRenderProcessor(&input_vgm::enhanced_post_render_callback, this);
#endif
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
\tauto* studio_source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
\tconst bool studio_requested = cfg_vgm_enhanced_enabled
\t\t&& !m_studio_deferred_failed
\t\t&& studio_source_player != nullptr
\t\t&& studio_source_player->source_topology_supported()
\t\t&& studio_source_player->ym_source_expected()
\t\t&& studio_source_player->studio_hq_fm_observer_valid();
\tif (!m_studio_deferred_engaged && studio_requested)
\t{
\t\tm_studio_fm_transport.reset();
\t\tm_studio_deferred_engaged = true;
\t\tm_studio_deferred_active = true;
\t}
\tif (m_studio_deferred_engaged && (!cfg_vgm_enhanced_enabled
\t\t|| m_studio_deferred_failed || studio_source_player == nullptr
\t\t|| !studio_source_player->studio_hq_fm_observer_valid()))
\t{
\t\tm_studio_deferred_active = false;
\t\tm_studio_fm_transport.fail_closed_reference();
\t}
\tm_studio_deferred_capture_bypass = m_studio_deferred_engaged;
\tm_main_player.SetDeferredPostRenderProcessor(
\t\tm_studio_deferred_engaged ? &input_vgm::studio_deferred_post_render_callback : nullptr,
\t\tm_studio_deferred_engaged ? this : nullptr);
#else
\tm_studio_deferred_capture_bypass = false;
#endif
""",
        "Studio deferred PlayerA activation",
    )

    replace_once(
        shadow,
        """\tif (m_qsound_present && m_qsound_audio_shadow_valid && m_vgm_player != nullptr)
""",
        """\tif (!m_studio_deferred_capture_bypass
\t\t&& m_qsound_present && m_qsound_audio_shadow_valid && m_vgm_player != nullptr)
""",
        "bypass cumulative QSound source capture during render-ahead",
    )

    replace_once(
        shadow,
        """\tif (m_qsound_present && m_qsound_mix_shadow_valid && m_vgm_player != nullptr)
""",
        """\tif (!m_studio_deferred_capture_bypass
\t\t&& m_qsound_present && m_qsound_mix_shadow_valid && m_vgm_player != nullptr)
""",
        "bypass cumulative QSound mix capture during render-ahead",
    )

    replace_once(
        shadow,
        """\tm_source_capture_active = true;
""",
        """\tm_source_capture_active = !m_studio_deferred_capture_bypass;
""",
        "route render-ahead commands directly to engine-clock shadows",
    )

    replace_once(
        shadow,
        """\tif (!result)
\t\treturn false;
""",
        """\tif (!result)
\t{
\t\tif (m_studio_deferred_capture_bypass && m_vgm_player != nullptr)
\t\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
\t\treturn false;
\t}
""",
        "render-ahead shadow clock on terminal decode",
    )

    replace_once(
        shadow,
        """\tproject_qsound_consumer_sources(block_start, m_render_done);
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        """\tif (m_studio_deferred_capture_bypass)
\t{
\t\t// PlayerA may have rendered beyond m_render_done to satisfy the FIR. The
\t\t// command tap was in direct-shadow mode, so advance the continuous state
\t\t// to the actual engine clock rather than the delivered foobar clock.
\t\tif (m_vgm_player != nullptr)
\t\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
\t}
\telse
\t{
\t\tproject_qsound_consumer_sources(block_start, m_render_done);
\t\treplay_captured_sources(m_render_done);
\t}
\treturn true;
""",
        "separate rendered and delivered shadow clocks",
    )

    replace_once(
        shadow,
        """void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
""",
        """void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
\tconfigure_enhancement_shadow();
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
\tm_main_player.SetDeferredPostRenderProcessor(nullptr, nullptr);
\tm_studio_fm_transport.reset();
\tm_studio_deferred_engaged = false;
\tm_studio_deferred_active = false;
\tm_studio_deferred_failed = false;
\tm_studio_deferred_capture_bypass = false;
#endif
""",
        "Studio deferred seek boundary",
    )

    print("foo_input_vgm Studio HQ FM deferred runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())