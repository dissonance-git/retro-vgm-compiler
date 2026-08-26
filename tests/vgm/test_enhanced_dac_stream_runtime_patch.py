from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PATCH_DIR = ROOT / "patches" / "foo_input_vgm"


class EnhancedDacStreamRuntimePatchTest(unittest.TestCase):
    def test_observer_session_and_mix_chain(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp)
            (src / "input_vgm.h").write_text(
                r'''#pragma once
#include "../../enhancement/ym2612_pcm_stream.h"
class input_vgm {
	std::array<WAVE_32BS, 8192> m_enhanced_family_scratch{};
	std::array<gameaudio::vgm::ym2612_pcm_stream, 256> m_pcm_streams;
	uint_fast64_t m_pcm_stream_replay_sample = 0;
	void advance_pcm_streams_to(uint_fast64_t absolute_sample) noexcept;
	void apply_pcm_stream_event(const gameaudio::vgm::dac_stream_source_event& event) noexcept;
	void reset_pcm_streams() noexcept;
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
	static void enhanced_post_render_callback(void* user_param, WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample);
	void apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;
#endif
};
''', encoding="utf-8")
            (src / "input_vgm.cpp").write_text(
                r'''input_vgm::~input_vgm()
{
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	if (m_vgm_player != nullptr)
		m_vgm_player->SetCommandObserver(nullptr, nullptr);
#endif
}
void input_vgm::register_player()
{
	m_vgm_player = new VGMPlayer;
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
	m_vgm_player->SetCommandObserver(&input_vgm::command_observer_callback, this);
#endif
	m_main_player.RegisterPlayerEngine(m_vgm_player);
}
''', encoding="utf-8")
            (src / "input_vgm_shadow.cpp").write_text(
                r'''#include "input_vgm.h"
void input_vgm::configure_enhancement_shadow()
{
	m_shadow_configured = true;
	m_shadow_replay_sample = 0;
}
#ifdef LIBVGM_GAMEAUDIO_QSOUND_SOURCE_OBSERVER
void input_vgm::qsound_source_callback(void*, const VGM_QSOUND_SOURCE_FRAME*) {}
#endif
void input_vgm::source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept
{
	input_vgm* self = static_cast<input_vgm*>(user_param);
	if (event.kind == gameaudio::vgm::command_event_kind::reset)
	{
		self->m_qsound_state.reset();
		return;
	}
	const uint_fast64_t absolute_sample =
		static_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

	if (self->m_source_capture_active)
		return;
}
void input_vgm::advance_shadow_to(uint_fast64_t absolute_sample) noexcept
{
}
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
void input_vgm::enhanced_post_render_callback(
	void* user_param,
	WAVE_32BS* samples,
	UINT32 sample_count,
	UINT32)
{
	if (user_param == nullptr || samples == nullptr || sample_count == 0)
		return;
	static_cast<input_vgm*>(user_param)->apply_enhanced_post_render(samples, sample_count);
}

void input_vgm::apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept
{
	auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
	for (UINT32 frame = 0; frame < sample_count; ++frame)
		m_enhanced_candidate_mix[frame] = samples[frame];
	bool changed = false;
	// ---- YM2612 FM family -------------------------------------------------
	const bool dac_ready = !m_studio_deferred_engaged && m_dac_present[0]
		&& m_dac_shadow_valid[0];
}
#endif
void deferred_fixture()
{
	UINT32 rendered_count = 4;
	UINT32 rendered_base_playback_sample = 100;
	SourceAwareVGMPlayer* source_player = nullptr;
	bool deferred_psg_block = false;
	if (deferred_psg_block)
	{
		if (!deferred_psg_block)
			fail_studio_deferred_psg();
	}

	for (UINT32 frame = 0; frame < rendered_count; ++frame)
	{
		foobar_vgm::source_audio::studio_transport_input_frame input{};
		input.protected_left = deferred_psg_block
			? m_enhanced_family_scratch[frame].L : rendered_samples[frame].L;
		input.protected_right = deferred_psg_block
			? m_enhanced_family_scratch[frame].R : rendered_samples[frame].R;
	}
}
void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
	m_studio_deferred_capture_bypass = false;
#endif
	input_base::decode_initialize(p_flags, p_abort);
}

bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
	const uint_fast64_t block_start = m_vgm_player != nullptr
		? static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE))
		: m_played_sample;
	advance_shadow_to(block_start);
	try
	{
		return input_base::decode_run(p_chunk, p_abort);
	}
	catch (...)
	{
		m_source_capture_active = false;
		throw;
	}
	m_source_capture_active = false;
	m_qsound_audio_capture_active = false;
	if (!result)
		return false;
	return true;
}

void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)
{
	configure_enhancement_shadow();
	m_source_capture_active = false;
	input_base::decode_seek(p_seconds, p_abort);
	if (m_vgm_player != nullptr)
		advance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
}
''', encoding="utf-8")

            for name in (
                "apply_enhanced_dac_stream_observer.py",
                "apply_enhanced_dac_stream_session_reset.py",
                "apply_enhanced_dac_stream_mix.py",
            ):
                subprocess.run(
                    [sys.executable, str(PATCH_DIR / name), str(src)],
                    check=True,
                    cwd=ROOT,
                )

            header = (src / "input_vgm.h").read_text(encoding="utf-8")
            player = (src / "input_vgm.cpp").read_text(encoding="utf-8")
            shadow = (src / "input_vgm_shadow.cpp").read_text(encoding="utf-8")

            self.assertIn("ym2612_pcm_source_queue.h", header)
            self.assertIn("ym2612_pcm_stream_bank m_pcm_streams", header)
            self.assertIn("m_enhanced_pcm_source_scratch", header)
            self.assertIn("apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample)", header)

            self.assertIn("SetDACStreamSourceObserver(&input_vgm::dac_stream_source_callback, this)", player)
            self.assertIn("SetDACStreamSourceObserver(nullptr, nullptr)", player)
            self.assertIn("mapped.sample", shadow)
            self.assertIn("Tick2Sample", shadow)
            self.assertIn("m_pcm_stream_queue.render_until", shadow)
            self.assertIn("m_pcm_stream_queue.pop_expected", shadow)
            self.assertIn("replace_reference", shadow)
            self.assertIn("+ enhanced.left - static_cast<std::int64_t>(exact_dac[frame].left)", shadow)
            self.assertIn("deferred_pcm_changed", shadow)
            self.assertIn("input.protected_left = deferred_pcm_changed", shadow)
            self.assertIn("reset_pcm_streams();\n\tinput_base::decode_initialize", shadow)
            self.assertIn("m_pcm_stream_queue.reset(static_cast<std::uint64_t>(seek_sample))", shadow)
            self.assertIn("!m_pcm_stream_queue_failed && !pcm_stream_changed", shadow)


if __name__ == "__main__":
    unittest.main()
