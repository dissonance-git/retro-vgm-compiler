import importlib.util
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "spc_signed_routing_audit",
    ROOT / "tools" / "spc_signed_routing_audit.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class SpcSignedRoutingAuditTest(unittest.TestCase):
    def make_spc(self, routes):
        size = MODULE.DSP_OFFSET + MODULE.DSP_SIZE
        data = bytearray(size)
        data[: len(MODULE.SPC_SIGNATURE)] = MODULE.SPC_SIGNATURE
        for voice, (left, right) in enumerate(routes):
            base = MODULE.DSP_OFFSET + voice * MODULE.VOICE_STRIDE
            data[base] = left & 0xFF
            data[base + 1] = right & 0xFF
        return bytes(data)

    def test_signed_byte_and_unbalanced_cycle(self):
        self.assertEqual(MODULE.signed8(0x7F), 127)
        self.assertEqual(MODULE.signed8(0x80), -128)
        self.assertEqual(MODULE.signed8(0xFF), -1)

        routes = [
            MODULE.VoiceRoute(0, 1, 1),
            MODULE.VoiceRoute(1, 1, -1),
        ]
        pair = MODULE.analyze_pair(*routes)
        self.assertTrue(pair.full_support)
        self.assertEqual(pair.cycle_product, -1)
        self.assertEqual(pair.determinant, -2)
        self.assertEqual(pair.rank, 2)

    def test_balanced_cycle_can_collapse(self):
        pair = MODULE.analyze_pair(
            MODULE.VoiceRoute(0, 1, -1),
            MODULE.VoiceRoute(1, -1, 1),
        )
        self.assertTrue(pair.full_support)
        self.assertEqual(pair.cycle_product, 1)
        self.assertEqual(pair.determinant, 0)
        self.assertEqual(pair.rank, 1)

    def test_zero_route_has_no_cycle_sign(self):
        pair = MODULE.analyze_pair(
            MODULE.VoiceRoute(0, 1, 0),
            MODULE.VoiceRoute(1, -1, 1),
        )
        self.assertFalse(pair.full_support)
        self.assertIsNone(pair.cycle_product)

    def test_parser_preserves_physical_gain_sign(self):
        data = self.make_spc(
            [(1, -2), (3, 4)] + [(0, 0)] * 6
        )
        routes = MODULE.parse_routes(data)
        self.assertEqual((routes[0].left, routes[0].right), (1, -2))
        self.assertEqual((routes[1].left, routes[1].right), (3, 4))
        self.assertTrue(routes[0].has_negative_gain)

    def test_gun_hazard_saved_snapshots_stay_on_nonnegative_route_manifold(self):
        corpus = ROOT / "tests" / "corpus" / "front-mission-gun-hazard"
        report = MODULE.analyze_collection(corpus)
        self.assertEqual(report["snapshot_count"], 61)
        self.assertEqual(report["negative_gain_snapshot_count"], 0)
        self.assertEqual(report["negative_gain_voice_count"], 0)
        self.assertEqual(report["unbalanced_cycle_pair_count"], 0)


if __name__ == "__main__":
    unittest.main()
