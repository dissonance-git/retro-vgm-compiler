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
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x01))
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x40))
        self.assertNotEqual(MODULE.semantics(0x00), MODULE.semantics(0x80))

    def test_exact_driver_quotient_is_128_pairs(self):
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

    def test_modern_authoring_vocabulary_is_stricter_than_driver_decoder(self):
        authored = MODULE.addmusick_authored_values()
        self.assertEqual(len(authored), 84)
        self.assertTrue(all((raw & MODULE.DEAD_BIT) == 0 for raw in authored))
        self.assertEqual(len({MODULE.semantics(raw) for raw in authored}), 84)

        all_semantics = set(MODULE.equivalence_classes())
        authored_semantics = {MODULE.semantics(raw) for raw in authored}
        outside = all_semantics - authored_semantics
        self.assertEqual(len(outside), 44)
        self.assertTrue(all(pan >= 21 for pan, _, _ in outside))

    def test_full_control(self):
        report = MODULE.run_control()
        self.assertEqual(report["raw_state_count"], 256)
        self.assertEqual(report["driver_semantic_class_count"], 128)
        self.assertEqual(report["semantic_class_size"], 2)
        self.assertTrue(report["stable_driver_quotient"])
        self.assertEqual(report["transition_checks"], 128 * 256)
        self.assertEqual(report["modern_addmusick_authoring"]["semantic_states"], 84)
        self.assertEqual(
            report["driver_semantics_outside_modern_authoring"]["semantic_state_count"],
            44,
        )


if __name__ == "__main__":
    unittest.main()
