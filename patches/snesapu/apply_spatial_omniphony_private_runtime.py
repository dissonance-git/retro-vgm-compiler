#!/usr/bin/env python3
"""Replace the pinned foo_snesapu presentation stage with current Omniphony.

Private-build prerequisite:
  dissonance-git/vgmspc@2b7ec8bbd7326eabee3ba39bb91130b9b128e74b

That pin deliberately stops after the proven x86 spcplayer -> x64 foo_snesapu
[PCM][TLEM][SRCE-v2] transport and before the old Omniphony object/runtime work.
This patch keeps that process transport, stages the current VGM Compiler
model/SPC headers beside the pinned source tree, and replaces only the obsolete
Semantic Stereo Enhancer presentation path.

Source quality remains upstream of this layer. The child SRCE capture observes
whatever SNESAPU realization is actually active (reference or Enhanced). Spatial
therefore has no source-quality switch and can only present the selected causal
sources it receives.
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
    args = parser.parse_args()
    root = args.foo_snesapu_root.resolve()
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
#include "../../retro_vgm/components/spc/snesapu_direct_spatial_projection.h"
#include "../../retro_vgm/components/spc/spc_realtime_musical_omniphony_pipeline.h"
#include "../../retro_vgm/model/omniphony_dynamic_backend_loader.h"
#include <array>
""",
        "current SPC Omniphony header overlay",
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
\t// Source-aware Spatial presentation. Source quality is resolved in SNESAPU
\t// before SRCE reaches this process; none of this state can select synthesis.
\tbool m_Sem71Enabled = false;
\tSpcBlockTelem m_Telem{}; // retained only because the pinned wire pairs TLEM with SRCE
\tspc_source_block m_SourceBlock{};
\tgameaudio::spc::snesapu_source_transport_v2_storage<1024> m_SpatialSourceSlice{};
\tgameaudio::spc::snesapu_direct_spatial_projection_storage<1024> m_SpatialProjection{};
\tusing spc_omniphony_pipeline_type = gameaudio::spc::spc_realtime_musical_omniphony_pipeline<
\t\t10,
\t\tgameaudio::spc::snesapu_direct_spatial_projection_storage<1024>::max_route_events,
\t\t128>;
\tspc_omniphony_pipeline_type m_Omniphony{};
\tvgmtooling::model::omniphony_dynamic_backend_loader m_OmniphonyLoader{};
\tstd::array<float, 10u * 1024u> m_SpatialSourceScratch{};
\tstd::array<float, 2u * 1024u> m_SpatialSubStereo{};
\tpfc::array_t<float> m_SpatialStereo;
\tstd::uint64_t m_SpatialSamplePos = 0;
\tstd::uint32_t m_SpatialGeneration = 1;
\tbool m_OmniphonyAttempted = false;
#endif
""",
        "current SPC Omniphony runtime state",
    )

    replace_once(
        header,
        """\tbool LoadDll();
\tvoid FreeDll();
""",
        """\tbool LoadDll();
\tvoid FreeDll();
#ifdef _WIN64
\tbool EnsureOmniphony() noexcept;
\tvoid ResetSpatialRuntime(std::uint64_t sample_pos, bool new_generation) noexcept;
\tbool RenderSpatialBlock(audio_chunk& chunk, u32 frames) noexcept;
#endif
""",
        "SPC Omniphony method declarations",
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
bool input_snesapu::EnsureOmniphony() noexcept
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

void input_snesapu::ResetSpatialRuntime(std::uint64_t sample_pos, bool new_generation) noexcept
{
	m_SourceBlock.clear();
	m_SpatialSourceSlice.reset();
	if (new_generation)
	{
		++m_SpatialGeneration;
		if (m_SpatialGeneration == 0u)
			m_SpatialGeneration = 1u;
	}
	m_SpatialProjection.reset(m_SpatialGeneration);
	m_SpatialSamplePos = sample_pos;
	if (m_Omniphony.renderer_bound())
		m_Omniphony.reset();
	if (!m_OmniphonyLoader.open())
		m_OmniphonyAttempted = false;
}

bool input_snesapu::RenderSpatialBlock(audio_chunk& chunk, u32 frames) noexcept
{
	if (!m_Sem71Enabled || frames == 0 || !m_SourceBlock.valid()
		|| m_SourceBlock.frames != frames || !EnsureOmniphony())
		return false;

	const std::size_t required_samples = static_cast<std::size_t>(frames) * 2u;
	if (m_SpatialStereo.get_size() < required_samples)
		return false;

	std::size_t offset = 0;
	while (offset < frames)
	{
		const std::size_t count = std::min<std::size_t>(
			1024u,
			static_cast<std::size_t>(frames) - offset);
		if (!m_SpatialSourceSlice.load_planar_slice(
				m_SourceBlock.planar.data(),
				m_SourceBlock.frames,
				offset,
				count))
			return false;
		if (!m_SpatialProjection.build(m_SpatialSourceSlice.view()))
			return false;

		vgmtooling::model::spatial_source_host_chunk source_chunk{};
		source_chunk.sources = m_SpatialProjection.block();
		source_chunk.session_epoch = m_SpatialGeneration;
		source_chunk.reference_frame_start =
			m_SpatialSamplePos + static_cast<std::uint64_t>(offset);

		const auto rendered = m_Omniphony.process_chunk(
			source_chunk,
			static_cast<double>(m_CnfSampleRate),
			m_SpatialSourceScratch.data(),
			m_SpatialSourceScratch.size(),
			m_SpatialSubStereo.data(),
			m_SpatialSubStereo.size(),
			96u);
		if (!rendered.source_chunk_valid || !rendered.omniphony.rendered)
			return false;

		std::copy_n(
			m_SpatialSubStereo.data(),
			count * 2u,
			m_SpatialStereo.get_ptr() + offset * 2u);
		offset += count;
	}

	chunk.set_data_floatingpoint_ex(
		m_SpatialStereo.get_ptr(),
		static_cast<t_size>(required_samples * sizeof(float)),
		m_CnfSampleRate,
		2,
		32,
		0,
		audio_chunk::g_guess_channel_config(2));
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
        "current SPC Omniphony runtime helpers",
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
\t// Spatial requests causal SRCE transport. Enhanced remains an independent
\t// upstream synthesis choice and is never consulted here.
\tm_Sem71Enabled = cfg_sem71_enabled;
\tm_Apu.SetSourceEnabled(m_Sem71Enabled);
\tm_Apu.end_decode_initialization();
\tResetSpatialRuntime(0u, true);
#else
""",
        "enable causal source transport only for Spatial",
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
\tif (m_Sem71Enabled)
\t{
\t\tconst std::size_t max_frames = static_cast<std::size_t>(
\t\t\t((BUFFER_DURATION * m_CnfSampleRate) + BUFFER_DURATION) / 1000);
\t\tm_SpatialStereo.set_size(max_frames * 2u);
\t}
#endif
""",
        "allocate private Spatial stereo buffer before playback",
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
    new_decode = r'''		// Protected stereo is always produced first. Spatial may replace p_chunk only
		// after a complete causal SRCE block and Omniphony render both succeed.
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
		{
			RenderSpatialBlock(p_chunk, wanted_sample);
			m_SpatialSamplePos += static_cast<std::uint64_t>(wanted_sample);
		}
#endif
'''
    replace_once(source, old_decode, new_decode, "replace legacy SPC presentation with Omniphony")

    # The pinned seek loop remains authoritative for SPC execution and consumes
    # source metadata safely through controller::EmuAPU when source mode is on.
    # Reset only presentation identity/history at the final seek position.
    replace_once(
        source,
        """\tm_SilentTime = 0;
\tm_PlayedTime = static_cast<u32>(p_seconds * 1000);
}
""",
        """\tm_SilentTime = 0;
\tm_PlayedTime = static_cast<u32>(p_seconds * 1000);
#ifdef _WIN64
\tResetSpatialRuntime(
\t\tstatic_cast<std::uint64_t>(p_seconds * static_cast<double>(m_CnfSampleRate)),
\t\ttrue);
#endif
}
""",
        "reset SPC Spatial identity at seek destination",
    )

    print(
        "pinned SRCE foo_snesapu now uses current causal Omniphony Spatial runtime "
        f"(base {PINNED_VGMSPC_BASE})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
