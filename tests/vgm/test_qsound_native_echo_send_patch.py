from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "0009-qsound-native-echo-send-witness.patch"


class QSoundNativeEchoSendPatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PATCH.read_text(encoding="utf-8")

    def test_mix_frame_carries_exact_sixteen_pcm_echo_words(self):
        self.assertIn("INT16 pcmEcho[16];", self.text)
        self.assertIn("voice < 16", self.text)

    def test_witness_is_read_from_same_ctr_voice_state(self):
        self.assertIn("chip->voice[voice].echo", self.text)
        self.assertNotIn("register_map[", self.text)

    def test_unavailable_accounting_does_not_export_stale_send_words(self):
        self.assertIn("chip->mix_accounting_valid", self.text)
        self.assertIn(": 0;", self.text)

    def test_player_forwards_witness_without_reinterpretation(self):
        self.assertIn("event.pcmEcho[voice] = frame->pcmEcho[voice];", self.text)

    def test_patch_does_not_touch_historical_output_assignments(self):
        self.assertNotIn("outputs[0][curSmpl]", self.text)
        self.assertNotIn("outputs[1][curSmpl]", self.text)


if __name__ == "__main__":
    unittest.main()
