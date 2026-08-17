from __future__ import annotations

import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERIFIER_PATH = ROOT / "tools" / "verify_private_component_packages.py"
_spec = importlib.util.spec_from_file_location("private_package_verifier", VERIFIER_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError("could not load private component package verifier")
_verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_verifier)


def make_pe(machine: int, exports: set[str] | frozenset[str] = frozenset()) -> bytes:
    """Build a tiny structurally valid PE fixture with one export-bearing section."""
    is_x64 = machine == _verifier.IMAGE_FILE_MACHINE_AMD64
    optional_size = 0xF0 if is_x64 else 0xE0
    optional_magic = 0x20B if is_x64 else 0x10B
    pe_offset = 0x80
    coff = pe_offset + 4
    optional = coff + 20
    section_table = optional + optional_size
    raw_offset = 0x200
    raw_size = 0x1000
    data = bytearray(raw_offset + raw_size)

    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, pe_offset)
    data[pe_offset:pe_offset + 4] = b"PE\0\0"
    struct.pack_into("<H", data, coff, machine)
    struct.pack_into("<H", data, coff + 2, 1)  # NumberOfSections
    struct.pack_into("<H", data, coff + 16, optional_size)
    struct.pack_into("<H", data, optional, optional_magic)

    data[section_table:section_table + 8] = b".rdata\0\0"
    struct.pack_into("<I", data, section_table + 8, raw_size)  # VirtualSize
    struct.pack_into("<I", data, section_table + 12, 0x1000)  # VirtualAddress
    struct.pack_into("<I", data, section_table + 16, raw_size)
    struct.pack_into("<I", data, section_table + 20, raw_offset)

    if exports:
        data_directory = optional + (112 if is_x64 else 96)
        struct.pack_into("<I", data, data_directory, 0x1000)  # Export RVA
        struct.pack_into("<I", data, data_directory + 4, 0x400)

        export_offset = raw_offset
        names = sorted(exports)
        names_rva = 0x1040
        names_offset = raw_offset + 0x40
        strings_offset = raw_offset + 0x100
        struct.pack_into("<I", data, export_offset + 24, len(names))
        struct.pack_into("<I", data, export_offset + 32, names_rva)

        cursor = strings_offset
        for index, name in enumerate(names):
            encoded = name.encode("ascii") + b"\0"
            name_rva = 0x1000 + (cursor - raw_offset)
            struct.pack_into("<I", data, names_offset + index * 4, name_rva)
            data[cursor:cursor + len(encoded)] = encoded
            cursor += len(encoded)

    return bytes(data)


class PrivateComponentPackageContractTest(unittest.TestCase):
    def write_zip(self, path: Path, entries: dict[str, bytes]) -> None:
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, payload in entries.items():
                archive.writestr(name, payload)

    def test_accepts_exact_vgm_and_spc_payloads(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            vgm = root / "foo_input_vgm.private.fb2k-component"
            spc = root / "foo_snesapu.private.fb2k-component"
            self.write_zip(vgm, {name: b"x" for name in _verifier.VGM_EXPECTED})
            self.write_zip(spc, {name: b"x" for name in _verifier.SPC_EXPECTED})
            _verifier.verify_archive(vgm, _verifier.VGM_EXPECTED, "VGM")
            _verifier.verify_archive(spc, _verifier.SPC_EXPECTED, "SPC")

    def test_accepts_expected_packaged_pe_contracts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            vgm = root / "vgm.fb2k-component"
            spc = root / "spc.fb2k-component"
            self.write_zip(
                vgm,
                {
                    "foo_input_vgm.dll": make_pe(_verifier.IMAGE_FILE_MACHINE_AMD64),
                    "omniphony_source.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_AMD64,
                        _verifier.OMNIPHONY_REQUIRED_EXPORTS,
                    ),
                },
            )
            self.write_zip(
                spc,
                {
                    "foo_snesapu.dll": make_pe(_verifier.IMAGE_FILE_MACHINE_AMD64),
                    "spcplayer.exe": make_pe(_verifier.IMAGE_FILE_MACHINE_I386),
                    "SNESAPU.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_I386,
                        _verifier.SNESAPU_REQUIRED_EXPORTS,
                    ),
                    "omniphony_source.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_AMD64,
                        _verifier.OMNIPHONY_REQUIRED_EXPORTS,
                    ),
                },
            )
            _verifier.verify_runtime_contracts(vgm, _verifier.VGM_RUNTIME_CONTRACTS, "VGM")
            _verifier.verify_runtime_contracts(spc, _verifier.SPC_RUNTIME_CONTRACTS, "SPC")

    def test_rejects_missing_runtime_export(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            exports = set(_verifier.OMNIPHONY_REQUIRED_EXPORTS)
            exports.remove("omniphony_source_create")
            self.write_zip(
                package,
                {
                    "foo_input_vgm.dll": make_pe(_verifier.IMAGE_FILE_MACHINE_AMD64),
                    "omniphony_source.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_AMD64,
                        exports,
                    ),
                },
            )
            with self.assertRaisesRegex(AssertionError, "missing required exports"):
                _verifier.verify_runtime_contracts(
                    package, _verifier.VGM_RUNTIME_CONTRACTS, "VGM"
                )

    def test_rejects_wrong_packaged_machine(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            self.write_zip(
                package,
                {
                    "foo_input_vgm.dll": make_pe(_verifier.IMAGE_FILE_MACHINE_I386),
                    "omniphony_source.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_AMD64,
                        _verifier.OMNIPHONY_REQUIRED_EXPORTS,
                    ),
                },
            )
            with self.assertRaisesRegex(AssertionError, "machine mismatch"):
                _verifier.verify_runtime_contracts(
                    package, _verifier.VGM_RUNTIME_CONTRACTS, "VGM"
                )

    def test_rejects_malformed_packaged_pe(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            self.write_zip(
                package,
                {
                    "foo_input_vgm.dll": b"not-a-pe",
                    "omniphony_source.dll": make_pe(
                        _verifier.IMAGE_FILE_MACHINE_AMD64,
                        _verifier.OMNIPHONY_REQUIRED_EXPORTS,
                    ),
                },
            )
            with self.assertRaisesRegex(AssertionError, "valid packaged PE"):
                _verifier.verify_runtime_contracts(
                    package, _verifier.VGM_RUNTIME_CONTRACTS, "VGM"
                )

    def test_rejects_missing_or_extra_payload(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            self.write_zip(
                package,
                {
                    "foo_input_vgm.dll": b"x",
                    "unexpected.dll": b"x",
                },
            )
            with self.assertRaisesRegex(AssertionError, "payload mismatch"):
                _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_nested_or_traversal_entries(self) -> None:
        for bad_name in ("nested/foo_input_vgm.dll", "../foo_input_vgm.dll"):
            with self.subTest(bad_name=bad_name), tempfile.TemporaryDirectory() as tmp:
                package = Path(tmp) / "bad.fb2k-component"
                self.write_zip(
                    package,
                    {
                        bad_name: b"x",
                        "omniphony_source.dll": b"x",
                    },
                )
                with self.assertRaisesRegex(AssertionError, "unsafe/nested"):
                    _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_zero_byte_runtime(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            entries = {name: b"x" for name in _verifier.VGM_EXPECTED}
            entries["omniphony_source.dll"] = b""
            self.write_zip(package, entries)
            with self.assertRaisesRegex(AssertionError, "zero-byte"):
                _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_case_colliding_entries(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            with zipfile.ZipFile(package, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                archive.writestr("foo_snesapu.dll", b"x")
                archive.writestr("FOO_SNESAPU.DLL", b"x")
                archive.writestr("spcplayer.exe", b"x")
                archive.writestr("SNESAPU.dll", b"x")
                archive.writestr("omniphony_source.dll", b"x")
            with self.assertRaisesRegex(AssertionError, "duplicate"):
                _verifier.verify_archive(package, _verifier.SPC_EXPECTED, "SPC")


if __name__ == "__main__":
    unittest.main()
