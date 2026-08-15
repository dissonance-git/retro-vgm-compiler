from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "0007-qsound-native-mix-observer.patch"


class QSoundNativeMixPatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PATCH.read_text(encoding="utf-8")

    def test_patch_stays_on_superctr_native_boundary(self):
        self.assertIn("emu/cores/qsound_ctr.c", self.text)
        self.assertIn("QSOUND_CTR_MIX_FRAME", self.text)
        self.assertIn("SetQSoundMixObserver", self.text)
        self.assertIn("devDef->coreID != FCC_CTR_", self.text)
        self.assertIn("nativeSample", self.text)
        self.assertIn("sampleRate", self.text)

    def test_frame_exposes_shared_echo_and_final_branch_accounting(self):
        self.assertIn("echoInput", self.text)
        self.assertIn("echoOutput", self.text)
        self.assertIn("wetPostDelay[2]", self.text)
        self.assertIn("dryPostDelay[2]", self.text)
        self.assertIn("output[2]", self.text)
        self.assertIn("chip->mix_echo_input = echo_input;", self.text)
        self.assertIn("chip->mix_echo_output = echo_output;", self.text)

    def test_branch_tap_is_immediately_before_historical_final_sum(self):
        wet = self.text.find("chip->mix_wet_post_delay[ch] = delay(&chip->wet[ch], wet);")
        dry = self.text.find("chip->mix_dry_post_delay[ch] = delay(&chip->dry[ch], dry);")
        total = self.text.find("output = chip->mix_wet_post_delay[ch] + chip->mix_dry_post_delay[ch];")
        rounding = self.text.find("output = (output + 0x2000) >> 14;", total)
        self.assertGreaterEqual(wet, 0)
        self.assertGreater(dry, wet)
        self.assertGreater(total, dry)
        self.assertGreater(rounding, total)

    def test_source_and_mix_callbacks_share_one_native_index(self):
        source = self.text.find("chip->source_cb(chip->source_cb_param, chip->source_sample_index")
        mix_cb = self.text.find("chip->mix_cb(chip->mix_cb_param, chip->source_sample_index", source)
        advance = self.text.find("chip->source_sample_index ++;", mix_cb)
        self.assertGreaterEqual(source, 0)
        self.assertGreater(mix_cb, source)
        self.assertGreater(advance, mix_cb)

    def test_tap_does_not_create_source_or_branch_resamplers(self):
        self.assertNotIn("RESMPL_STATE", self.text)
        self.assertNotIn("source_resampler", self.text)
        self.assertNotIn("mix_resampler", self.text)
        self.assertNotIn("wet_resampler", self.text)


if __name__ == "__main__":
    unittest.main()
