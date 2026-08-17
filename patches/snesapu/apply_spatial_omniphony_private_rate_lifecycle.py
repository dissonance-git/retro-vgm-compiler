#!/usr/bin/env python3
"""Keep foo_snesapu's cached Omniphony processor coherent with its host rate.

Enhanced SPC playback is standardized at 48 kHz, but the independent reference
source-quality path may still use the user's configured sample rate while
Spatial is enabled. Omniphony's processor configuration is rate-specific, so a
cached processor must be recreated if a later decode generation changes rate.

This is presentation-only. It never changes SNESAPU synthesis, Enhanced
eligibility, SRCE transport, or the stereo fallback.
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
        "\tbool m_OmniphonyAttempted = false;\n",
        "\tbool m_OmniphonyAttempted = false;\n"
        "\tstd::uint32_t m_OmniphonySampleRate = 0;\n",
        "SPC Omniphony configured sample rate",
    )

    old = r'''bool input_snesapu::EnsureOmniphony() noexcept
{
	if (m_Omniphony.renderer_bound())
		return true;
	if (m_OmniphonyLoader.open())
		return m_OmniphonyLoader.bind(m_Omniphony);
	if (m_OmniphonyAttempted)
		return false;

	m_OmniphonyAttempted = true;
	vgmtooling::model::omniphony_source_config_transport config{};
	config.sample_rate_hz = static_cast<std::uint32_t>(m_CnfSampleRate);
	config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
	config.externalization = 1u;
	config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
	config.unit_scale_m = 1.0f;
	config.reflection_level = 0.22f;
	if (!m_OmniphonyLoader.open_default(config))
		return false;
	return m_OmniphonyLoader.bind(m_Omniphony);
}
'''
    new = r'''bool input_snesapu::EnsureOmniphony() noexcept
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
    replace_once(
        source,
        old,
        new,
        "SPC Omniphony rate-coherent lifecycle",
    )

    print("foo_snesapu Omniphony sample-rate lifecycle applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
