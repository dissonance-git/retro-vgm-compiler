#!/usr/bin/env python3
"""Compose source-bank YM2612 DAC frames before ordinary or deferred FM output."""

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
    shadow = root / "input_vgm_shadow.cpp"

    replace_once(
        header,
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};\n",
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};\n"
        "\tstd::array<WAVE_32BS, 8192> m_enhanced_pcm_source_scratch{};\n",
        "PCM source mix scratch",
    )
    replace_once(
        header,
        "\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;\n",
        "\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "ordinary post-render ordinal ABI",
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
        "ordinary source-bank DAC ordinal input",
    )

    ordinary = r'''	// ---- source-bank YM2612 DAC ------------------------------------------
	// The observer queue has already resolved the authored source stream onto
	// PlayerA's exact host ordinals. Subtract the protected DAC lane only on
	// frames carrying explicit stream ownership.
	bool pcm_stream_changed = false;
	bool pcm_stream_block = !m_pcm_stream_queue_failed
		&& sample_count <= m_enhanced_pcm_source_scratch.size();
	const std::uint64_t pcm_rendered_end =
		static_cast<std::uint64_t>(base_playback_sample)
		+ static_cast<std::uint64_t>(sample_count);
	if (pcm_stream_block
		&& !advance_pcm_streams_to(static_cast<uint_fast64_t>(pcm_rendered_end)))
		pcm_stream_block = false;

	if (pcm_stream_block)
	{
		const auto* exact_dac =
			source_player->source_output(SourceAwareVGMPlayer::source_lane::ym2612_dac);
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
        "ordinary source-bank DAC mix",
    )
    replace_once(
        shadow,
        "\tconst bool dac_ready = !m_studio_deferred_engaged && m_dac_present[0]\n",
        "\tconst bool dac_ready = !m_studio_deferred_engaged\n"
        "\t\t&& !m_pcm_stream_queue_failed && !pcm_stream_changed && m_dac_present[0]\n",
        "source-bank ownership before direct DAC",
    )

    deferred = r'''		bool deferred_pcm_block = !m_pcm_stream_queue_failed
			&& rendered_count <= m_enhanced_pcm_source_scratch.size()
			&& source_player != nullptr
			&& source_player->source_topology_supported()
			&& source_player->source_output_count() == rendered_count;
		const std::uint64_t pcm_rendered_end =
			static_cast<std::uint64_t>(rendered_base_playback_sample)
			+ static_cast<std::uint64_t>(rendered_count);
		if (deferred_pcm_block
			&& !advance_pcm_streams_to(static_cast<uint_fast64_t>(pcm_rendered_end)))
			deferred_pcm_block = false;

		bool deferred_pcm_changed = false;
		if (deferred_pcm_block)
		{
			const auto* exact_dac =
				source_player->source_output(SourceAwareVGMPlayer::source_lane::ym2612_dac);
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
        "deferred source-bank DAC mix",
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
        "source-bank DAC before deferred FM transport",
    )

    print("foo_input_vgm source-bank DAC mix applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
