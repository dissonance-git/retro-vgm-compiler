#!/usr/bin/env python3
"""Admit the source-native YM2612 DAC descendant in non-deferred enhanced blocks."""
from __future__ import annotations
import argparse
from pathlib import Path


def decode(raw: bytes):
    bom = raw.startswith(b"\xef\xbb\xbf")
    try: return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError: return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str):
    raw = path.read_bytes(); nl = "\r\n" if b"\r\n" in raw else "\n"
    text, enc, bom = decode(raw); old = old.replace("\n", nl); new = new.replace("\n", nl)
    if text.count(old) != 1: raise RuntimeError(f"{label}: expected one match in {path}")
    out = text.replace(old, new, 1).encode(enc)
    path.write_bytes((b"\xef\xbb\xbf" if bom else b"") + out)


def main() -> int:
    ap = argparse.ArgumentParser(); ap.add_argument("source_dir", type=Path); root = ap.parse_args().source_dir.resolve()
    header, shadow, player = root / "input_vgm.h", root / "input_vgm_shadow.cpp", root / "source_aware_vgm_player.h"

    replace_once(header,
        '#include "../../enhancement/ym2612_dac_enhanced.h"\n',
        '#include "../../enhancement/ym2612_dac_enhanced.h"\n#include "../../enhancement/ym2612_dac_enhanced_source_block.h"\n',
        "DAC source block include")
    replace_once(header,
        "\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;\n",
        "\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;\n"
        "\tgameaudio::vgm::ym2612_dac_enhanced_source_block_storage<8192> m_enhanced_dac_source_block;\n"
        "\tbool m_enhanced_dac_block_rendered = false;\n\tbool m_enhanced_dac_block_pan_left = true;\n\tbool m_enhanced_dac_block_pan_right = true;\n",
        "DAC runtime state")
    replace_once(player,
        """    bool psg_source_volume(INT16& left, INT16& right) const noexcept
    {
        if (!m_psg.attached || m_psg.resampler == nullptr)
            return false;
        left = m_psg.resampler->volumeL;
        right = m_psg.resampler->volumeR;
        return true;
    }

protected:
""",
        """    bool psg_source_volume(INT16& left, INT16& right) const noexcept
    {
        if (!m_psg.attached || m_psg.resampler == nullptr)
            return false;
        left = m_psg.resampler->volumeL;
        right = m_psg.resampler->volumeR;
        return true;
    }

    bool ym_source_volume(INT16& left, INT16& right) const noexcept
    {
        if (!m_ym.attached || m_ym.resampler == nullptr) return false;
        left = m_ym.resampler->volumeL; right = m_ym.resampler->volumeR; return true;
    }

protected:
""", "YM volume view")

    dac = r'''	// YM2612 DAC is independent from FM and PSG. Preserve exact bytes/timing;
	// remove only the hold/ladder/output artifacts when pan is stable and proven.
	const bool dac_ready = !m_studio_deferred_engaged && m_dac_present[0]
		&& m_dac_shadow_valid[0] && !m_dac_capture.overflowed(0)
		&& !m_dac_capture.pan_changed(0) && source_player->ym_source_expected()
		&& source_player->ym_source_block_valid();
	if (dac_ready)
	{
		INT16 vl = 0, vr = 0;
		auto candidate = m_enhanced_dac[0];
		const auto* exact = source_player->source_output(SourceAwareVGMPlayer::source_lane::ym2612_dac);
		if (exact && source_player->ym_source_volume(vl, vr)
			&& m_enhanced_dac_source_block.render(candidate, m_dac_capture.events(0),
				m_dac_capture.count(0), sample_count, m_enhanced_dac_block_pan_left,
				m_enhanced_dac_block_pan_right, vl, vr))
		{
			for (UINT32 f = 0; f < sample_count; ++f) m_enhanced_family_scratch[f] = m_enhanced_candidate_mix[f];
			bool ok = true;
			for (UINT32 f = 0; f < sample_count && ok; ++f)
			{
				const double el = m_enhanced_dac_source_block.left()[f], er = m_enhanced_dac_source_block.right()[f];
				if (!std::isfinite(el) || !std::isfinite(er)) { ok = false; break; }
				const std::int64_t l = static_cast<std::int64_t>(m_enhanced_family_scratch[f].L)
					+ static_cast<std::int64_t>(std::llround(el)) - static_cast<std::int64_t>(exact[f].left);
				const std::int64_t r = static_cast<std::int64_t>(m_enhanced_family_scratch[f].R)
					+ static_cast<std::int64_t>(std::llround(er)) - static_cast<std::int64_t>(exact[f].right);
				if (l < INT32_MIN || l > INT32_MAX || r < INT32_MIN || r > INT32_MAX) { ok = false; break; }
				m_enhanced_family_scratch[f].L = static_cast<INT32>(l); m_enhanced_family_scratch[f].R = static_cast<INT32>(r);
			}
			if (ok) {
				for (UINT32 f = 0; f < sample_count; ++f) m_enhanced_candidate_mix[f] = m_enhanced_family_scratch[f];
				m_enhanced_dac[0] = candidate; m_enhanced_dac_block_rendered = true; changed = true;
			}
		}
	}

'''
    replace_once(shadow, "\t// ---- SN76489/96 family ------------------------------------------------\n",
        dac + "\t// ---- SN76489/96 family ------------------------------------------------\n", "audible DAC family")
    replace_once(shadow, "\tm_enhanced_psg_block_rendered = false;\n",
        "\tm_enhanced_psg_block_rendered = false;\n\tm_enhanced_dac_block_rendered = false;\n"
        "\tconst auto& dac_pan = m_genesis_state.ym2612(0).channels[5];\n"
        "\tm_enhanced_dac_block_pan_left = dac_pan.pan_left;\n\tm_enhanced_dac_block_pan_right = dac_pan.pan_right;\n",
        "DAC block-origin pan")
    replace_once(shadow,
        "\t\tif (m_dac_present[instance] && m_dac_shadow_valid[instance])\n\t\t{\n",
        "\t\tif (m_dac_present[instance] && m_dac_shadow_valid[instance]\n\t\t\t&& !(instance == 0 && m_enhanced_dac_block_rendered))\n\t\t{\n",
        "avoid DAC double advance")
    print("foo_input_vgm audible enhanced DAC block path applied")
    return 0


if __name__ == "__main__": raise SystemExit(main())
