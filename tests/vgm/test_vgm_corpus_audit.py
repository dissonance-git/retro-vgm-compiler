from __future__ import annotations

import importlib.util
import pathlib
import struct
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "vgm_corpus_audit", ROOT / "tools" / "vgm_corpus_audit.py"
)
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


def write_u32(raw: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", raw, offset, value)


def make_file(raw: bytes, name: str) -> pathlib.Path:
    directory = pathlib.Path(tempfile.mkdtemp(prefix="vgm-audit-"))
    path = directory / name
    path.write_bytes(raw)
    return path


class VgmCorpusAuditTest(unittest.TestCase):
    def test_172d_mikey_transport_surface(self) -> None:
        raw = bytearray(0x100)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x172)
        write_u32(raw, 0x34, 0x100 - 0x34)
        write_u32(raw, 0xE4, 16_000_000)
        raw.extend(
            bytes(
                [
                    0x40, 0x10, 0x20,
                    0x67, 0x66, 0x08, 0x02, 0x00, 0x00, 0x00, 0xAA, 0xBB,
                    0x61, 100, 0,
                    0x66,
                ]
            )
        )
        write_u32(raw, 0x18, 100)
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "mikey.vgm"))

        self.assertTrue(report["valid"], report["errors"])
        self.assertEqual(report["version"], "1.72d")
        self.assertIn("upstream VGM 1.72 beta", report["version_provenance"])
        self.assertTrue(report["eof_matches_file"])
        self.assertEqual(report["computed_total_samples"], 100)
        self.assertEqual(report["command_counts"]["40"], 1)
        self.assertEqual(report["data_block_counts"]["08"], 1)
        self.assertEqual(report["data_blocks"][0]["name"], "Mikey PCM")
        self.assertEqual(report["declared_chips"][0]["chip"], "Mikey")

    def test_170_extra_header_and_gd3(self) -> None:
        raw = bytearray(0x130)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x170)
        write_u32(raw, 0x34, 0x130 - 0x34)
        write_u32(raw, 0xBC, 0x100 - 0xBC)

        write_u32(raw, 0x100, 12)
        write_u32(raw, 0x104, 0x10C - 0x104)
        raw[0x10C] = 1
        raw[0x10D] = 2
        write_u32(raw, 0x10E, 7_670_454)

        write_u32(raw, 0x108, 0x113 - 0x108)
        raw[0x113] = 1
        raw[0x114] = 2
        raw[0x115] = 1
        struct.pack_into("<H", raw, 0x116, 0x8100)

        raw.extend(bytes([0x62, 0x66]))
        write_u32(raw, 0x18, 735)

        gd3_offset = len(raw)
        payload = b"\x00\x00" * 11
        raw.extend(b"Gd3 " + struct.pack("<II", 0x100, len(payload)) + payload)
        write_u32(raw, 0x14, gd3_offset - 0x14)
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "extra.vgm"))

        self.assertTrue(report["valid"], report["errors"])
        self.assertTrue(report["eof_matches_file"])
        self.assertEqual(report["gd3"]["field_count"], 11)
        self.assertEqual(report["extra_header"]["chip_clocks"][0]["chip_id"], 2)
        self.assertEqual(report["extra_header"]["chip_clocks"][0]["clock_hz"], 7_670_454)
        volume = report["extra_header"]["chip_volumes"][0]
        self.assertTrue(volume["second_chip"])
        self.assertTrue(volume["relative"])

    def test_bad_eof_is_not_silently_accepted(self) -> None:
        raw = bytearray(0x40)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x150)
        write_u32(raw, 0x34, 0x0C)
        raw.extend(bytes([0x66]))
        write_u32(raw, 0x04, len(raw) - 5)

        report = AUDIT.audit(make_file(raw, "bad-eof.vgm"))
        self.assertFalse(report["valid"])
        self.assertTrue(any("EOF offset" in error for error in report["errors"]))

    def test_gd3_surplus_null_padding_is_visible_but_not_a_field(self) -> None:
        raw = bytearray(0x40)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x150)
        write_u32(raw, 0x34, 0x0C)
        raw.extend(bytes([0x66]))

        gd3_offset = len(raw)
        fields = ["track", "", "game", "", "system", "", "author", "", "1994", "ripper", "notes"]
        payload = ("\x00".join(fields) + "\x00\x00\x00").encode("utf-16-le")
        raw.extend(b"Gd3 " + struct.pack("<II", 0x100, len(payload)) + payload)
        write_u32(raw, 0x14, gd3_offset - 0x14)
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "padded-gd3.vgm"))

        self.assertTrue(report["valid"], report["errors"])
        self.assertEqual(report["gd3"]["field_count"], 11)
        self.assertEqual(report["gd3"]["extra_null_padding_fields"], 2)
        self.assertTrue(any("surplus empty field terminator" in warning for warning in report["warnings"]))

    def test_gd3_surplus_nonempty_field_remains_invalid(self) -> None:
        raw = bytearray(0x40)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x150)
        write_u32(raw, 0x34, 0x0C)
        raw.extend(bytes([0x66]))

        gd3_offset = len(raw)
        payload = ("\x00".join([""] * 11 + ["unexpected"]) + "\x00").encode("utf-16-le")
        raw.extend(b"Gd3 " + struct.pack("<II", 0x100, len(payload)) + payload)
        write_u32(raw, 0x14, gd3_offset - 0x14)
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "extra-field-gd3.vgm"))

        self.assertFalse(report["valid"])
        self.assertTrue(any("GD3 field count" in error for error in report["errors"]))

    def test_invalid_gd3_utf16_is_reported_without_crashing(self) -> None:
        raw = bytearray(0x40)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x150)
        write_u32(raw, 0x34, 0x0C)
        raw.extend(bytes([0x66]))

        gd3_offset = len(raw)
        payload = b"\x00\xD8"
        raw.extend(b"Gd3 " + struct.pack("<II", 0x100, len(payload)) + payload)
        write_u32(raw, 0x14, gd3_offset - 0x14)
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "invalid-utf16-gd3.vgm"))

        self.assertFalse(report["valid"])
        self.assertTrue(any("invalid GD3 UTF-16LE" in error for error in report["errors"]))

    def test_unknown_rom_block_is_skipped_but_visible(self) -> None:
        raw = bytearray(0x100)
        raw[0:4] = b"Vgm "
        write_u32(raw, 0x08, 0x171)
        write_u32(raw, 0x34, 0x100 - 0x34)
        raw.extend(bytes([0x67, 0x66, 0xBF, 0, 0, 0, 0, 0x66]))
        write_u32(raw, 0x04, len(raw) - 4)

        report = AUDIT.audit(make_file(raw, "unknown-rom.vgm"))
        self.assertTrue(report["valid"], report["errors"])
        self.assertEqual(report["data_blocks"][0]["category"], "rom_ram_image")
        self.assertFalse(report["data_blocks"][0]["defined"])
        self.assertTrue(any("unknown ROM/RAM" in warning for warning in report["warnings"]))


if __name__ == "__main__":
    unittest.main()
