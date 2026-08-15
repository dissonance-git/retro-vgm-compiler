from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "0006-qsound-native-source-observer.patch"


class QSoundNativeSourcePatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PATCH.read_text(encoding="utf-8")

    def test_patch_is_pinned_to_ctr_core_boundary(self):
        self.assertIn("emu/cores/qsound_ctr.c", self.text)
        self.assertIn("emu/cores/qsound_ctr.h", self.text)
        self.assertIn("QSOUND_CTR_SOURCE_COUNT 19", self.text)
        self.assertIn("chip->voice_output", self.text)

    def test_native_rate_is_explicit(self):
        self.assertIn("cfg->clock / 2 / 1248", self.text)
        self.assertIn("source_sample_rate", self.text)
        self.assertIn("nativeSample", self.text)

    def test_reference_stereo_path_remains_present(self):
        self.assertIn("outputs[0][curSmpl] = chip->out[0];", self.text)
        self.assertIn("outputs[1][curSmpl] = chip->out[1];", self.text)

    def test_player_only_hooks_superctr_core(self):
        self.assertIn("devDef->coreID != FCC_CTR_", self.text)
        self.assertIn("SetQSoundSourceObserver", self.text)

    def test_tap_does_not_add_per_source_resamplers(self):
        self.assertNotIn("RESMPL_STATE source", self.text)
        self.assertNotIn("source_resampler", self.text)
        self.assertNotIn("sourceResampler", self.text)


if __name__ == "__main__":
    unittest.main()
