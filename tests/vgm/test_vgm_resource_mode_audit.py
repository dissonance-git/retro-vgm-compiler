from __future__ import annotations

import pathlib
import struct
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from vgm_resource_mode_audit import audit_bytes  # noqa: E402


def make_vgm(commands: bytes) -> bytes:
    header = bytearray(0x40)
    header[0:4] = b"Vgm "
    struct.pack_into("<I", header, 0x08, 0x00000150)
    raw = bytes(header) + commands + b"\x66"
    raw = bytearray(raw)
    struct.pack_into("<I", raw, 0x04, len(raw) - 4)
    return bytes(raw)


class ResourceModeAuditTests(unittest.TestCase):
    def test_tracks_enable_disable_transitions_at_exact_ticks(self) -> None:
        raw = make_vgm(
            bytes(
                [
                    0x52, 0x2B, 0x80,  # YM2612 DAC on, tick 0
                    0x70,              # wait 1
                    0x52, 0x2B, 0x80,  # repeated on: write, not transition
                    0x71,              # wait 2 -> tick 3
                    0x52, 0x2B, 0x00,  # YM2612 DAC off
                    0x51, 0x0E, 0x20,  # YM2413 rhythm on
                    0x70,              # wait 1 -> tick 4
                    0x51, 0x0E, 0x00,  # YM2413 rhythm off
                    0x5E, 0xBD, 0x20,  # YMF262 rhythm on
                    0x70,              # wait 1 -> tick 5
                    0x5E, 0xBD, 0x00,  # YMF262 rhythm off
                ]
            )
        )
        report = audit_bytes(raw)

        self.assertEqual(report["errors"], [])
        self.assertEqual(report["ym2612_dac"]["write_count"], 3)
        self.assertEqual(report["ym2612_dac"]["state_change_count"], 2)
        self.assertEqual(
            [event["tick"] for event in report["ym2612_dac"]["transitions"]],
            [0, 3],
        )
        self.assertEqual(report["ym2413_rhythm"]["state_change_count"], 2)
        self.assertEqual(
            [event["tick"] for event in report["ym2413_rhythm"]["transitions"]],
            [3, 4],
        )
        self.assertEqual(report["ymf262_rhythm"]["state_change_count"], 2)
        self.assertEqual(
            [event["tick"] for event in report["ymf262_rhythm"]["transitions"]],
            [4, 5],
        )

    def test_second_chip_instance_has_independent_state(self) -> None:
        raw = make_vgm(
            bytes(
                [
                    0x52, 0x2B, 0x80,  # YM2612 instance 0 on
                    0xA2, 0x2B, 0x80,  # YM2612 instance 1 on
                    0xA2, 0x2B, 0x00,  # instance 1 off
                ]
            )
        )
        report = audit_bytes(raw)
        transitions = report["ym2612_dac"]["transitions"]

        self.assertEqual(report["errors"], [])
        self.assertEqual(report["ym2612_dac"]["state_change_count"], 3)
        self.assertEqual(
            [(event["instance"], event["enabled"]) for event in transitions],
            [(0, True), (1, True), (1, False)],
        )

    def test_disabled_reset_writes_do_not_create_fake_transitions(self) -> None:
        raw = make_vgm(
            bytes(
                [
                    0x52, 0x2B, 0x00,
                    0x51, 0x0E, 0x00,
                    0x5E, 0xBD, 0x00,
                ]
            )
        )
        report = audit_bytes(raw)

        self.assertEqual(report["errors"], [])
        for mode_name in ("ym2612_dac", "ym2413_rhythm", "ymf262_rhythm"):
            self.assertEqual(report[mode_name]["write_count"], 1)
            self.assertFalse(report[mode_name]["enabled_seen"])
            self.assertEqual(report[mode_name]["state_change_count"], 0)
            self.assertEqual(report[mode_name]["transitions"], [])


if __name__ == "__main__":
    unittest.main()
