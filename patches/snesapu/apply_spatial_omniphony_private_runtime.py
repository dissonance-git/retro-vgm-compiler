#!/usr/bin/env python3
"""Replace the pinned foo_snesapu presentation stage with a minimal 7.1 bed.

Private-build prerequisite:
  dissonance-git/vgmspc@2b7ec8bbd7326eabee3ba39bb91130b9b128e74b

The pinned parent/child already transports protected stereo plus SRCE-v2. This
patch removes the old semantic enhancer and uses only source-native separation:

  front remainder = protected stereo - exact post-EVOL S-DSP echo
  rear/side field = exact post-EVOL S-DSP echo, equal-power split

No compiler-side scene inference, role classifier, adaptive governor, HRTF, room
model, source enhancement, or synthetic decorrelation participates. The Foobar
chunk itself becomes standard 7.1 and may be consumed as an authored bed by
Omniphony or any other multichannel-capable output.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil

PINNED_VGMSPC_BASE = "2b7ec8bbd7326eabee3ba39bb91130b9b128e74b"


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


def stage_current_headers(foo_root: Path) -> None:
    here = Path(__file__).resolve().parent
    repo = here.parents[1]
    overlay = foo_root / "retro_vgm"
    if overlay.exists():
        shutil.rmtree(overlay)
    (overlay / "components").mkdir(parents=True)
    shutil.copytree(repo / "model", overlay / "model")
    shutil.copytree(repo / "components" / "spc", overlay / "components" / "spc")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "foo_snesapu_root",
        type=Path,
        help="Pinned vgmspc foo_snesapu root containing foobar2000/ and spcplayer/",
    )
    root = parser.parse_args().foo_snesapu_root.resolve()
    parent = root / "foobar2000" / "foo_snesapu"
    header = parent / "input_snesapu.hpp"
    source = parent / "input_snesapu.cpp"
    controller = parent / "spcplayer_controller.h"

    for required in (
        root / "spcplayer" / "source_capture_output.h",
        parent / "spc_source_block.h",
        controller,
        header,
        source,
    ):
        if not required.exists():
            raise RuntimeError(
                f"missing pinned SRCE transport prerequisite: {required}; "
                f"use vgmspc {PINNED_VGMSPC_BASE}"
            )

    controller_text = controller.read_text(encoding="utf-8-sig", errors="replace")
    for needle in ("EmuAPU_with_sources", "SetSourceEnabled", "spc_source_block"):
        if needle not in controller_text:
            raise RuntimeError(
                f"pinned SRCE controller contract missing {needle!r}; "
                f"expected vgmspc {PINNED_VGMSPC_BASE}"
            )

    stage_current_headers(root)

    replace_once(
        header,
        """#include "spcplayer_controller.h"
#include "role_classifier_snes.h"
#include "semantic_mixer_71.h"
#include "semantic_stereo_enhancer.h"
#include "spc_adapter.h"
#include "../../../inference/chip_hint_bus.h"
#include "../../spcplayer/spcplayer.h"
""",
        """#include "spcplayer_controller.h"
#include "../../spcplayer/spcplayer.h"
#include "../../retro_vgm/components/spc/snesapu_source_transport_v2_storage.h"
#include "../../retro_vgm/model/surround_bed_7_1.h"
""",
        "minimal SPC 7.1 header overlay",
    )

    replace_once(
        header,
        """#ifdef _WIN64
\t// Semantic 7.1 enhancement
\tbool\t\t\t\t\t\tm_Sem71Enabled;\t\t// 7.1 mode active
\tSpcBlockTelem\t\t\t\tm_Telem;\t\t\t// current voice telemetry block
\tSnesRoleClassifier\t\t\tm_Classifier;\t\t// per-voice role inference
\tSpcAdapter\t\t\t\t\tm_Adapter;
\tSemanticStereoEnhancer\t\tm_Enhancer;
\tpfc::array_t<t_uint8>\t\tm_Buf71;\t\t\t// 7.1 interleaved output buffer
#endif
""",
        """#ifdef _WIN64
\t// Minimal source-native Surround path: protected stereo + native S-DSP echo.
\tbool m_Sem71Enabled = false;
\tSpcBlockTelem m_Telem{}; // retained because the pinned wire pairs TLEM with SRCE
\tspc_source_block m_SourceBlock{};
\tgameaudio::spc::snesapu_source_transport_v2_storage<1024> m_SurroundSource{};
\tvgmtooling::model::surround_7_1_bed_storage<1024> m_SurroundBed{};
#endif
""",
        "minimal SPC 7.1 runtime state",
    )

    replace_once(
        header,
        """\tbool LoadDll();
\tvoid FreeDll();
""",
        """\tbool LoadDll();
\tvoid FreeDll();
#ifdef _WIN64
\tvoid ResetSurroundRuntime() noexcept;
\tbool RenderSurroundBlock(audio_chunk& chunk, u32 frames) noexcept;
#endif
""",
        "SPC 7.1 method declarations",
    )

    replace_once(
        header,
        """\tinput_snesapu()
\t\t:  m_DllInst(NULL)
\t\t,  m_Adapter(m_Classifier)
\t{
""",
        """\tinput_snesapu()
\t\t:  m_DllInst(NULL)
\t{
""",
        "remove obsolete classifier constructor dependency",
    )

    helpers = r'''#ifdef _WIN64
void input_snesapu::ResetSurroundRuntime() noexcept
{
	m_SourceBlock.clear();
	m_SurroundSource.reset();
	m_SurroundBed.reset();
}

bool input_snesapu::RenderSurroundBlock(audio_chunk& chunk, u32 frames) noexcept
{
	if (!m_Sem71Enabled || frames == 0 || frames > 1024u
		|| !m_SourceBlock.valid() || m_SourceBlock.frames != frames)
		return false;
	if (chunk.get_channels() != 2 || chunk.get_srate() != m_CnfSampleRate)
		return false;

	const audio_sample* reference = chunk.get_data();
	if (reference == nullptr)
		return false;
	if (!m_SurroundSource.load_planar_slice(
			m_SourceBlock.planar.data(),
			m_SourceBlock.frames,
			0,
			static_cast<std::size_t>(frames)))
		return false;

	const auto source = m_SurroundSource.view();
	if (!m_SurroundBed.begin_from_interleaved_stereo(
			reference, static_cast<std::size_t>(frames)))
		return false;
	if (!m_SurroundBed.move_stereo_to_surround_field(
			source.echo_left(),
			source.echo_right(),
			static_cast<std::size_t>(frames)))
		return false;

	chunk.set_data_floatingpoint_ex(
		m_SurroundBed.data(),
		static_cast<t_size>(
			static_cast<std::size_t>(frames)
			* vgmtooling::model::surround_7_1_channel_count
			* sizeof(float)),
		m_CnfSampleRate,
		static_cast<unsigned>(vgmtooling::model::surround_7_1_channel_count),
		32,
		0,
		audio_chunk::channel_config_7point1);
	return true;
}
#endif

'''
    replace_once(
        source,
        """void input_snesapu::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
""",
        helpers + """void input_snesapu::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
""",
        "minimal SPC 7.1 runtime helpers",
    )

    replace_once(
        source,
        """#ifdef _WIN64
\t// Semantic Stereo Enhancer (spatial pre-conditioning)
\tm_Sem71Enabled = cfg_sem71_enabled;
\tm_Apu.SetTelemetryEnabled(m_Sem71Enabled);

\tm_Apu.end_decode_initialization();
#else
""",
        """#ifdef _WIN64
\t// Surround requests exact SRCE transport only. Presentation is a normal 7.1
\t// Foobar bed and does not invoke a renderer inside the decoder.
\tm_Sem71Enabled = cfg_sem71_enabled;
\tm_Apu.SetSourceEnabled(m_Sem71Enabled);
\tm_Apu.end_decode_initialization();
\tResetSurroundRuntime();
#else
""",
        "enable source transport only for SPC Surround",
    )

    replace_once(
        source,
        """#ifdef _WIN64
\tif (m_Sem71Enabled)
\t{
\t\t// 7.1 output: 8 channels, same bit depth, same sample count
\t\tu32 Buf71Length = static_cast<u32>(((BUFFER_DURATION * m_CnfSampleRate) + BUFFER_DURATION) / 1000);
\t\tBuf71Length = Buf71Length * 8 * (m_CnfBPS / 8);
\t\tm_Buf71.set_size(Buf71Length);
\t\tm_Enhancer.reset();
\t}
#endif
""",
        """#ifdef _WIN64
\t// The 7.1 bed is fixed-capacity model storage; no presentation buffer or
\t// renderer allocation is needed here.
\tm_SurroundBed.reset();
#endif
""",
        "remove legacy SPC presentation allocation",
    )

    old_decode = r'''		// Stereo output path — feeds Scene7 Spatial DSP downstream.
#ifdef _WIN64
		m_Apu.EmuAPU_with_telem(m_DecodeBuffer.get_ptr(), wanted_sample, 1, m_Telem);
#else
		m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), wanted_sample, 1);
#endif

#ifdef _WIN64
		if (m_Sem71Enabled)
		{
			m_Classifier.update(m_Telem, (float)wanted_ms);
			ChipSemanticHintBus::instance().publish(m_Adapter.make_hint());

			if (m_CnfBPS == 16)
			{
				m_Enhancer.process_s16(
					(const int16_t*)m_DecodeBuffer.get_ptr(),
					wanted_sample, m_Telem, m_Classifier,
					(int16_t*)m_DecodeBuffer.get_ptr());
			}
			else if (m_CnfBPS == 32)
			{
				m_Enhancer.process_s32(
					(const int32_t*)m_DecodeBuffer.get_ptr(),
					wanted_sample, m_Telem, m_Classifier,
					(int32_t*)m_DecodeBuffer.get_ptr());
			}
		}
#endif

		p_chunk.set_data_fixedpoint(
			m_DecodeBuffer.get_ptr(),
			wanted_sample * m_CnfChannels * (m_CnfBPS / 8),
			m_CnfSampleRate, m_CnfChannels, m_CnfBPS,
			audio_chunk::g_guess_channel_config(m_CnfChannels));
'''
    new_decode = r'''		// Protected stereo is always produced first. Surround may replace the
		// Foobar chunk only after the exact native echo sidecar is complete.
#ifdef _WIN64
		if (m_Sem71Enabled)
			m_Apu.EmuAPU_with_sources(
				m_DecodeBuffer.get_ptr(), wanted_sample, 1, m_Telem, m_SourceBlock);
		else
			m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), wanted_sample, 1);
#else
		m_Apu.EmuAPU(m_DecodeBuffer.get_ptr(), wanted_sample, 1);
#endif

		p_chunk.set_data_fixedpoint(
			m_DecodeBuffer.get_ptr(),
			wanted_sample * m_CnfChannels * (m_CnfBPS / 8),
			m_CnfSampleRate, m_CnfChannels, m_CnfBPS,
			audio_chunk::g_guess_channel_config(m_CnfChannels));

#ifdef _WIN64
		if (m_Sem71Enabled)
			RenderSurroundBlock(p_chunk, wanted_sample);
#endif
'''
    replace_once(source, old_decode, new_decode, "replace legacy SPC presentation with 7.1 bed")

    replace_once(
        source,
        """\tm_SilentTime = 0;
\tm_PlayedTime = static_cast<u32>(p_seconds * 1000);
}
""",
        """\tm_SilentTime = 0;
\tm_PlayedTime = static_cast<u32>(p_seconds * 1000);
#ifdef _WIN64
\tResetSurroundRuntime();
#endif
}
""",
        "reset SPC Surround source state after seek",
    )

    print(
        "pinned SRCE foo_snesapu now emits a minimal source-native 7.1 Surround bed "
        f"(base {PINNED_VGMSPC_BASE})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
