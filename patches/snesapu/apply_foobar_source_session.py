#!/usr/bin/env python3
"""Prefer Output: Omniphony's one-render source session for SPC Spatial.

The source-aware SNESAPU child remains authoritative for the eight dry voices
and linked shared-wet return. This patch changes only the final presentation
owner. When Output: Omniphony is active at its 48 kHz contract, each causal SPC
slice is published to the process-local source session while the protected
stereo chunk remains Foobar-visible. Other outputs retain the established direct
source_ffi FullSphere fallback.
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
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    parent = root / "foobar2000" / "foo_snesapu"
    header = parent / "input_snesapu.hpp"
    source = parent / "input_snesapu.cpp"

    replace_once(
        header,
        '#include "../../retro_vgm/model/omniphony_dynamic_backend_loader.h"\n',
        '#include "../../retro_vgm/model/omniphony_dynamic_backend_loader.h"\n'
        '#include "../../retro_vgm/model/omniphony_foobar_source_session_client.h"\n',
        "SPC foobar source-session include",
    )

    replace_once(
        header,
        "\tvgmtooling::model::omniphony_dynamic_backend_loader m_OmniphonyLoader{};\n"
        "\tstd::array<float, 10u * 1024u> m_SpatialSourceScratch{};\n",
        "\tvgmtooling::model::omniphony_dynamic_backend_loader m_OmniphonyLoader{};\n"
        "\tvgmtooling::model::omniphony_foobar_source_session_client m_FoobarSourceSession{};\n"
        "\tpfc::array_t<float> m_SpatialReferenceStereo;\n"
        "\tbool m_OmniphonyUsingFoobarSession = false;\n"
        "\tstd::array<float, 10u * 1024u> m_SpatialSourceScratch{};\n",
        "SPC foobar source-session state",
    )

    old_ensure = r'''bool input_snesapu::EnsureOmniphony() noexcept
{
	const std::uint32_t desired_sample_rate = static_cast<std::uint32_t>(m_CnfSampleRate);
	if (m_Omniphony.renderer_bound()
		&& m_OmniphonySampleRate == desired_sample_rate)
		return true;

	if (m_OmniphonyLoader.open()
		&& m_OmniphonySampleRate != desired_sample_rate)
	{
		m_Omniphony.unbind_renderer();
		m_OmniphonyLoader.close();
		m_OmniphonyAttempted = false;
		m_OmniphonySampleRate = 0;
	}
	else if (m_Omniphony.renderer_bound())
	{
		m_Omniphony.unbind_renderer();
	}

	if (!m_OmniphonyLoader.open())
	{
		if (m_OmniphonyAttempted)
			return false;
		m_OmniphonyAttempted = true;

		vgmtooling::model::omniphony_source_config_transport config{};
		config.sample_rate_hz = desired_sample_rate;
		config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
		config.externalization = 1u;
		config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
		config.unit_scale_m = 1.0f;
		config.reflection_level = 0.22f;
		if (!m_OmniphonyLoader.open_default(config))
			return false;
	}

	if (!m_OmniphonyLoader.bind(m_Omniphony))
		return false;
	m_OmniphonySampleRate = desired_sample_rate;
	return true;
}
'''

    new_ensure = r'''bool input_snesapu::EnsureOmniphony() noexcept
{
	const std::uint32_t desired_sample_rate = static_cast<std::uint32_t>(m_CnfSampleRate);
	vgmtooling::model::omniphony_source_config_transport config{};
	config.sample_rate_hz = desired_sample_rate;
	config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
	config.externalization = 1u;
	config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
	config.unit_scale_m = 1.0f;
	config.reflection_level = 0.22f;

	const bool output_owns_render = m_FoobarSourceSession.output_active();
	if (output_owns_render)
	{
		// Output: Omniphony's source-session and physical output clocks are 48 kHz.
		// A non-48 kHz SPC generation therefore stays protected stereo and lets
		// the actual Foobar output path perform its ordinary Current render.
		if (desired_sample_rate != 48000u)
			return false;
		if (!m_FoobarSourceSession.configure(config))
			return false;
		if (!m_OmniphonyUsingFoobarSession || !m_Omniphony.renderer_bound())
		{
			if (m_Omniphony.renderer_bound())
			{
				m_Omniphony.reset();
				m_Omniphony.unbind_renderer();
			}
			if (m_OmniphonyLoader.open())
				m_OmniphonyLoader.close();
			m_OmniphonyAttempted = false;
			if (!m_FoobarSourceSession.bind(m_Omniphony))
				return false;
			m_OmniphonyUsingFoobarSession = true;
		}
		m_OmniphonySampleRate = desired_sample_rate;
		return true;
	}

	// Another output is active. Preserve the established direct source_ffi
	// FullSphere renderer rather than making the source-session a hard dependency.
	if (m_OmniphonyUsingFoobarSession)
	{
		if (m_Omniphony.renderer_bound())
		{
			m_Omniphony.reset();
			m_Omniphony.unbind_renderer();
		}
		m_FoobarSourceSession.close();
		m_OmniphonyUsingFoobarSession = false;
		m_OmniphonySampleRate = 0;
	}

	if (m_Omniphony.renderer_bound()
		&& m_OmniphonySampleRate == desired_sample_rate)
		return true;

	if (m_OmniphonyLoader.open()
		&& m_OmniphonySampleRate != desired_sample_rate)
	{
		m_Omniphony.unbind_renderer();
		m_OmniphonyLoader.close();
		m_OmniphonyAttempted = false;
		m_OmniphonySampleRate = 0;
	}
	else if (m_Omniphony.renderer_bound())
	{
		m_Omniphony.unbind_renderer();
	}

	if (!m_OmniphonyLoader.open())
	{
		if (m_OmniphonyAttempted)
			return false;
		m_OmniphonyAttempted = true;
		if (!m_OmniphonyLoader.open_default(config))
			return false;
	}

	if (!m_OmniphonyLoader.bind(m_Omniphony))
		return false;
	m_OmniphonySampleRate = desired_sample_rate;
	return true;
}
'''
    replace_once(source, old_ensure, new_ensure, "prefer SPC foobar source session")

    replace_once(
        source,
        """\tif (m_SpatialStereo.get_size() < required_samples)
\t\treturn false;

\tstd::size_t offset = 0;
""",
        """\tif (m_SpatialStereo.get_size() < required_samples)
\t\treturn false;

\tif (m_OmniphonyUsingFoobarSession)
\t{
\t\tconst audio_sample* reference = chunk.get_data();
\t\tif ((frames != 0 && reference == nullptr)
\t\t\t|| chunk.get_channels() != 2
\t\t\t|| chunk.get_srate() != m_CnfSampleRate
\t\t\t|| m_SpatialReferenceStereo.get_size() < required_samples)
\t\t\treturn false;
\t\tfor (std::size_t sample = 0; sample < required_samples; ++sample)
\t\t\tm_SpatialReferenceStereo[sample] = static_cast<float>(reference[sample]);
\t}

\tstd::size_t offset = 0;
""",
        "capture SPC protected stereo for source session",
    )

    replace_once(
        source,
        """\t\tvgmtooling::model::spatial_source_host_chunk source_chunk{};
\t\tsource_chunk.sources = m_SpatialProjection.block();
\t\tsource_chunk.session_epoch = m_SpatialGeneration;
\t\tsource_chunk.reference_frame_start =
\t\t\tm_SpatialSamplePos + static_cast<std::uint64_t>(offset);

\t\tconst auto rendered = m_Omniphony.process_chunk(
""",
        """\t\tvgmtooling::model::spatial_source_host_chunk source_chunk{};
\t\tsource_chunk.sources = m_SpatialProjection.block();
\t\tsource_chunk.session_epoch = m_SpatialGeneration;
\t\tsource_chunk.reference_frame_start =
\t\t\tm_SpatialSamplePos + static_cast<std::uint64_t>(offset);

\t\tif (m_OmniphonyUsingFoobarSession)
\t\t{
\t\t\tm_FoobarSourceSession.set_reference_stereo(
\t\t\t\tm_SpatialReferenceStereo.get_ptr() + offset * 2u,
\t\t\t\tcount);
\t\t}

\t\tconst auto rendered = m_Omniphony.process_chunk(
""",
        "stage SPC causal slice reference",
    )

    replace_once(
        source,
        """\t\tif (!rendered.source_chunk_valid || !rendered.omniphony.rendered)
\t\t\treturn false;

\t\tstd::copy_n(
""",
        """\t\tm_FoobarSourceSession.clear_reference_stereo();
\t\tif (!rendered.source_chunk_valid || !rendered.omniphony.rendered)
\t\t\treturn false;

\t\tstd::copy_n(
""",
        "clear SPC source-session reference after slice",
    )

    replace_once(
        source,
        """\t\tm_SpatialStereo.set_size(max_frames * 2u);
\t}
#endif
""",
        """\t\tm_SpatialStereo.set_size(max_frames * 2u);
\t\tm_SpatialReferenceStereo.set_size(max_frames * 2u);
\t}
#endif
""",
        "allocate SPC protected stereo session buffer",
    )

    print("foo_snesapu now prefers Output: Omniphony source session")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
