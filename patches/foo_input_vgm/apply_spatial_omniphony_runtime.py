#!/usr/bin/env python3
"""Build a minimal Genesis 7.1 bed from exact delivered source families.

Runs after apply_spatial_selected_source_transport.py. The protected stereo mix
is still produced first. Surround then redistributes only source contributions
that the patched libvgm path has already proven exact and aligned to that mix:

  FM + residual/unproven content -> front
  exact SN76489 tone/noise       -> sides
  exact YM2612 DAC               -> backs
  center / LFE                   -> empty

This is a fixed presentation convention, not a claim that the Mega Drive
authored rear speakers. It deliberately contains no role inference, scene
governor, HRTF, room model, source-session side channel, or decoder-side
Omniphony renderer. Future VGM chips with real multichannel buses can map those
buses directly into the same standard 7.1 bed contract.
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
        '#include "../../enhancement/genesis_selected_source_queue.h"\n',
        '#include "../../enhancement/genesis_selected_source_queue.h"\n'
        '#include "../../enhancement/genesis_selected_source_block.h"\n'
        '#include "../../../../model/surround_bed_7_1.h"\n',
        "Genesis 7.1 runtime includes",
    )

    replace_once(
        header,
        "\tgenesis_selected_source_queue_type m_genesis_selected_sources{};\n",
        "\tgenesis_selected_source_queue_type m_genesis_selected_sources{};\n"
        "\tgameaudio::vgm::genesis_selected_source_block_storage<8192> m_genesis_delivered_sources{};\n"
        "\tvgmtooling::model::surround_7_1_bed_storage<8192> m_genesis_surround_bed{};\n"
        "\tstd::uint64_t m_genesis_delivered_ordinal = 0;\n",
        "Genesis 7.1 runtime state",
    )

    replace_once(
        header,
        "\tbool capture_genesis_reference_sources(SourceAwareVGMPlayer* source_player, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "\tvoid reset_genesis_surround_transport(std::uint64_t delivered_ordinal = 0) noexcept;\n"
        "\tbool render_genesis_surround_output(audio_chunk& chunk, std::uint64_t block_start, std::size_t frame_count) noexcept;\n"
        "\tbool capture_genesis_reference_sources(SourceAwareVGMPlayer* source_player, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "Genesis 7.1 runtime declarations",
    )

    helpers = r'''void input_vgm::reset_genesis_surround_transport(std::uint64_t delivered_ordinal) noexcept
{
	m_genesis_selected_sources.reset(delivered_ordinal);
	m_genesis_delivered_sources.reset();
	m_genesis_surround_bed.reset();
	m_genesis_delivered_ordinal = delivered_ordinal;
}

bool input_vgm::render_genesis_surround_output(
	audio_chunk& chunk,
	std::uint64_t block_start,
	std::size_t frame_count) noexcept
{
	// Consume the delivered source clock even while Surround is off. Toggling the
	// preference must not leave render-ahead source packets queued against a stale
	// Foobar ordinal.
	const bool sources_ready = m_genesis_delivered_sources.consume(
		m_genesis_selected_sources,
		block_start,
		frame_count);

	if (!cfg_vgm_sem71_enabled || !sources_ready || frame_count == 0
		|| frame_count > 8192u || chunk.get_channels() != 2
		|| chunk.get_srate() != m_sample_rate)
		return false;

	const audio_sample* reference = chunk.get_data();
	if (reference == nullptr || !m_genesis_surround_bed.begin_from_interleaved_stereo(
			reference, frame_count))
		return false;

	const auto& sources = m_genesis_delivered_sources.sources();

	// YM2612 DAC is an exact independently delivered source contribution. Move it
	// from the protected front mix to the rear pair without cloning it.
	const std::size_t dac = static_cast<std::size_t>(
		gameaudio::vgm::genesis_recomposition_source::ym2612_dac);
	if (m_genesis_delivered_sources.source_present(dac))
	{
		const auto& source = sources[dac];
		if (!source.exact || !m_genesis_surround_bed.move_stereo_to_backs(
				source.left, source.right, frame_count))
			return false;
	}

	// SN76489 tones/noise remain distinct in the transport but share one simple
	// presentation family for this first bed. Every exact lane is subtracted from
	// the front once before being accumulated into the side pair.
	for (std::size_t source_index =
			static_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::sn76489_tone0);
		source_index <=
			static_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::sn76489_noise);
		++source_index)
	{
		if (!m_genesis_delivered_sources.source_present(source_index))
			continue;
		const auto& source = sources[source_index];
		if (!source.exact || !m_genesis_surround_bed.move_stereo_to_sides(
				source.left, source.right, frame_count))
			return false;
	}

	chunk.set_data_floatingpoint_ex(
		m_genesis_surround_bed.data(),
		static_cast<t_size>(
			frame_count
			* vgmtooling::model::surround_7_1_channel_count
			* sizeof(float)),
		m_sample_rate,
		static_cast<unsigned>(vgmtooling::model::surround_7_1_channel_count),
		32,
		0,
		audio_chunk::channel_config_7point1);
	return true;
}

'''
    replace_once(
        shadow,
        "bool input_vgm::capture_genesis_reference_sources(\n",
        helpers + "bool input_vgm::capture_genesis_reference_sources(\n",
        "Genesis 7.1 runtime helpers",
    )

    replace_once(
        shadow,
        """\tif (event.kind == gameaudio::vgm::command_event_kind::reset)
\t{
""",
        """\tif (event.kind == gameaudio::vgm::command_event_kind::reset)
\t{
\t\tself->reset_genesis_surround_transport(0);
""",
        "reset Genesis Surround transport with source generation",
    )

    replace_once(
        shadow,
        """void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
""",
        """void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
\treset_genesis_surround_transport(0);
""",
        "initialize Genesis delivered Surround clock",
    )

    replace_once(
        shadow,
        """\tproject_qsound_consumer_sources(block_start, m_render_done);
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        """\tproject_qsound_consumer_sources(block_start, m_render_done);
\tconst std::uint64_t genesis_block_start = m_genesis_delivered_ordinal;
\tif (m_render_done != 0)
\t{
\t\trender_genesis_surround_output(
\t\t\tp_chunk,
\t\t\tgenesis_block_start,
\t\t\tstatic_cast<std::size_t>(m_render_done));
\t\tm_genesis_delivered_ordinal += static_cast<std::uint64_t>(m_render_done);
\t}
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        "render delivered Genesis sources into 7.1 bed",
    )

    replace_once(
        shadow,
        """\tif (m_vgm_player != nullptr)
\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
}
""",
        """\tstd::uint64_t genesis_seek_sample = static_cast<std::uint64_t>(
\t\taudio_math::time_to_samples(p_seconds, m_sample_rate));
\tif (m_vgm_player != nullptr)
\t{
\t\tconst auto player_sample = static_cast<uint_fast64_t>(
\t\t\tm_vgm_player->GetCurPos(PLAYPOS_SAMPLE));
\t\tadvance_shadow_to(player_sample);
\t\tgenesis_seek_sample = static_cast<std::uint64_t>(player_sample);
\t}
\treset_genesis_surround_transport(genesis_seek_sample);
}
""",
        "reseed Genesis Surround source clock after seek",
    )

    print("foo_input_vgm minimal source-native Genesis 7.1 runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
