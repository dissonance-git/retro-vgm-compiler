#!/usr/bin/env python3
"""Connect delivered selected Genesis sources to Omniphony Spatial playback.

Runs after apply_spatial_selected_source_transport.py. Source quality has already
been resolved before this layer. This patch owns only route-evidence timing,
Omniphony lifecycle, the foobar delivery clock, and the final conditional chunk
replacement.

Failure rule: the historical/enhanced stereo chunk is produced first and remains
untouched unless a complete selected-source block, complete authored route state,
and Omniphony render all succeed.
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
        '#include "../../enhancement/genesis_spatial_route_transport.h"\n'
        '#include "../../enhancement/genesis_realtime_musical_omniphony_pipeline.h"\n'
        '#include "../../../../model/omniphony_dynamic_backend_loader.h"\n',
        "Genesis Omniphony runtime includes",
    )

    replace_once(
        header,
        "\tgenesis_selected_source_queue_type m_genesis_selected_sources{};\n",
        "\tgenesis_selected_source_queue_type m_genesis_selected_sources{};\n"
        "\tgameaudio::vgm::genesis_selected_source_block_storage<8192> m_genesis_delivered_sources{};\n"
        "\tusing genesis_spatial_route_transport_type = gameaudio::vgm::genesis_spatial_route_transport<1024, 256>;\n"
        "\tgenesis_spatial_route_transport_type m_genesis_spatial_routes{};\n"
        "\tgenesis_spatial_route_transport_type::delivered_block m_genesis_spatial_route_block{};\n"
        "\tgameaudio::vgm::genesis_realtime_musical_omniphony_pipeline<8192, 256> m_genesis_omniphony{};\n"
        "\tvgmtooling::model::omniphony_dynamic_backend_loader m_genesis_omniphony_loader{};\n"
        "\tstd::array<float, gameaudio::vgm::genesis_recomposition_source_count * 8192> m_genesis_spatial_source_scratch{};\n"
        "\tstd::array<float, 8192 * 2> m_genesis_spatial_stereo{};\n"
        "\tstd::uint64_t m_genesis_delivered_ordinal = 0;\n"
        "\tbool m_genesis_omniphony_attempted = false;\n",
        "Genesis Omniphony runtime state",
    )

    replace_once(
        header,
        "\tbool capture_genesis_reference_sources(SourceAwareVGMPlayer* source_player, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "\tvoid reset_genesis_spatial_transport(std::uint64_t delivered_ordinal = 0) noexcept;\n"
        "\tvoid seed_genesis_spatial_routes_from_current_state() noexcept;\n"
        "\tbool ensure_genesis_omniphony() noexcept;\n"
        "\tbool render_genesis_spatial_output(audio_chunk& chunk, std::uint64_t block_start, std::size_t frame_count) noexcept;\n"
        "\tbool capture_genesis_reference_sources(SourceAwareVGMPlayer* source_player, UINT32 sample_count, UINT32 base_playback_sample) noexcept;\n",
        "Genesis Omniphony runtime declarations",
    )

    helpers = r'''void input_vgm::reset_genesis_spatial_transport(std::uint64_t delivered_ordinal) noexcept
{
	m_genesis_selected_sources.reset(delivered_ordinal);
	m_genesis_delivered_sources.reset();
	m_genesis_spatial_routes.reset();
	m_genesis_spatial_route_block = {};
	m_genesis_delivered_ordinal = delivered_ordinal;
	if (m_genesis_omniphony.renderer_bound())
		m_genesis_omniphony.reset();
	if (!m_genesis_omniphony_loader.open())
		m_genesis_omniphony_attempted = false;
}

void input_vgm::seed_genesis_spatial_routes_from_current_state() noexcept
{
	const auto& ym = m_genesis_state.ym2612(0);
	for (std::size_t channel = 0; channel < ym.channels.size(); ++channel)
	{
		const auto route = gameaudio::vgm::ym2612_authored_route(
			ym.channels[channel].pan_left,
			ym.channels[channel].pan_right);
		m_genesis_spatial_routes.seed(
			channel,
			gameaudio::vgm::make_genesis_spatial_source(
				gameaudio::vgm::genesis_spatial_device::ym2612_fm,
				0,
				static_cast<std::uint8_t>(channel),
				1,
				route));
	}

	const auto ch6_route = gameaudio::vgm::ym2612_authored_route(
		ym.channels[5].pan_left,
		ym.channels[5].pan_right);
	m_genesis_spatial_routes.seed(
		static_cast<std::size_t>(gameaudio::vgm::genesis_recomposition_source::ym2612_dac),
		gameaudio::vgm::make_genesis_spatial_source(
			gameaudio::vgm::genesis_spatial_device::ym2612_dac,
			0,
			0,
			1,
			ch6_route));

	const std::uint8_t stereo_mask = m_genesis_state.psg(0).stereo_mask;
	for (std::size_t channel = 0; channel < 4; ++channel)
	{
		const auto device = channel < 3
			? gameaudio::vgm::genesis_spatial_device::sn76489_tone
			: gameaudio::vgm::genesis_spatial_device::sn76489_noise;
		m_genesis_spatial_routes.seed(
			7u + channel,
			gameaudio::vgm::make_genesis_spatial_source(
				device,
				0,
				static_cast<std::uint8_t>(channel),
				1,
				gameaudio::vgm::sn76489_authored_route(stereo_mask, channel)));
	}
}

bool input_vgm::ensure_genesis_omniphony() noexcept
{
	if (m_genesis_omniphony.renderer_bound())
		return true;

	if (!m_genesis_omniphony_loader.open())
	{
		if (m_genesis_omniphony_attempted)
			return false;
		m_genesis_omniphony_attempted = true;

		vgmtooling::model::omniphony_source_config_transport config{};
		config.sample_rate_hz = static_cast<std::uint32_t>(m_sample_rate);
		config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
		config.externalization = 1u;
		config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
		config.unit_scale_m = 1.0f;
		config.reflection_level = 0.22f;
		if (!m_genesis_omniphony_loader.open_default(config))
			return false;
	}

	return m_genesis_omniphony_loader.bind(m_genesis_omniphony);
}

bool input_vgm::render_genesis_spatial_output(
	audio_chunk& chunk,
	std::uint64_t block_start,
	std::size_t frame_count) noexcept
{
	genesis_spatial_route_transport_type::presence_array present{};
	const bool sources_ready = m_genesis_delivered_sources.consume(
		m_genesis_selected_sources,
		block_start,
		frame_count);
	if (sources_ready)
	{
		for (std::size_t source = 0; source < present.size(); ++source)
			present[source] = m_genesis_delivered_sources.source_present(source);
	}

	// Drain route evidence even while Spatial is off or source audio failed. The
	// evidence clock must always advance with delivered audio, never with a UI
	// switch or with PlayerA's render-ahead clock.
	const bool routes_ready = m_genesis_spatial_routes.prepare_delivered_block(
		block_start,
		frame_count,
		present,
		m_genesis_spatial_route_block);

	if (!cfg_vgm_sem71_enabled || !sources_ready || !routes_ready
		|| !m_genesis_spatial_route_block.routes_complete)
		return false;
	if (!ensure_genesis_omniphony())
		return false;

	const auto rendered = m_genesis_omniphony.process_selected_sources_timed(
		m_genesis_delivered_sources.sources(),
		m_genesis_spatial_route_block.initial_evidence,
		m_genesis_spatial_route_block.event_count == 0
			? nullptr : m_genesis_spatial_route_block.events.data(),
		m_genesis_spatial_route_block.event_count,
		frame_count,
		static_cast<double>(m_sample_rate),
		m_genesis_spatial_source_scratch.data(),
		m_genesis_spatial_source_scratch.size(),
		m_genesis_spatial_stereo.data(),
		m_genesis_spatial_stereo.size(),
		block_start,
		96u);
	if (!rendered.source_block_valid || !rendered.omniphony.rendered)
		return false;

	const t_size byte_count = static_cast<t_size>(
		frame_count * 2u * sizeof(m_genesis_spatial_stereo[0]));
	chunk.set_data_floatingpoint_ex(
		m_genesis_spatial_stereo.data(),
		byte_count,
		m_sample_rate,
		2,
		32,
		0,
		audio_chunk::g_guess_channel_config(2));
	return true;
}

'''
    replace_once(
        shadow,
        "bool input_vgm::capture_genesis_reference_sources(\n",
        helpers + "bool input_vgm::capture_genesis_reference_sources(\n",
        "Genesis Omniphony runtime helpers",
    )

    # Player reset is a source-transport generation boundary. Seek will replay
    # source state and then explicitly reseed at its destination below.
    replace_once(
        shadow,
        """\tif (event.kind == gameaudio::vgm::command_event_kind::reset)
\t{
""",
        """\tif (event.kind == gameaudio::vgm::command_event_kind::reset)
\t{
\t\tself->reset_genesis_spatial_transport(0);
""",
        "reset Genesis Spatial transport with source generation",
    )

    # Route evidence is observed before the block-local capture early-return.
    replace_once(
        shadow,
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\tif (self->m_source_capture_active)
""",
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\tself->m_genesis_spatial_routes.observe(
\t\tevent,
\t\tstatic_cast<std::uint64_t>(absolute_sample));

\tif (self->m_source_capture_active)
""",
        "observe sample-timed Genesis Spatial routes",
    )

    # Start each decode session with empty source/evidence clocks. Omniphony's
    # DLL may stay loaded across tracks; its runtime state is reset separately.
    replace_once(
        shadow,
        """void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
""",
        """void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
\treset_genesis_spatial_transport(0);
""",
        "initialize Genesis delivered Spatial clock",
    )

    # Consume source/evidence sidecars on *every* delivered block. Spatial only
    # replaces the chunk after Omniphony succeeds; false leaves p_chunk untouched.
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
\t\trender_genesis_spatial_output(
\t\t\tp_chunk,
\t\t\tgenesis_block_start,
\t\t\tstatic_cast<std::size_t>(m_render_done));
\t\tm_genesis_delivered_ordinal += static_cast<std::uint64_t>(m_render_done);
\t}
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        "render delivered Genesis sources through Omniphony",
    )

    # Seek replay may emit historical route writes. Discard their queued ordinals
    # after replay, seed the exact reconstructed current device routes, and start
    # a fresh delivered clock at the actual PlayerA destination.
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
\treset_genesis_spatial_transport(genesis_seek_sample);
\tseed_genesis_spatial_routes_from_current_state();
}
""",
        "reseed Genesis Spatial state after seek replay",
    )

    print("foo_input_vgm Omniphony Spatial runtime applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
