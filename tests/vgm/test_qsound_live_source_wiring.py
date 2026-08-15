from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "components" / "vgm" / "foo_input_vgm" / "src" / "input_vgm.h"
SHADOW = ROOT / "components" / "vgm" / "foo_input_vgm" / "src" / "input_vgm_shadow.cpp"


class QSoundLiveSourceWiringTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.shadow = SHADOW.read_text(encoding="utf-8")

    def test_wrapper_owns_bounded_native_capture(self):
        self.assertIn("qsound_native_source_capture.h", self.header)
        self.assertIn("m_qsound_audio_capture", self.header)
        self.assertIn("m_qsound_audio_shadow_valid", self.header)

    def test_callback_copies_the_borrowed_native_frame(self):
        self.assertIn("VGM_QSOUND_SOURCE_FRAME", self.header)
        self.assertIn("m_qsound_audio_capture.observe(", self.shadow)
        self.assertIn("event->nativeSample", self.shadow)
        self.assertIn("event->sampleRate", self.shadow)
        self.assertIn("event->sourceCount", self.shadow)

    def test_observer_is_scoped_to_real_decode_blocks(self):
        attach = self.shadow.find(
            "SetQSoundSourceObserver(&input_vgm::qsound_source_callback, this)")
        decode = self.shadow.find("result = input_base::decode_run(p_chunk, p_abort);")
        self.assertGreaterEqual(attach, 0)
        self.assertGreater(decode, attach)
        self.assertGreaterEqual(
            self.shadow.count("SetQSoundSourceObserver(nullptr, nullptr)"), 3)

    def test_reference_renderer_still_returns_the_audio_chunk(self):
        self.assertIn("result = input_base::decode_run(p_chunk, p_abort);", self.shadow)
        self.assertNotIn("p_chunk.set_data", self.shadow)
        self.assertNotIn("p_chunk.set_data_fixedpoint", self.shadow)

    def test_seek_never_collects_discarded_native_audio(self):
        seek = self.shadow[self.shadow.find("void input_vgm::decode_seek") :]
        self.assertIn("m_qsound_audio_capture_active = false;", seek)
        self.assertIn("SetQSoundSourceObserver(nullptr, nullptr)", seek)
        self.assertNotIn("qsound_source_callback, this", seek)


if __name__ == "__main__":
    unittest.main()
