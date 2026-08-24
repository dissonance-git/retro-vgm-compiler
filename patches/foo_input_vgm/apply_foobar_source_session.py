#!/usr/bin/env python3
"""Prefer Output: Omniphony's process-local source session for Genesis Spatial.

The existing direct omniphony_source.dll path remains the fallback when another
Foobar output is selected. When Output: Omniphony is active, source truth is
published to its source-session ABI and the protected stereo chunk remains the
Foobar-visible control. This prevents a pre-binauralized VGM chunk from entering
Current a second time.
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
        '#include "../../../../model/omniphony_dynamic_backend_loader.h"\n',
        '#include "../../../../model/omniphony_dynamic_backend_loader.h"\n'
        '#include "../../../../model/omniphony_foobar_source_session_client.h"\n',
        "Genesis foobar source-session include",
    )

    replace_once(
        header,
        "\tvgmtooling::model::omniphony_dynamic_backend_loader m_genesis_omniphony_loader{};\n"
        "\tstd::array<float, gameaudio::vgm::genesis_recomposition_source_count * 8192> m_genesis_spatial_source_scratch{};\n",
        "\tvgmtooling::model::omniphony_dynamic_backend_loader m_genesis_omniphony_loader{};\n"
        "\tvgmtooling::model::omniphony_foobar_source_session_client m_genesis_foobar_session{};\n"
        "\tstd::array<float, 8192 * 2> m_genesis_reference_stereo{};\n"
        "\tbool m_genesis_omniphony_using_foobar_session = false;\n"
        "\tstd::array<float, gameaudio::vgm::genesis_recomposition_source_count * 8192> m_genesis_spatial_source_scratch{};\n",
        "Genesis foobar source-session state",
    )

    old_ensure = r'''bool input_vgm::ensure_genesis_omniphony() noexcept
{
	const std::uint32_t desired_sample_rate = static_cast<std::uint32_t>(m_sample_rate);
	if (m_genesis_omniphony.renderer_bound()
		&& m_genesis_omniphony_sample_rate == desired_sample_rate)
		return true;

	// The processor configuration is rate-specific. Never leave the generic
	// pipeline bound to an object which is about to be destroyed/recreated.
	if (m_genesis_omniphony_loader.open()
		&& m_genesis_omniphony_sample_rate != desired_sample_rate)
	{
		m_genesis_omniphony.unbind_renderer();
		m_genesis_omniphony_loader.close();
		m_genesis_omniphony_attempted = false;
		m_genesis_omniphony_sample_rate = 0;
	}
	else if (m_genesis_omniphony.renderer_bound())
	{
		// A bound pipeline without the expected configured-rate witness is not a
		// state to guess through. Rebind from the loader below.
		m_genesis_omniphony.unbind_renderer();
	}

	if (!m_genesis_omniphony_loader.open())
	{
		if (m_genesis_omniphony_attempted)
			return false;
		m_genesis_omniphony_attempted = true;

		vgmtooling::model::omniphony_source_config_transport config{};
		config.sample_rate_hz = desired_sample_rate;
		config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
		config.externalization = 1u;
		config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
		config.unit_scale_m = 1.0f;
		config.reflection_level = 0.22f;
		if (!m_genesis_omniphony_loader.open_default(config))
			return false;
	}

	if (!m_genesis_omniphony_loader.bind(m_genesis_omniphony))
		return false;
	m_genesis_omniphony_sample_rate = desired_sample_rate;
	return true;
}
'''

    new_ensure = r'''bool input_vgm::ensure_genesis_omniphony() noexcept
{
	const std::uint32_t desired_sample_rate = static_cast<std::uint32_t>(m_sample_rate);
	vgmtooling::model::omniphony_source_config_transport config{};
	config.sample_rate_hz = desired_sample_rate;
	config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
	config.externalization = 1u;
	config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
	config.unit_scale_m = 1.0f;
	config.reflection_level = 0.22f;

	// Output: Omniphony owns the final headphone render when it is active. The
	// source-session client exposes the same renderer-call surface to the causal
	// pipeline, so musical evidence/governor state does not fork by host route.
	if (m_genesis_foobar_session.output_active())
	{
		if (!m_genesis_foobar_session.configure(config))
			return false;
		if (!m_genesis_omniphony_using_foobar_session || !m_genesis_omniphony.renderer_bound())
		{
			if (m_genesis_omniphony.renderer_bound())
			{
				m_genesis_omniphony.reset();
				m_genesis_omniphony.unbind_renderer();
			}
			if (m_genesis_omniphony_loader.open())
				m_genesis_omniphony_loader.close();
			m_genesis_omniphony_attempted = false;
			if (!m_genesis_foobar_session.bind(m_genesis_omniphony))
				return false;
			m_genesis_omniphony_using_foobar_session = true;
		}
		m_genesis_omniphony_sample_rate = desired_sample_rate;
		return true;
	}

	// Another output is selected. Preserve the established direct FullSphere
	// renderer so source-aware playback remains available outside foo_out.
	if (m_genesis_omniphony_using_foobar_session)
	{
		if (m_genesis_omniphony.renderer_bound())
		{
			m_genesis_omniphony.reset();
			m_genesis_omniphony.unbind_renderer();
		}
		m_genesis_foobar_session.close();
		m_genesis_omniphony_using_foobar_session = false;
		m_genesis_omniphony_sample_rate = 0;
	}

	if (m_genesis_omniphony.renderer_bound()
		&& m_genesis_omniphony_sample_rate == desired_sample_rate)
		return true;

	// The direct processor configuration is rate-specific. Never leave the
	// generic pipeline bound to an object which is about to be recreated.
	if (m_genesis_omniphony_loader.open()
		&& m_genesis_omniphony_sample_rate != desired_sample_rate)
	{
		m_genesis_omniphony.unbind_renderer();
		m_genesis_omniphony_loader.close();
		m_genesis_omniphony_attempted = false;
		m_genesis_omniphony_sample_rate = 0;
	}
	else if (m_genesis_omniphony.renderer_bound())
	{
		m_genesis_omniphony.unbind_renderer();
	}

	if (!m_genesis_omniphony_loader.open())
	{
		if (m_genesis_omniphony_attempted)
			return false;
		m_genesis_omniphony_attempted = true;
		if (!m_genesis_omniphony_loader.open_default(config))
			return false;
	}

	if (!m_genesis_omniphony_loader.bind(m_genesis_omniphony))
		return false;
	m_genesis_omniphony_sample_rate = desired_sample_rate;
	return true;
}
'''
    replace_once(
        shadow,
        old_ensure,
        new_ensure,
        "prefer Genesis foobar source session",
    )

    old_render = r'''	if (!ensure_genesis_omniphony())
		return false;

	const auto rendered = m_genesis_omniphony.process_selected_sources_timed(
'''
    new_render = r'''	if (!ensure_genesis_omniphony())
		return false;

	if (m_genesis_omniphony_using_foobar_session)
	{
		const audio_sample* reference = chunk.get_data();
		if ((frame_count != 0 && reference == nullptr)
			|| chunk.get_channels() != 2
			|| chunk.get_srate() != m_sample_rate
			|| frame_count > m_genesis_reference_stereo.size() / 2u)
			return false;
		for (std::size_t sample = 0; sample < frame_count * 2u; ++sample)
			m_genesis_reference_stereo[sample] = static_cast<float>(reference[sample]);
		m_genesis_foobar_session.set_reference_stereo(
			m_genesis_reference_stereo.data(), frame_count);
	}

	const auto rendered = m_genesis_omniphony.process_selected_sources_timed(
'''
    replace_once(
        shadow,
        old_render,
        new_render,
        "stage Genesis protected stereo for source session",
    )

    old_result = r'''	if (!rendered.source_block_valid || !rendered.omniphony.rendered)
		return false;

	const t_size byte_count = static_cast<t_size>(
'''
    new_result = r'''	m_genesis_foobar_session.clear_reference_stereo();
	if (!rendered.source_block_valid || !rendered.omniphony.rendered)
		return false;

	const t_size byte_count = static_cast<t_size>(
'''
    replace_once(
        shadow,
        old_result,
        new_result,
        "clear Genesis source-session reference after render",
    )

    print("foo_input_vgm Genesis now prefers Output: Omniphony source session")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
