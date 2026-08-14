import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "spc_driver_routing_reachability",
    ROOT / "tools" / "spc_driver_routing_reachability.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class SpcDriverRoutingReachabilityTest(unittest.TestCase):
    def test_gun_hazard_ordinary_pan_cannot_reach_negative_gain(self):
        for level in range(0x80):
            for pan in range(0x100):
                left, right = MODULE.gun_hazard_unsigned_route(level, pan)
                self.assertGreaterEqual(left, 0)
                self.assertGreaterEqual(right, 0)
                self.assertLessEqual(left, 0x7F)
                self.assertLessEqual(right, 0x7F)

    def test_wolf_team_ad_reaches_all_four_phase_quadrants(self):
        observed = {
            MODULE.apply_wolfteam_phase(40, 24, MODULE.wolfteam_ad_flags(argument))
            for argument in range(4)
        }
        self.assertEqual(
            observed,
            {(40, 24), (40, -24), (-40, 24), (-40, -24)},
        )

    def test_wolf_team_can_reach_unbalanced_two_voice_cycle(self):
        voice_a = MODULE.apply_wolfteam_phase(40, 24, MODULE.wolfteam_ad_flags(0))
        voice_b = MODULE.apply_wolfteam_phase(40, 24, MODULE.wolfteam_ad_flags(1))
        self.assertEqual(MODULE.cycle_product(voice_a, voice_b), -1)
        self.assertNotEqual(MODULE.determinant(voice_a, voice_b), 0)

    def test_nspc_e1_reaches_all_four_phase_quadrants(self):
        observed = {
            MODULE.apply_nspc_phase(40, 24, raw)
            for raw in (0x0A, 0x4A, 0x8A, 0xCA)
        }
        self.assertEqual(
            observed,
            {(40, 24), (40, -24), (-40, 24), (-40, -24)},
        )

    def test_capcom_megaman_x_final_pan_stage_never_sets_sign_bit(self):
        for magnitude in range(0x80):
            for level in range(0x100):
                gain = MODULE.capcom_final_route_gain(magnitude, level)
                self.assertGreaterEqual(gain, 0)
                self.assertLessEqual(gain, 0x7E)

    def test_konami_axelay_final_pan_stage_never_sets_sign_bit(self):
        for magnitude in range(0x80):
            for level in range(0x80):
                gain = MODULE.konami_final_route_gain(magnitude, level)
                self.assertGreaterEqual(gain, 0)
                self.assertLessEqual(gain, 0x7F)

    def test_full_control(self):
        report = MODULE.run_control()
        self.assertFalse(report["square_akao_gun_hazard"]["signed_route_reachable"])
        self.assertTrue(report["wolf_team"]["all_four_sign_quadrants_reachable"])
        self.assertTrue(report["wolf_team"]["unbalanced_two_voice_cycle_reachable"])
        self.assertTrue(report["nintendo_nspc"]["all_four_sign_quadrants_reachable"])
        self.assertTrue(report["nintendo_nspc"]["unbalanced_two_voice_cycle_reachable"])
        self.assertFalse(report["capcom_megaman_x"]["signed_route_reachable_through_ordinary_pan"])
        self.assertFalse(report["konami_axelay"]["signed_route_reachable_through_ordinary_pan"])


if __name__ == "__main__":
    unittest.main()
