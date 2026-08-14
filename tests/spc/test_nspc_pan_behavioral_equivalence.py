import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "nspc_pan_behavioral_equivalence",
    ROOT / "tools" / "nspc_pan_behavioral_equivalence.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class NspcPanBehavioralEquivalenceTest(unittest.TestCase):
    def test_bit_five_does_not_change_pan_or_phase_semantics(self):
        for raw in range(0x100):
            self.assertEqual(
                MODULE.semantics(raw),
                MODULE.semantics(raw ^ MODULE.DEAD_BIT),
            )

    def test_other_declared_pan_coordinates_are_live(self):
        # One scalar-pan bit, one right-phase bit, and one left-phase bit all
        # change the declared semantic projection.
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x01))
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x40))
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x80))

    def test_exact_quotient_is_128_pairs(self):
        classes = MODULE.equivalence_classes()
        self.assertEqual(len(classes), 128)
        self.assertTrue(all(len(members) == 2 for members in classes.values()))
        self.assertTrue(
            all((members[0] ^ members[1]) == MODULE.DEAD_BIT for members in classes.values())
        )

    def test_future_e1_assignments_cannot_reveal_dead_bit(self):
        classes = MODULE.equivalence_classes()
        for members in classes.values():
            a, b = members
            for action in range(0x100):
                self.assertEqual(
                    MODULE.semantics(MODULE.overwrite(a, action)),
                    MODULE.semantics(MODULE.overwrite(b, action)),
                )

    def test_full_control(self):
        report = MODULE.run_control()
        self.assertEqual(report["raw_state_count"], 256)
        self.assertEqual(report["semantic_class_count"], 128)
        self.assertEqual(report["class_size"], 2)
        self.assertTrue(report["stable_quotient"])
        self.assertEqual(report["transition_checks"], 128 * 256)


if __name__ == "__main__":
    unittest.main()
