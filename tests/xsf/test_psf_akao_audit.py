from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path
import zlib

from tools.psf_akao_audit import audit_path


def make_akao_v3() -> bytes:
    data = bytearray(0x80)
    data[:4] = b"AKAO"
    struct.pack_into("<H", data, 4, 1)
    struct.pack_into("<H", data, 6, len(data))
    struct.pack_into("<H", data, 0x14, 2)
    struct.pack_into("<I", data, 0x20, 1)
    struct.pack_into("<H", data, 0x40, 0x10)
    return bytes(data)


def make_ps_x_exe(payload: bytes) -> bytes:
    header = bytearray(0x800)
    header[:8] = b"PS-X EXE"
    struct.pack_into("<I", header, 0x10, 0x80010000)
    struct.pack_into("<I", header, 0x18, 0x80010000)
    struct.pack_into("<I", header, 0x1C, len(payload))
    return bytes(header) + payload


def make_psf(payload: bytes) -> bytes:
    program = make_ps_x_exe(payload)
    compressed = zlib.compress(program)
    return b"PSF\x01" + struct.pack(
        "<III", 0, len(compressed), zlib.crc32(compressed) & 0xFFFFFFFF
    ) + compressed


class PsfAkaoAuditTests(unittest.TestCase):
    def test_audit_preserves_structural_only_claims(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "song.psf").write_bytes(make_psf(make_akao_v3()))
            report = audit_path(root)
            self.assertEqual(report["root_count"], 1)
            self.assertEqual(report["accepted_candidate_count"], 1)
            boundary = report["claim_boundary"]
            self.assertTrue(boundary["structural_candidate"])
            self.assertFalse(boundary["exact_akao_version_proven"])
            self.assertFalse(boundary["event_stream_decoded"])
            self.assertFalse(boundary["runtime_use_proven"])
            self.assertFalse(boundary["spu_correspondence_proven"])
            self.assertFalse(boundary["raw_fe13_is_decoded_event"])


if __name__ == "__main__":
    unittest.main()
