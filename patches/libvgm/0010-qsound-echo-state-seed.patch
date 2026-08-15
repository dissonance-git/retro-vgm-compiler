from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "0010-qsound-echo-state-seed.patch"


class QSoundEchoSeedPatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = PATCH.read_text(encoding="utf-8")

    def test_runtime_echo_parameters_are_small_per_tick_witnesses(self):
        self.assertIn("INT16 echoFeedback;", self.text)
        self.assertIn("UINT16 echoLength;", self.text)
        self.assertIn("chip->echo.feedback", self.text)
        self.assertIn("chip->echo.length", self.text)

    def test_seed_contains_only_recurrence_memory(self):
        self.assertIn("INT16 lastSample;", self.text)
        self.assertIn("INT16 delayLine[1024];", self.text)
        self.assertIn("INT16 delayPos;", self.text)
        seed = self.text[self.text.find("typedef struct _qsound_ctr_echo_seed"):]
        seed = seed[: seed.find("} QSOUND_CTR_ECHO_SEED;")]
        self.assertNotIn("feedback", seed.lower())
        self.assertNotIn("length", seed.lower())

    def test_snapshot_is_on_demand_not_in_realtime_callback(self):
        self.assertIn("qsoundc_get_echo_seed", self.text)
        update = self.text[self.text.find("static void qsoundc_update"):]
        update = update[: update.find("void qsoundc_set_mix_callback")]
        self.assertNotIn("memcpy(seed->delayLine", update)
        self.assertEqual(self.text.count("memcpy(seed->delayLine"), 2)

    def test_only_superctr_seed_is_exposed_through_player(self):
        self.assertIn("devDef->coreID != FCC_CTR_", self.text)
        self.assertIn("GetQSoundEchoSeed", self.text)

    def test_patch_does_not_touch_historical_output_assignments(self):
        self.assertNotIn("outputs[0][curSmpl]", self.text)
        self.assertNotIn("outputs[1][curSmpl]", self.text)


if __name__ == "__main__":
    unittest.main()
