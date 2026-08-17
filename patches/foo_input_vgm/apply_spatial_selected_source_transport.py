#!/usr/bin/env python3
"""Mirror the audible Genesis source choices into an ordinal source transport.

This patch runs after every current Enhanced family has been composed. It does
not render Spatial audio and does not inspect the Spatial preference. Its only
job is to make the already-selected FM/PSG/DAC realization available at the
same PlayerA output ordinal as the audible whole-frame transport.

Reference source frames are captured even when Enhanced is off. Each successful
family-local Enhanced transaction then replaces only that family's source lanes.
A failed family transaction leaves its reference lanes untouched, exactly like
the audible stereo path.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    root = parser.parse_args().source_dir.resolve()
    header = root / "input_vgm.h"
    shadow = root / "input_vgm_shadow.cpp"

    replace_once(
        header,
        '#include "../../enhancement/sn76489_deferred_source_queue.h"\n',
        '#include "../../enhancement/sn76489_deferred_source_queue.h"\n'
        '#include "../../enhancement/genesis_selected_source_queue.h"\n',
        "selected Genesis source queue include",
    )

    replace_once(
        header,
        "\tbool m_studio_deferred_psg_failed = false;\n",
        "\tbool m_studio_deferred_psg_failed = false;\n"
        "\tusing genesis_selected_source_queue_type = gameaudio::vgm::genesis_selected_source_queue<16640>;\n"
        "\tgenesis_selected_source_queue_type m_genesis_selected_sources{};\n"
        "\tstd::array<gameaudio::vgm::sn76489_deferred_source_frame, 8192> m_genesis_deferred_psg_source_scratch{};\n"
        "\tstd::array<gameaudio::vgm::ym2612_pcm_source_frame, 8192> m_genesis_pcm_source_scratch{};\n",
        "selected Genesis source transport state",
    )

    replace_once(
        header,
        "\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;\n",
        "\tbool capture_genesis_reference_sources(SourceAwareVGMPlayer* source_player, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n"
        "\tbool replace_genesis_selected_source(std::uint64_t ordinal, std::size_t source_index, double left, double right) noexcept;\n"
        "\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;\n",
        "selected Genesis source transport declarations",
    )

    helpers = r'''bool input_vgm::capture_genesis_reference_sources(
	SourceAwareVGMPlayer* source_player,
	UINT32 sample_count,
	UINT32 base_playback_sample) noexcept
{
	if (source_player == nullptr || sample_count == 0
		|| source_player->source_output_count() != sample_count
		|| !m_genesis_selected_sources.valid())
		return false;

	const bool ym_valid = source_player->ym_source_expected()
		&& source_player->ym_source_block_valid();
	const bool psg_valid = source_player->psg_source_expected()
		&& source_player->psg_source_block_valid();

	for (UINT32 frame = 0; frame < sample_count; ++frame)
	{
		gameaudio::vgm::genesis_selected_source_frame selected{};
		selected.ordinal = static_cast<std::uint64_t>(base_playback_sample)
			+ static_cast<std::uint64_t>(frame);

		if (ym_valid)
		{
			for (std::size_t source_index = 0; source_index < 7; ++source_index)
			{
				const auto lane = static_cast<SourceAwareVGMPlayer::source_lane>(source_index);
				const auto* source = source_player->source_output(lane);
				if (source == nullptr)
				{
					m_genesis_selected_sources.fail_closed_state();
					return false;
				}
				selected.source[source_index] = {
					static_cast<double>(source[frame].left),
					static_cast<double>(source[frame].right),
					true,
					true,
				};
			}
		}

		if (psg_valid)
		{
			for (std::size_t source_index = 7;
				source_index < gameaudio::vgm::genesis_recomposition_source_count;
				++source_index)
			{
				const auto lane = static_cast<SourceAwareVGMPlayer::source_lane>(source_index);
				const auto* source = source_player->source_output(lane);
				if (source == nullptr)
				{
					m_genesis_selected_sources.fail_closed_state();
					return false;
				}
				selected.source[source_index] = {
					static_cast<double>(source[frame].left),
					static_cast<double>(source[frame].right),
					true,
					true,
				};
			}
		}

		if (!m_genesis_selected_sources.push_reference(selected))
			return false;
	}
	return true;
}

bool input_vgm::replace_genesis_selected_source(
	std::uint64_t ordinal,
	std::size_t source_index,
	double left,
	double right) noexcept
{
	return m_genesis_selected_sources.valid()
		&& m_genesis_selected_sources.replace_source(
			ordinal, source_index, left, right, true);
}

'''
    replace_once(
        shadow,
        "void input_vgm::configure_enhancement_shadow()\n",
        helpers + "void input_vgm::configure_enhancement_shadow()\n",
        "selected Genesis source transport helpers",
    )

    # Source capture must sit outside the quality gate. Spatial-only playback
    # needs the exact protected source lanes even when Enhanced is disabled.
    replace_once(
        shadow,
        """#if defined(FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI)
\tif (!cfg_vgm_enhanced_enabled || samples == nullptr || sample_count == 0
\t\t|| sample_count > m_enhanced_candidate_mix.size() || !m_shadow_configured)
\t\treturn;

\tauto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
\tif (source_player == nullptr || !source_player->source_topology_supported()
\t\t|| source_player->source_output_count() != sample_count)
\t\treturn;
""",
        """#if defined(FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI)
\tif (samples == nullptr || sample_count == 0
\t\t|| sample_count > m_enhanced_candidate_mix.size() || !m_shadow_configured)
\t\treturn;

\tauto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
\tconst bool source_block_ready = source_player != nullptr
\t\t&& source_player->source_topology_supported()
\t\t&& source_player->source_output_count() == sample_count;
\tif (source_block_ready)
\t\tcapture_genesis_reference_sources(source_player, sample_count, base_playback_sample);
\telse if (m_genesis_selected_sources.valid())
\t\tm_genesis_selected_sources.fail_closed_state();

\tif (!cfg_vgm_enhanced_enabled || !source_block_ready)
\t\treturn;
""",
        "capture reference Genesis sources outside quality gate",
    )

    # Ordinary FM transaction. Only after the complete six-lane family has
    # validated do its selected source lanes change.
    replace_once(
        shadow,
        """\t\tif (fm_valid)
\t\t{
\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
\t\t\tchanged = true;
\t\t}
""",
        """\t\tif (fm_valid)
\t\t{
\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t{
\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
\t\t\t\tconst std::uint64_t ordinal = static_cast<std::uint64_t>(base_playback_sample)
\t\t\t\t\t+ static_cast<std::uint64_t>(frame);
\t\t\t\tfor (std::size_t channel = 0; channel < 6; ++channel)
\t\t\t\t{
\t\t\t\t\tconst auto* hq = source_player->hq_fm_source_output(channel);
\t\t\t\t\tif (hq != nullptr)
\t\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\t\tordinal, channel, hq[frame].left, hq[frame].right);
\t\t\t\t}
\t\t\t}
\t\t\tchanged = true;
\t\t}
""",
        "ordinary selected FM source replacement",
    )

    # Ordinary PSG transaction retains all four source identities.
    replace_once(
        shadow,
        """\t\t\tif (psg_valid)
\t\t\t{
\t\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
\t\t\t\tm_enhanced_psg[0] = candidate_psg;
\t\t\t\tm_enhanced_psg_block_rendered = true;
\t\t\t\tchanged = true;
\t\t\t}
""",
        """\t\t\tif (psg_valid)
\t\t\t{
\t\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t\t{
\t\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_family_scratch[frame];
\t\t\t\t\tconst std::uint64_t ordinal = static_cast<std::uint64_t>(base_playback_sample)
\t\t\t\t\t\t+ static_cast<std::uint64_t>(frame);
\t\t\t\t\tfor (std::size_t channel = 0;
\t\t\t\t\t\tchannel < gameaudio::vgm::sn76489_enhanced::stem_count;
\t\t\t\t\t\t++channel)
\t\t\t\t\t{
\t\t\t\t\t\tconst auto enhanced = m_enhanced_psg_source_block.source(channel);
\t\t\t\t\t\tif (enhanced.left != nullptr && enhanced.right != nullptr)
\t\t\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\t\t\tordinal, 7u + channel,
\t\t\t\t\t\t\t\tenhanced.left[frame], enhanced.right[frame]);
\t\t\t\t\t}
\t\t\t\t}
\t\t\t\tm_enhanced_psg[0] = candidate_psg;
\t\t\t\tm_enhanced_psg_block_rendered = true;
\t\t\t\tchanged = true;
\t\t\t}
""",
        "ordinary selected PSG source replacement",
    )

    # Classic DAC transaction.
    replace_once(
        shadow,
        """\t\t\tif (ok) {
\t\t\t\tfor (UINT32 f = 0; f < sample_count; ++f) m_enhanced_candidate_mix[f] = m_enhanced_family_scratch[f];
\t\t\t\tm_enhanced_dac[0] = candidate; m_enhanced_dac_block_rendered = true; changed = true;
\t\t\t}
""",
        """\t\t\tif (ok) {
\t\t\t\tfor (UINT32 f = 0; f < sample_count; ++f) {
\t\t\t\t\tm_enhanced_candidate_mix[f] = m_enhanced_family_scratch[f];
\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\tstatic_cast<std::uint64_t>(base_playback_sample) + static_cast<std::uint64_t>(f),
\t\t\t\t\t\tstatic_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::ym2612_dac),
\t\t\t\t\t\tm_enhanced_dac_source_block.left()[f],
\t\t\t\t\t\tm_enhanced_dac_source_block.right()[f]);
\t\t\t\t}
\t\t\t\tm_enhanced_dac[0] = candidate; m_enhanced_dac_block_rendered = true; changed = true;
\t\t\t}
""",
        "ordinary selected classic DAC source replacement",
    )

    # Source-bank DAC is validated as a complete block before selected-source
    # state is mutated. Save every popped frame first.
    replace_once(
        shadow,
        """\t\t\tgameaudio::vgm::ym2612_pcm_source_frame enhanced{};
\t\t\tif (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
\t\t\t{
\t\t\t\tpcm_stream_block = false;
\t\t\t\tbreak;
\t\t\t}
\t\t\tif (!enhanced.replace_reference)
""",
        """\t\t\tgameaudio::vgm::ym2612_pcm_source_frame enhanced{};
\t\t\tif (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
\t\t\t{
\t\t\t\tpcm_stream_block = false;
\t\t\t\tbreak;
\t\t\t}
\t\t\tm_genesis_pcm_source_scratch[frame] = enhanced;
\t\t\tif (!enhanced.replace_reference)
""",
        "stage ordinary selected source-bank DAC frames",
    )
    replace_once(
        shadow,
        """\t\tif (pcm_stream_block && pcm_stream_changed)
\t\t{
\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_pcm_source_scratch[frame];
\t\t\tchanged = true;
\t\t}
""",
        """\t\tif (pcm_stream_block && pcm_stream_changed)
\t\t{
\t\t\tfor (UINT32 frame = 0; frame < sample_count; ++frame)
\t\t\t{
\t\t\t\tm_enhanced_candidate_mix[frame] = m_enhanced_pcm_source_scratch[frame];
\t\t\t\tconst auto& selected_dac = m_genesis_pcm_source_scratch[frame];
\t\t\t\tif (selected_dac.replace_reference)
\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\tstatic_cast<std::uint64_t>(base_playback_sample) + static_cast<std::uint64_t>(frame),
\t\t\t\t\t\tstatic_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::ym2612_dac),
\t\t\t\t\t\tstatic_cast<double>(selected_dac.left),
\t\t\t\t\t\tstatic_cast<double>(selected_dac.right));
\t\t\t}
\t\t\tchanged = true;
\t\t}
""",
        "commit ordinary selected source-bank DAC frames",
    )

    # Deferred PSG: stage the four-lane frame alongside the candidate stereo and
    # publish only if the whole family block survives validation.
    replace_once(
        shadow,
        """\t\t\t\tgameaudio::vgm::sn76489_deferred_source_frame enhanced{};
\t\t\t\tif (!m_studio_deferred_psg_queue.pop_expected(ordinal, enhanced))
\t\t\t\t{
\t\t\t\t\tdeferred_psg_block = false;
\t\t\t\t\tbreak;
\t\t\t\t}

\t\t\t\tstd::int64_t exact_left = 0;
""",
        """\t\t\t\tgameaudio::vgm::sn76489_deferred_source_frame enhanced{};
\t\t\t\tif (!m_studio_deferred_psg_queue.pop_expected(ordinal, enhanced))
\t\t\t\t{
\t\t\t\t\tdeferred_psg_block = false;
\t\t\t\t\tbreak;
\t\t\t\t}
\t\t\t\tm_genesis_deferred_psg_source_scratch[frame] = enhanced;

\t\t\t\tstd::int64_t exact_left = 0;
""",
        "stage deferred selected PSG source frames",
    )
    replace_once(
        shadow,
        """\t\t\tif (!deferred_psg_block)
\t\t\t\tfail_studio_deferred_psg();
\t\t}

\t\tbool deferred_pcm_block = !m_pcm_stream_queue_failed
""",
        """\t\t\tif (!deferred_psg_block)
\t\t\t\tfail_studio_deferred_psg();
\t\t\telse
\t\t\t{
\t\t\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
\t\t\t\t{
\t\t\t\t\tconst auto& enhanced = m_genesis_deferred_psg_source_scratch[frame];
\t\t\t\t\tfor (std::size_t channel = 0;
\t\t\t\t\t\tchannel < gameaudio::vgm::sn76489_enhanced::stem_count;
\t\t\t\t\t\t++channel)
\t\t\t\t\t{
\t\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\t\trendered_base_playback_sample + static_cast<std::uint64_t>(frame),
\t\t\t\t\t\t\t7u + channel,
\t\t\t\t\t\t\tstatic_cast<double>(enhanced.source_left[channel]),
\t\t\t\t\t\t\tstatic_cast<double>(enhanced.source_right[channel]));
\t\t\t\t\t}
\t\t\t\t}
\t\t\t}
\t\t}

\t\tbool deferred_pcm_block = !m_pcm_stream_queue_failed
""",
        "commit deferred selected PSG source frames",
    )

    # Deferred source-bank DAC uses the same block-transaction rule.
    replace_once(
        shadow,
        """\t\t\t\tgameaudio::vgm::ym2612_pcm_source_frame enhanced{};
\t\t\t\tif (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
\t\t\t\t{
\t\t\t\t\tdeferred_pcm_block = false;
\t\t\t\t\tbreak;
\t\t\t\t}
\t\t\t\tif (!enhanced.replace_reference)
""",
        """\t\t\t\tgameaudio::vgm::ym2612_pcm_source_frame enhanced{};
\t\t\t\tif (!m_pcm_stream_queue.pop_expected(ordinal, enhanced))
\t\t\t\t{
\t\t\t\t\tdeferred_pcm_block = false;
\t\t\t\t\tbreak;
\t\t\t\t}
\t\t\t\tm_genesis_pcm_source_scratch[frame] = enhanced;
\t\t\t\tif (!enhanced.replace_reference)
""",
        "stage deferred selected source-bank DAC frames",
    )
    replace_once(
        shadow,
        """\t\t\tif (!deferred_pcm_block)
\t\t\t{
\t\t\t\tm_pcm_stream_queue.fail_closed();
\t\t\t\tm_pcm_stream_queue_capture_active = false;
\t\t\t\tm_pcm_stream_queue_failed = true;
\t\t\t\tdeferred_pcm_changed = false;
\t\t\t}
\t\t}

\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        """\t\t\tif (!deferred_pcm_block)
\t\t\t{
\t\t\t\tm_pcm_stream_queue.fail_closed();
\t\t\t\tm_pcm_stream_queue_capture_active = false;
\t\t\t\tm_pcm_stream_queue_failed = true;
\t\t\t\tdeferred_pcm_changed = false;
\t\t\t}
\t\t\telse if (deferred_pcm_changed)
\t\t\t{
\t\t\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
\t\t\t\t{
\t\t\t\t\tconst auto& selected_dac = m_genesis_pcm_source_scratch[frame];
\t\t\t\t\tif (selected_dac.replace_reference)
\t\t\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\t\t\trendered_base_playback_sample + static_cast<std::uint64_t>(frame),
\t\t\t\t\t\t\tstatic_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::ym2612_dac),
\t\t\t\t\t\t\tstatic_cast<double>(selected_dac.left),
\t\t\t\t\t\t\tstatic_cast<double>(selected_dac.right));
\t\t\t\t}
\t\t\t}
\t\t}

\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        "commit deferred selected source-bank DAC frames",
    )

    # Deferred FM ready frames are already per-channel and have exact destination
    # ordinals. Publish them only after the whole-frame FM transport accepted the
    # same ready frame.
    replace_once(
        shadow,
        """\t\t\tif (!finite || !m_studio_fm_transport.apply_studio_fm(
\t\t\t\t\tready.destination_ordinal,
\t\t\t\t\tstatic_cast<std::int64_t>(rounded_left),
\t\t\t\t\tstatic_cast<std::int64_t>(rounded_right)))
\t\t\t{
\t\t\t\tfail_studio_deferred_quality();
\t\t\t\tbreak;
\t\t\t}
""",
        """\t\t\tif (!finite || !m_studio_fm_transport.apply_studio_fm(
\t\t\t\t\tready.destination_ordinal,
\t\t\t\t\tstatic_cast<std::int64_t>(rounded_left),
\t\t\t\t\tstatic_cast<std::int64_t>(rounded_right)))
\t\t\t{
\t\t\t\tfail_studio_deferred_quality();
\t\t\t\tbreak;
\t\t\t}
\t\t\tfor (std::size_t channel = 0; channel < ready.lane.size(); ++channel)
\t\t\t\treplace_genesis_selected_source(
\t\t\t\t\tready.destination_ordinal,
\t\t\t\t\tchannel,
\t\t\t\t\tready.lane[channel].left,
\t\t\t\t\tready.lane[channel].right);
""",
        "commit deferred selected FM source frames",
    )

    print("foo_input_vgm selected Genesis source transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
