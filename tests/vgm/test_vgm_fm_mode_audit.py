import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[2] / "tools"
sys.path.insert(0, str(TOOLS))
SPEC = importlib.util.spec_from_file_location("vgm_fm_mode_audit", TOOLS / "vgm_fm_mode_audit.py")
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def make_vgm(commands: bytes) -> bytes:
    raw = bytearray(0x40)
    raw[0:4] = b"Vgm "
    struct.pack_into("<I", raw, 0x08, 0x151)
    raw.extend(commands)
    struct.pack_into("<I", raw, 0x04, len(raw) - 4)
    return bytes(raw)


def ym(command, address, data):
    return bytes((command, address, data))


class FmModeAuditTest(unittest.TestCase):
    def test_opn_effect_mode_and_special_frequency_are_separate_evidence(self):
        raw = make_vgm(
            ym(0x52, 0x27, 0x40)
            + ym(0x52, 0xAC, 0x22)
            + ym(0x52, 0xA8, 0x34)
            + bytes((0x66,))
        )
        report = MODULE.audit_bytes(raw)
        self.assertEqual(report["opn"]["mode_values"], [1])
        self.assertTrue(report["opn"]["effect_mode_seen"])
        self.assertFalse(report["opn"]["csm_exact_seen"])
        self.assertEqual(report["opn"]["special_frequency_write_count"], 2)
        self.assertEqual(report["opn"]["special_frequency_writes_while_effect_enabled"], 2)

    def test_opn_csm_value_is_retained_without_flattening(self):
        raw = make_vgm(ym(0x56, 0x27, 0x80) + bytes((0x66,)))
        report = MODULE.audit_bytes(raw)
        self.assertEqual(report["opn"]["mode_values"], [2])
        self.assertTrue(report["opn"]["effect_mode_seen"])
        self.assertTrue(report["opn"]["csm_exact_seen"])
        self.assertFalse(report["opn"]["three_slot_bit_seen"])

    def test_opl3_programmed_pair_mask_is_not_active_before_new_mode(self):
        raw = make_vgm(
            ym(0x5F, 0x04, 0x05)
            + ym(0x5F, 0x05, 0x00)
            + bytes((0x66,))
        )
        report = MODULE.audit_bytes(raw)
        self.assertEqual(report["opl3"]["programmed_four_op_mask_values"], [5])
        self.assertFalse(report["opl3"]["active_four_op_seen"])

    def test_opl3_pair_mask_becomes_active_when_new_mode_turns_on(self):
        raw = make_vgm(
            ym(0x5F, 0x04, 0x05)
            + ym(0x5F, 0x05, 0x01)
            + bytes((0x66,))
        )
        report = MODULE.audit_bytes(raw)
        self.assertTrue(report["opl3"]["new_mode_seen"])
        self.assertTrue(report["opl3"]["active_four_op_seen"])
        self.assertEqual(report["opl3"]["active_four_op_mask_values"], [5])

    def test_same_complete_patch_can_be_reused_at_distinct_pitches(self):
        commands = bytearray()
        # Program one complete 4-op patch on YM2612 channel 0.
        for group in range(0x30, 0xA0, 0x10):
            for slot_offset in (0x00, 0x04, 0x08, 0x0C):
                commands += ym(0x52, group + slot_offset, (group + slot_offset) & 0x7F)
        commands += ym(0x52, 0xB0, 0x32)

        # Pitch A, then key on all operators.
        commands += ym(0x52, 0xA4, 0x22)
        commands += ym(0x52, 0xA0, 0x34)
        commands += ym(0x52, 0x28, 0xF0)
        commands += ym(0x52, 0x28, 0x00)

        # Same static patch, different pitch, then key on again.
        commands += ym(0x52, 0xA4, 0x2A)
        commands += ym(0x52, 0xA0, 0x78)
        commands += ym(0x52, 0x28, 0xF0)
        commands += bytes((0x66,))

        report = MODULE.audit_bytes(make_vgm(bytes(commands)))
        self.assertEqual(report["opn"]["complete_patch_key_on_count"], 2)
        self.assertEqual(report["opn"]["distinct_complete_patch_count"], 1)
        self.assertEqual(report["opn"]["complete_patch_reused_at_distinct_pitches"], 1)


if __name__ == "__main__":
    unittest.main()
