from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "components" / "vgm" / "foo_input_vgm" / "src"
HEADER = SRC / "input_vgm.h"
SHADOW = SRC / "input_vgm_shadow.cpp"
CONSUMER = SRC / "input_vgm_qsound_consumer.cpp"


class QSoundLiveSpatialBusWiringTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.shadow = SHADOW.read_text(encoding="utf-8")
        cls.consumer = CONSUMER.read_text(encoding="utf-8")

    def test_wrapper_owns_neutral_qsound_bus_storage(self):
        self.assertIn("qsound_spatial_source_bus.h", self.header)
        self.assertIn("m_qsound_spatial_source_bus", self.header)

    def test_bus_uses_block_start_state_before_control_replay(self):
        build = self.consumer.find("m_qsound_spatial_source_bus.build(")
        projection = self.shadow.find("project_qsound_consumer_sources(block_start, m_render_done);")
        replay = self.shadow.find("replay_captured_sources(m_render_done);", projection)
        self.assertGreaterEqual(build, 0)
        self.assertGreaterEqual(projection, 0)
        self.assertGreater(replay, projection)
        self.assertIn("m_qsound_state,", self.consumer)
        self.assertIn("still the block-start source state", self.consumer)

    def test_audio_and_spatial_validity_are_not_collapsed(self):
        self.assertIn("controls_complete", self.consumer)
        self.assertIn("m_qsound_capture.overflowed()", self.consumer)
        self.assertIn("invalidates only the spatial handoff", self.consumer)
        after_build = self.consumer[self.consumer.find("m_qsound_spatial_source_bus.build(") :]
        self.assertNotIn("m_qsound_consumer_source_shadow_valid = false;", after_build)
        self.assertNotIn("m_qsound_consumer_source_storage.reset();", after_build)

    def test_seek_and_reset_clear_bus_without_touching_reference_audio(self):
        self.assertGreaterEqual(self.consumer.count("m_qsound_spatial_source_bus.reset();"), 2)
        seek = self.shadow[self.shadow.find("void input_vgm::decode_seek") :]
        self.assertIn("reset_qsound_consumer_source_path(m_qsound_audio_shadow_valid);", seek)
        self.assertNotIn("p_chunk.set_data", self.consumer + self.shadow)


if __name__ == "__main__":
    unittest.main()
