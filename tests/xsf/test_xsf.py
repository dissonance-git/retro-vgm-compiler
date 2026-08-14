from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path
import zlib

from components.psf.psf1 import Psf1Error, build_psf1_effective_image
from components.twosf.twosf import TwoSfError, build_twosf_effective_state
from components.usf.usf import UsfError, build_usf_effective_state
from components.xsf.envelope import XsfDependencyError, XsfError, parse_xsf, resolve_xsf


def make_xsf(
    version: int,
    *,
    program: bytes = b"",
    reserved: bytes = b"",
    tags: tuple[tuple[str, str], ...] = (),
) -> bytes:
    compressed = zlib.compress(program) if program else b""
    header = b"PSF" + bytes((version,)) + struct.pack(
        "<III", len(reserved), len(compressed), zlib.crc32(compressed) & 0xFFFFFFFF
    )
    tag_section = b""
    if tags:
        tag_section = b"[TAG]" + "".join(f"{key}={value}\n" for key, value in tags).encode(
            "latin-1"
        )
    return header + reserved + compressed + tag_section


def make_ps_x_exe(text_start: int, payload: bytes, *, pc: int = 0, sp: int = 0) -> bytes:
    header = bytearray(0x800)
    header[:8] = b"PS-X EXE"
    struct.pack_into("<I", header, 0x10, pc)
    struct.pack_into("<I", header, 0x18, text_start)
    struct.pack_into("<I", header, 0x1C, len(payload))
    struct.pack_into("<I", header, 0x30, sp)
    header[0x4C:0x55] = b"Synthetic"
    return bytes(header) + payload


def make_twosf_map(offset: int, payload: bytes) -> bytes:
    return struct.pack("<II", offset, len(payload)) + payload


def make_save_record(offset: int, payload: bytes) -> bytes:
    compressed = zlib.compress(make_twosf_map(offset, payload))
    return b"SAVE" + struct.pack("<II", len(compressed), zlib.crc32(compressed) & 0xFFFFFFFF) + compressed


def make_sr64_table(patches: tuple[tuple[int, bytes], ...]) -> bytes:
    result = bytearray(b"SR64")
    for offset, payload in patches:
        result += struct.pack("<II", len(payload), offset) + payload
    result += struct.pack("<I", 0)
    return bytes(result)


def make_usf_reserved(
    rom: tuple[tuple[int, bytes], ...] = (),
    save: tuple[tuple[int, bytes], ...] = (),
) -> bytes:
    return make_sr64_table(rom) + make_sr64_table(save)


class EnvelopeTests(unittest.TestCase):
    def test_header_version_tags_and_crc(self) -> None:
        data = make_xsf(
            0x24,
            program=b"payload",
            reserved=b"reserved",
            tags=(("ALBUM ARTIST", "Nintendo"), ("_lib", "base.2sflib")),
        )
        obj = parse_xsf(data, source_id="song.mini2sf", expected_version=0x24)
        self.assertEqual(obj.program, b"payload")
        self.assertEqual(obj.reserved, b"reserved")
        self.assertEqual(obj.tag_values("album artist"), ("Nintendo",))
        self.assertEqual(obj.library_directives(), (("_lib", "base.2sflib"),))
        with self.assertRaises(XsfError):
            parse_xsf(data, expected_version=0x21)

    def test_rejects_truncation_crc_and_extra_zlib_stream(self) -> None:
        with self.assertRaises(XsfError):
            parse_xsf(b"PSF\x01")
        data = bytearray(make_xsf(1, program=b"abc"))
        data[12] ^= 0x01
        with self.assertRaises(XsfError):
            parse_xsf(bytes(data))
        compressed = zlib.compress(b"one") + zlib.compress(b"two")
        data = b"PSF\x01" + struct.pack("<III", 0, len(compressed), zlib.crc32(compressed)) + compressed
        with self.assertRaises(XsfError):
            parse_xsf(data)

    def test_dependency_order_missing_cycle_and_provenance_are_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.psflib").write_bytes(
                make_xsf(1, program=make_ps_x_exe(0x80010000, b"BASE"))
            )
            (root / "second.psflib").write_bytes(
                make_xsf(1, program=make_ps_x_exe(0x80010002, b"22"))
            )
            song = root / "song.minipsf"
            song.write_bytes(
                make_xsf(
                    1,
                    program=make_ps_x_exe(0x80010001, b"rr"),
                    tags=(("_lib", "base.psflib"), ("_lib2", "second.psflib")),
                )
            )
            resolved = resolve_xsf(song, expected_version=1)
            self.assertEqual([obj.source_id for obj in resolved.objects], ["base.psflib", "song.minipsf", "second.psflib"])
            first = build_psf1_effective_image(resolved)
            second = build_psf1_effective_image(resolve_xsf(song, expected_version=1))
            self.assertEqual(first.memory, b"Br22")
            self.assertEqual(first, second)
            self.assertEqual(first.provenance_at_address(0x80010002).source_id, "second.psflib")

            (root / "missing.minipsf").write_bytes(make_xsf(1, tags=(("_lib", "nope.psflib"),)))
            with self.assertRaises(XsfDependencyError):
                resolve_xsf(root / "missing.minipsf")
            (root / "a.psflib").write_bytes(make_xsf(1, tags=(("_lib", "b.psflib"),)))
            (root / "b.psflib").write_bytes(make_xsf(1, tags=(("_lib", "a.psflib"),)))
            with self.assertRaises(XsfDependencyError):
                resolve_xsf(root / "a.psflib")

    def test_library_paths_are_relative_to_each_containing_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "libs" / "common").mkdir(parents=True)
            (root / "patches").mkdir()

            (root / "libs" / "common" / "driver.psflib").write_bytes(
                make_xsf(1, program=make_ps_x_exe(0x80010000, b"AAAA"))
            )
            (root / "libs" / "base.psflib").write_bytes(
                make_xsf(
                    1,
                    program=make_ps_x_exe(0x80010001, b"bb"),
                    # The format treats both slash styles as separators and
                    # resolves this from libs/base.psflib, not song.minipsf.
                    tags=(("_lib", "common\\driver.psflib"),),
                )
            )
            (root / "patches" / "second.psflib").write_bytes(
                make_xsf(1, program=make_ps_x_exe(0x80010003, b"2"))
            )
            song = root / "song.minipsf"
            song.write_bytes(
                make_xsf(
                    1,
                    program=make_ps_x_exe(0x80010002, b"R"),
                    tags=(("_lib", "libs/base.psflib"), ("_lib2", "patches/second.psflib")),
                )
            )

            resolved = resolve_xsf(song, expected_version=1)
            self.assertEqual(
                [obj.source_id for obj in resolved.objects],
                [
                    "libs/common/driver.psflib",
                    "libs/base.psflib",
                    "song.minipsf",
                    "patches/second.psflib",
                ],
            )
            self.assertEqual(
                [(edge.parent, edge.tag, edge.child) for edge in resolved.edges],
                [
                    ("song.minipsf", "_lib", "libs/base.psflib"),
                    ("libs/base.psflib", "_lib", "libs/common/driver.psflib"),
                    ("song.minipsf", "_lib2", "patches/second.psflib"),
                ],
            )
            state = build_psf1_effective_image(resolved)
            self.assertEqual(state.memory, b"AbR2")
            self.assertEqual(
                state.provenance_at_address(0x80010000).source_id,
                "libs/common/driver.psflib",
            )

    def test_nested_parent_reference_can_stay_inside_selected_tree(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "libs").mkdir()
            (root / "shared.psflib").write_bytes(
                make_xsf(1, program=make_ps_x_exe(0x80010000, b"S"))
            )
            (root / "libs" / "base.psflib").write_bytes(
                make_xsf(1, tags=(("_lib", "../shared.psflib"),))
            )
            song = root / "song.minipsf"
            song.write_bytes(make_xsf(1, tags=(("_lib", "libs/base.psflib"),)))

            resolved = resolve_xsf(song, expected_version=1)
            self.assertEqual(
                [obj.source_id for obj in resolved.objects],
                ["shared.psflib", "libs/base.psflib", "song.minipsf"],
            )

    def test_dependency_escape_and_absolute_paths_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            root = workspace / "set"
            root.mkdir()
            (workspace / "outside.psflib").write_bytes(make_xsf(1))

            escaped = root / "escaped.minipsf"
            escaped.write_bytes(make_xsf(1, tags=(("_lib", "../outside.psflib"),)))
            with self.assertRaises(XsfDependencyError):
                resolve_xsf(escaped, expected_version=1)

            absolute = root / "absolute.minipsf"
            absolute.write_bytes(make_xsf(1, tags=(("_lib", "/tmp/absolute.psflib"),)))
            with self.assertRaises(XsfDependencyError):
                resolve_xsf(absolute, expected_version=1)

            windows_absolute = root / "windows-absolute.minipsf"
            windows_absolute.write_bytes(make_xsf(1, tags=(("_lib", "C:\\Music\\base.psflib"),)))
            with self.assertRaises(XsfDependencyError):
                resolve_xsf(windows_absolute, expected_version=1)


class PlatformTests(unittest.TestCase):
    def test_psf1_reconstructs_ps_x_exe_but_not_runtime(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            song = Path(directory) / "song.psf"
            song.write_bytes(make_xsf(1, program=make_ps_x_exe(0x80010000, b"PSX", pc=0x80010000)))
            state = build_psf1_effective_image(resolve_xsf(song, expected_version=1))
            self.assertEqual(state.memory, b"PSX")
            self.assertEqual(state.entry.initial_pc, 0x80010000)
            self.assertFalse(state.runtime_available)
            with self.assertRaises(Psf1Error):
                build_psf1_effective_image(resolve_xsf(song, expected_version=1), max_address_span=2)

    def test_2sf_overlays_rom_and_save_maps(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.2sflib").write_bytes(
                make_xsf(0x24, program=make_twosf_map(0, b"ROM0"), reserved=make_save_record(1, b"SAVE"))
            )
            song = root / "song.mini2sf"
            song.write_bytes(
                make_xsf(0x24, program=make_twosf_map(2, b"xx"), tags=(("_lib", "base.2sflib"),))
            )
            state = build_twosf_effective_state(resolve_xsf(song, expected_version=0x24))
            self.assertEqual(state.rom, b"ROxx")
            self.assertEqual(state.save_state, b"\x00SAVE")
            self.assertEqual(state.rom_allocated_size, 4)
            self.assertTrue(state.save_records[0].compressed_crc_matches)
            self.assertFalse(state.runtime_available)

            bad = root / "bad.2sf"
            bad.write_bytes(make_xsf(0x24, program=b"short"))
            with self.assertRaises(TwoSfError):
                build_twosf_effective_state(resolve_xsf(bad))

    def test_usf_overlays_reserved_rom_and_save_tables(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "base.usflib").write_bytes(
                make_xsf(0x21, reserved=make_usf_reserved(((0, b"ROM0"),), ((1, b"STATE"),)))
            )
            song = root / "song.miniusf"
            song.write_bytes(
                make_xsf(0x21, reserved=make_usf_reserved(((2, b"xx"),), ()), tags=(("_lib", "base.usflib"),))
            )
            state = build_usf_effective_state(resolve_xsf(song, expected_version=0x21))
            self.assertEqual(state.rom, b"ROxx")
            self.assertEqual(state.save_state, b"\x00STATE")
            self.assertEqual(state.contributions[-1].source_id, "song.miniusf")
            self.assertFalse(state.runtime_available)

            program = root / "program.usf"
            program.write_bytes(make_xsf(0x21, program=b"not-used", reserved=make_usf_reserved()))
            with self.assertRaises(UsfError):
                build_usf_effective_state(resolve_xsf(program))


if __name__ == "__main__":
    unittest.main()
