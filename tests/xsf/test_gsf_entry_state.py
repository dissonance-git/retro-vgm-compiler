from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path
import zlib

from components.gsf.gsf import GsfError, build_gsf_effective_image
from components.xsf.envelope import resolve_xsf


def make_xsf(
    *,
    program: bytes,
    tags: tuple[tuple[str, str], ...] = (),
) -> bytes:
    compressed = zlib.compress(program)
    header = b"PSF\x22" + struct.pack(
        "<III", 0, len(compressed), zlib.crc32(compressed) & 0xFFFFFFFF
    )
    tag_section = b""
    if tags:
        tag_section = b"[TAG]" + "".join(
            f"{key}={value}\n" for key, value in tags
        ).encode("latin-1")
    return header + compressed + tag_section


def upload(entry: int, load: int, payload: bytes) -> bytes:
    return struct.pack("<III", entry, load, len(payload)) + payload


class GsfEntryStateTests(unittest.TestCase):
    def test_entry_and_load_must_use_same_gba_address_space(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.gsf"
            path.write_bytes(
                make_xsf(program=upload(0x02000000, 0x08000000, b"ROM"))
            )
            with self.assertRaisesRegex(GsfError, "entry/load address spaces disagree"):
                build_gsf_effective_image(resolve_xsf(path, expected_version=0x22))

    def test_dependency_entry_disagreement_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.gsflib").write_bytes(
                make_xsf(program=upload(0x08000000, 0x08000000, b"BASE"))
            )
            song = root / "song.minigsf"
            song.write_bytes(
                make_xsf(
                    program=upload(0x08000004, 0x08000002, b"xx"),
                    tags=(("_lib", "base.gsflib"),),
                )
            )
            with self.assertRaisesRegex(GsfError, "ambiguous GSF entry state"):
                build_gsf_effective_image(resolve_xsf(song, expected_version=0x22))

    def test_consistent_library_and_root_entries_still_overlay(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.gsflib").write_bytes(
                make_xsf(program=upload(0x08000000, 0x08000000, b"BASE"))
            )
            song = root / "song.minigsf"
            song.write_bytes(
                make_xsf(
                    program=upload(0x08000000, 0x08000002, b"xx"),
                    tags=(("_lib", "base.gsflib"),),
                )
            )
            state = build_gsf_effective_image(resolve_xsf(song, expected_version=0x22))
            self.assertEqual(state.selected_entry_address, 0x08000000)
            self.assertEqual(state.image[:4], b"BAxx")


if __name__ == "__main__":
    unittest.main()
