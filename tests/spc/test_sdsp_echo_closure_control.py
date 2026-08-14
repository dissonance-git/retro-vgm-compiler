import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "sdsp_echo_closure_control",
    ROOT / "tools" / "sdsp_echo_closure_control.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class SdspEchoClosureControlTest(unittest.TestCase):
    def test_integer_convolution_support(self):
        self.assertEqual(MODULE.convolve_integer([1, 1], [1, 1]), [1, 2, 1])
        self.assertEqual(MODULE.support_indices([1, 1, 0, 0, 0, 0, 0, 0], 2), [0, 1, 2])

    def test_single_tap_never_broadens_symbolic_support(self):
        rows = MODULE.support_summary([126, 0, 0, 0, 0, 0, 0, 0])
        self.assertEqual([row["nonzero_support_count"] for row in rows], [1] * 7)

    def test_nspc_multitap_presets_broaden(self):
        preset1 = MODULE.support_summary(MODULE.NSPC_FIR_PRESETS[1])
        preset2 = MODULE.support_summary(MODULE.NSPC_FIR_PRESETS[2])
        preset3 = MODULE.support_summary(MODULE.NSPC_FIR_PRESETS[3])
        self.assertEqual(
            [row["nonzero_support_count"] for row in preset1],
            [8, 15, 22, 29, 36, 43, 50],
        )
        self.assertEqual(
            [row["nonzero_support_count"] for row in preset2],
            [8, 15, 22, 29, 36, 43, 50],
        )
        self.assertEqual(
            [row["nonzero_support_count"] for row in preset3],
            [7, 15, 22, 29, 36, 43, 50],
        )

    def test_real_gun_hazard_snapshot_is_single_tap_control(self):
        state = MODULE.parse_echo_state(
            ROOT
            / "tests"
            / "corpus"
            / "front-mission-gun-hazard"
            / "1.33 - Naval Fortress.spc"
        )
        self.assertEqual(state["efb"], 43)
        self.assertEqual(state["eon"], 0x6F)
        self.assertEqual(state["edl"], 7)
        self.assertEqual(state["fir"], [126, 0, 0, 0, 0, 0, 0, 0])
        self.assertEqual(state["delay_sample_frames"], 3584)
        self.assertAlmostEqual(state["nominal_delay_ms"], 112.0)

    def test_full_control(self):
        report = MODULE.run_control(ROOT)
        self.assertFalse(report["result"]["single_tap_feedback_broadens_support"])
        self.assertTrue(report["result"]["nspc_preset_1_reaches_full_7k_plus_1_support"])
        self.assertTrue(report["result"]["nspc_preset_2_reaches_full_7k_plus_1_support"])
        self.assertTrue(report["result"]["nspc_preset_3_reaches_full_support_from_generation_2"])


if __name__ == "__main__":
    unittest.main()
