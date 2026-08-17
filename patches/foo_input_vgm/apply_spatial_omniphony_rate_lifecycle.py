#!/usr/bin/env python3
"""Keep the cached VGM Omniphony processor coherent with the host sample rate.

The historical VGM shell takes its output rate from cfg_sample_rate, while the
Omniphony source processor is created with a rate-specific configuration. The
DLL/processor may remain resident across decode generations, so a later track or
preference change must not reuse a processor created for a different rate.

This patch is presentation-only: it never changes VGM output rate or source
quality. On a rate change it unbinds and recreates only the Omniphony processor;
ordinary stereo remains the already-produced fallback throughout.
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
        "\tbool m_genesis_omniphony_attempted = false;\n",
        "\tbool m_genesis_omniphony_attempted = false;\n"
        "\tstd::uint32_t m_genesis_omniphony_sample_rate = 0;\n",
        "Genesis Omniphony configured sample rate",
    )

    old = r'''bool input_vgm::ensure_genesis_omniphony() noexcept
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
'''
    new = r'''bool input_vgm::ensure_genesis_omniphony() noexcept
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
    replace_once(
        shadow,
        old,
        new,
        "Genesis Omniphony rate-coherent lifecycle",
    )

    print("foo_input_vgm Omniphony sample-rate lifecycle applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
