from __future__ import annotations

import importlib.util
import pathlib
import struct
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "vgm_172d_strict", ROOT / "tools" / "vgm_172d_strict.py"
)
assert SPEC is not None and SPEC.loader is not None
STRICT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(STRICT)


def write_u32(raw: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", raw, offset, value)


def make_vgm(*commands: int) -> pathlib.Path:
    raw = bytearray(0x100)
    raw[0:4] = b"Vgm "
    write_u32(raw, 0x08, 0x172)
    write_u32(raw, 0x34, 0x100 - 0x34)
    raw.extend(bytes(commands))
    if not commands or commands[-1] != 0x66:
        raw.append(0x66)
    write_u32(raw, 0x04, len(raw) - 4)
    directory = pathlib.Path(tempfile.mkdtemp(prefix="vgm-172d-strict-"))
    path = directory / "fixture.vgm"
    path.write_bytes(raw)
    return path


class Vgm172dStrictTest(unittest.TestCase):
    def test_current_reserved_header_bytes_are_zero(self) -> None:
        path = make_vgm(0x66)
        report = STRICT.strict_audit(path)
        self.assertTrue(report["strict_172d"], report["strict_errors"])

    def test_reserved_header_byte_is_rejected(self) -> None:
        path = make_vgm(0x66)
        raw = bytearray(path.read_bytes())
        raw[0xD7] = 0x01
        path.write_bytes(raw)
        report = STRICT.strict_audit(path)
        self.assertFalse(report["valid"])
        self.assertTrue(any("0xD7" in error for error in report["strict_errors"]))

    def test_reserved_flag_bits_are_rejected(self) -> None:
        path = make_vgm(0x66)
        raw = bytearray(path.read_bytes())
        raw[0x79] = 0x80
        path.write_bytes(raw)
        report = STRICT.strict_audit(path)
        self.assertFalse(report["strict_172d"])
        self.assertTrue(any("AY8910" in error for error in report["strict_errors"]))

    def test_header_overlap_is_logically_zero_not_sound_data(self) -> None:
        raw = bytearray(0x80)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x171)
        # data begins at 0x80, so later nominal header bytes overlap sound data
        write_u32(raw, 0x34, 0x80 - 0x34)
        raw.extend(bytes([0xE2, 0xFF, 0xFF, 0xFF, 0xFF, 0x66]))
        write_u32(raw, 0x04, len(raw) - 4)
        directory = pathlib.Path(tempfile.mkdtemp(prefix="vgm-overlap-"))
        path = directory / "overlap.vgm"
        path.write_bytes(raw)
        report = STRICT.strict_audit(path)
        self.assertTrue(report["strict_172d"], report["strict_errors"])

    def test_every_fixed_width_family_matches_the_closure_table(self) -> None:
        for command in range(256):
            expected = STRICT.expected_fixed_operand_count(command)
            actual = STRICT._BASE.fixed_operand_count(command, STRICT._BASE.VGM_172_BETA)
            if command in (0x67, 0x68):
                self.assertIsNone(actual)
                continue
            self.assertEqual(
                actual,
                expected,
                f"opcode 0x{command:02X} fixed width drifted",
            )

    def test_reserved_command_families_are_walked_not_misdecoded(self) -> None:
        # One member from each mandated reserved family, then an end marker.
        path = make_vgm(
            0x32, 0xAA,
            0x41, 0xAA, 0xBB,
            0xC9, 0x01, 0x02, 0x03,
            0xD7, 0x04, 0x05, 0x06,
            0xE2, 0x07, 0x08, 0x09, 0x0A,
            0x66,
        )
        report = STRICT.strict_audit(path)
        self.assertTrue(report["valid"], report["errors"])
        for opcode in ("32", "41", "C9", "D7", "E2"):
            self.assertEqual(report["command_counts"][opcode], 1)

    def test_newer_unknown_version_reopens_closure(self) -> None:
        path = make_vgm(0x66)
        raw = bytearray(path.read_bytes())
        write_u32(raw, 0x08, 0x173)
        path.write_bytes(raw)
        report = STRICT.strict_audit(path)
        self.assertFalse(report["strict_172d"])
        self.assertTrue(any("newer than" in error for error in report["strict_errors"]))


if __name__ == "__main__":
    unittest.main()
