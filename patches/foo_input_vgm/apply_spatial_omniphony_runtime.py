#!/usr/bin/env python3
"""Build the first role-free Genesis 7.1 bed from exact YM2612/SN76489 lanes.

Runs after apply_spatial_selected_source_transport.py. The protected stereo mix
is produced first. Every exact independently delivered YM2612 FM/DAC or SN76489
lane then keeps a constant-power front anchor while part of its energy is spread
between side and back speakers on the same L/R hemisphere.

Physical channel number is only a low-discrepancy spacing seed. It is never used
as a claim about bass, lead, percussion or any other musical role. Center/LFE
remain empty. No delay, phase inversion, detune, pseudo-stereo timing, semantic
governor, HRTF, source-session side channel or decoder-side Omniphony renderer
is introduced.
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


def insert_before_function_close(
    path: Path,
    signature: str,
    insertion: str,
    label: str,
) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, bom = decode_source(raw)
    signature_file = signature.replace("\n", newline)
    insertion_file = insertion.replace("\n", newline)

    start = text.find(signature_file)
    if start < 0 or text.find(signature_file, start + len(signature_file)) >= 0:
        raise RuntimeError(f"{label}: expected one function signature in {path}")

    open_brace = text.find("{", start + len(signature_file))
    if open_brace < 0:
        raise RuntimeError(f"{label}: function body not found in {path}")

    depth = 0
    close_brace = -1
    for index in range(open_brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                close_brace = index
                break

    if close_brace < 0:
        raise RuntimeError(f"{label}: unterminated function body in {path}")
    if insertion_file.strip() in text[start:close_brace]:
        raise RuntimeError(f"{label}: insertion already present in {path}")

    encoded = (text[:close_brace] + insertion_file + text[close_brace:]).encode(encoding)
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
        '#include "../../enhancement/genesis_source_spread_7_1.h"\n'
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
	if (reference == nullptr
		|| !gameaudio::vgm::project_genesis_source_spread_7_1(
			m_genesis_delivered_sources,
			reference,
			frame_count,
			m_genesis_surround_bed))
		return false;

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

\t// Surround is a host-delivery operation. Run it after the deferred/ordinary
\t// shadow-clock branch so both playback paths feed the same authored 7.1 bed.
\tconst std::uint64_t genesis_block_start = m_genesis_delivered_ordinal;
\tif (m_render_done != 0)
\t{
\t\trender_genesis_surround_output(
\t\t\tp_chunk,
\t\t\tgenesis_block_start,
\t\t\tstatic_cast<std::size_t>(m_render_done));
\t\tm_genesis_delivered_ordinal += static_cast<std::uint64_t>(m_render_done);
\t}
\treturn true;
""",
        "render delivered Genesis sources into 7.1 bed",
    )

    insert_before_function_close(
        shadow,
        "void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)\n",
        """\t// Rejoin the delivered-source clock to the post-seek player position.
\tstd::uint64_t genesis_seek_sample = static_cast<std::uint64_t>(
\t\taudio_math::time_to_samples(p_seconds, m_sample_rate));
\tif (m_vgm_player != nullptr)
\t\tgenesis_seek_sample = static_cast<std::uint64_t>(
\t\t\tm_vgm_player->GetCurPos(PLAYPOS_SAMPLE));
\treset_genesis_surround_transport(genesis_seek_sample);
""",
        "reseed Genesis Surround source clock after seek",
    )

    print("foo_input_vgm minimal source-native Genesis 7.1 runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
