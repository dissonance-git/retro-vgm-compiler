from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "0008-qsound-native-mix-validity.patch"


class QSoundNativeMixValidityPatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PATCH.read_text(encoding="utf-8")

    def test_frame_carries_explicit_accounting_validity(self):
        self.assertIn("UINT8 accountingValid;", self.text)
        self.assertIn("event.accountingValid = frame->accountingValid;", self.text)

    def test_every_dsp_tick_starts_unearned(self):
        update = self.text.find("static void update_sample(struct qsound_chip *chip)")
        clear = self.text.find("chip->mix_accounting_valid = 0;", update)
        switch = self.text.find("switch(chip->state)", update)
        self.assertGreaterEqual(update, 0)
        self.assertGreater(clear, update)
        self.assertGreater(switch, clear)

    def test_only_normal_render_earns_fresh_accounting(self):
        normal = self.text.find("static void state_normal_update(struct qsound_chip *chip)")
        earned = self.text.find("chip->mix_accounting_valid = 1;", normal)
        self.assertGreaterEqual(normal, 0)
        self.assertGreater(earned, normal)
        self.assertEqual(self.text.count("chip->mix_accounting_valid = 1;"), 1)

    def test_invalid_ticks_do_not_export_stale_values(self):
        self.assertIn(
            "frame.echoInput = chip->mix_accounting_valid ? chip->mix_echo_input : 0;",
            self.text,
        )
        self.assertIn(
            "frame.wetPostDelay[0] = chip->mix_accounting_valid ? chip->mix_wet_post_delay[0] : 0;",
            self.text,
        )
        self.assertIn(
            "frame.output[0] = chip->mix_accounting_valid ? chip->out[0] : 0;",
            self.text,
        )

    def test_validity_is_evidence_not_a_second_timeline(self):
        self.assertNotIn("source_sample_index ++", self.text)
        self.assertNotIn("RESMPL_STATE", self.text)


if __name__ == "__main__":
    unittest.main()
