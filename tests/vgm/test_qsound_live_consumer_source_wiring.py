from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "components" / "vgm" / "foo_input_vgm" / "src"
HEADER = SRC / "input_vgm.h"
SHADOW = SRC / "input_vgm_shadow.cpp"
CONSUMER = SRC / "input_vgm_qsound_consumer.cpp"
TARGETS = ROOT / "components" / "vgm" / "foo_input_vgm" / "Directory.Build.targets"


class QSoundLiveConsumerSourceWiringTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.shadow = SHADOW.read_text(encoding="utf-8")
        cls.consumer = CONSUMER.read_text(encoding="utf-8")
        cls.targets = TARGETS.read_text(encoding="utf-8")

    def test_wrapper_owns_one_shared_consumer_time_path(self):
        self.assertIn("m_qsound_consumer_time_map", self.header)
        self.assertIn("m_qsound_consumer_source_window", self.header)
        self.assertIn("m_qsound_consumer_source_storage", self.header)
        self.assertEqual(self.header.count("qsound_native_time_map m_qsound_consumer_time_map"), 1)
        self.assertNotIn("std::array<gameaudio::vgm::qsound_native_time_map", self.header)

    def test_consumer_rate_is_earned_from_first_nonempty_native_capture(self):
        configure = self.consumer.find("m_qsound_consumer_time_map.configure(")
        nonempty_guard = self.consumer.find("if (native_count == 0)")
        self.assertGreaterEqual(nonempty_guard, 0)
        self.assertGreater(configure, nonempty_guard)
        self.assertIn("m_qsound_audio_capture.native_sample_rate()", self.consumer)
        self.assertIn("static_cast<std::uint32_t>(m_sample_rate)", self.consumer)

    def test_consumer_projection_runs_only_after_native_observer_validation(self):
        invalid = self.shadow.find("if (qsound_audio_attached && !m_qsound_audio_capture.valid())")
        alignment = self.shadow.find("const bool aligned =", invalid)
        projection = self.shadow.find("project_qsound_consumer_sources(block_start, m_render_done);", alignment)
        self.assertGreaterEqual(invalid, 0)
        self.assertGreater(alignment, invalid)
        self.assertGreater(projection, alignment)

    def test_structural_corruption_fails_closed_without_auto_recovery(self):
        self.assertIn("m_qsound_consumer_source_shadow_valid = false;", self.consumer)
        self.assertIn("m_qsound_consumer_source_storage.reset();", self.consumer)
        self.assertIn("if (!m_qsound_consumer_source_shadow_valid)", self.consumer)
        self.assertNotIn("m_qsound_consumer_source_shadow_valid = true;", self.consumer)

    def test_missing_native_brackets_remain_availability_evidence(self):
        self.assertIn("Missing source brackets are intentionally not a structural failure", self.consumer)
        self.assertIn("m_qsound_consumer_source_storage.render(", self.consumer)
        self.assertNotIn("all_available()", self.consumer)

    def test_seek_resets_consumer_history_and_never_attaches_a_consumer_renderer(self):
        seek = self.shadow[self.shadow.find("void input_vgm::decode_seek") :]
        self.assertIn("reset_qsound_consumer_source_path(m_qsound_audio_shadow_valid);", seek)
        self.assertNotIn("project_qsound_consumer_sources(", seek)
        self.assertNotIn("m_qsound_consumer_source_storage.render(", seek)

    def test_reference_audio_chunk_is_never_rewritten(self):
        combined = self.shadow + self.consumer
        self.assertIn("result = input_base::decode_run(p_chunk, p_abort);", self.shadow)
        self.assertNotIn("p_chunk.set_data", combined)
        self.assertNotIn("p_chunk.set_data_fixedpoint", combined)
        self.assertNotIn("p_chunk.get_data", self.consumer)

    def test_windows_build_owns_consumer_translation_unit(self):
        self.assertIn('src\\input_vgm_qsound_consumer.cpp', self.targets)
        self.assertIn('qsound_consumer_source_storage.h', self.targets)


if __name__ == "__main__":
    unittest.main()
