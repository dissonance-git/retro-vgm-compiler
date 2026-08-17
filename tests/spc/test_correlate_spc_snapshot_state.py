from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "correlate_spc_snapshot_state.py"
SPEC = importlib.util.spec_from_file_location("correlate_spc_snapshot_state", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
correlate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = correlate
SPEC.loader.exec_module(correlate)


class SpcSnapshotStateCorrelationTest(unittest.TestCase):
    @staticmethod
    def make_spc(ordinal: int) -> bytes:
        data = bytearray(correlate.SPC_MIN_FILE_SIZE)
        data[: len(correlate.SPC_SIGNATURE_PREFIX)] = correlate.SPC_SIGNATURE_PREFIX

        # Deliberate metadata/header poison. The scanner must never look here.
        data[0x2C] = ordinal
        data[0x2D] = ordinal - 1

        ram = correlate.SPC_RAM_OFFSET
        data[ram + 0x1234] = ordinal
        data[ram + 0x2345] = ordinal - 1
        data[ram + 0x3456] = (ordinal + 0x40) & 0xFF

        # Keep the surrounding control blocks stable so the true state loci
        # receive the strongest local-stability evidence.
        for delta in range(-8, 9):
            if delta != 0:
                data[ram + 0x1234 + delta] = 0xA5
                data[ram + 0x2345 + delta] = 0x5A
        return bytes(data)

    def test_recovers_order_tracking_ram_state_without_header_leakage(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for ordinal in range(1, 26):
                (root / f"{ordinal:02d} - BGM {ordinal}.spc").write_bytes(
                    self.make_spc(ordinal)
                )

            snapshots = correlate.load_ordered_snapshots(
                root,
                expected_count=25,
            )
            result = correlate.correlate(snapshots, top=32)
            exact = {
                (item["space"], item["location"]): item
                for item in result["exact_sequence_candidates"]
            }

            self.assertIn(("ram_u8", "0x1234"), exact)
            self.assertTrue(exact[("ram_u8", "0x1234")]["exact_one_based"])
            self.assertEqual(
                exact[("ram_u8", "0x1234")]["values"],
                list(range(1, 26)),
            )
            self.assertEqual(exact[("ram_u8", "0x1234")]["neighbor_stability"], 1.0)

            self.assertIn(("ram_u8", "0x2345"), exact)
            self.assertTrue(exact[("ram_u8", "0x2345")]["exact_zero_based"])
            self.assertEqual(
                exact[("ram_u8", "0x2345")]["values"],
                list(range(25)),
            )

            self.assertIn(("ram_u8", "0x3456"), exact)
            self.assertTrue(exact[("ram_u8", "0x3456")]["exact_additive"])
            self.assertEqual(exact[("ram_u8", "0x3456")]["best_additive_delta_modulo"], 0x40)

            self.assertTrue(result["summary"]["sequential_selector_locus_found"])
            locations = {item["location"] for item in result["exact_sequence_candidates"]}
            self.assertNotIn("0x002C", locations)
            self.assertNotIn("0x002D", locations)

    def test_rejects_noncontiguous_fixture_ordinals(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            (root / "01 - BGM 1.spc").write_bytes(self.make_spc(1))
            (root / "03 - BGM 3.spc").write_bytes(self.make_spc(3))
            with self.assertRaisesRegex(ValueError, "contiguous"):
                correlate.load_ordered_snapshots(root)

    def test_real_monster_maker_intake_is_a_25_snapshot_contiguous_pack(self):
        intake = ROOT / "imports" / "sonic3-attribution" / "monster-maker-3-spc"
        snapshots = correlate.load_ordered_snapshots(intake, expected_count=25)
        self.assertEqual([item.ordinal for item in snapshots], list(range(1, 26)))
        self.assertTrue(all(item.path.suffix.lower() == ".spc" for item in snapshots))


if __name__ == "__main__":
    unittest.main()
