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


def make_pe(machine: int, imports: tuple[str, ...] = ()) -> bytes:
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
    struct.pack_into("<H", data, coff + 2, 1)
    struct.pack_into("<H", data, coff + 16, optional_size)
    struct.pack_into("<H", data, optional, optional_magic)

    data[section_table:section_table + 8] = b".rdata\0\0"
    struct.pack_into("<I", data, section_table + 8, raw_size)
    struct.pack_into("<I", data, section_table + 12, 0x1000)
    struct.pack_into("<I", data, section_table + 16, raw_size)
    struct.pack_into("<I", data, section_table + 20, raw_offset)

    if imports:
        data_directory = optional + (112 if is_x64 else 96)
        descriptor_offset = raw_offset + 0x500
        descriptor_rva = 0x1500
        descriptor_size = (len(imports) + 1) * 20
        struct.pack_into("<I", data, data_directory + 8, descriptor_rva)
        struct.pack_into("<I", data, data_directory + 12, descriptor_size)

        cursor = raw_offset + 0x700
        for index, name in enumerate(imports):
            encoded = name.encode("ascii") + b"\0"
            name_rva = 0x1000 + (cursor - raw_offset)
            struct.pack_into("<I", data, descriptor_offset + index * 20 + 12, name_rva)
            data[cursor:cursor + len(encoded)] = encoded
            cursor += len(encoded)
        # The final descriptor stays zero-filled as the terminator.

    return bytes(data)


def write_packages(
    root: Path,
    *,
    vgm_imports: tuple[str, ...] = ("KERNEL32.dll",),
    parent_imports: tuple[str, ...] = ("KERNEL32.dll",),
    player_imports: tuple[str, ...] = ("KERNEL32.dll", "SNESAPU.dll"),
    snesapu_imports: tuple[str, ...] = ("KERNEL32.dll",),
) -> tuple[Path, Path]:
    vgm = root / "vgm.fb2k-component"
    spc = root / "spc.fb2k-component"
    with zipfile.ZipFile(vgm, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(
            "foo_input_vgm.dll",
            make_pe(_verifier.IMAGE_FILE_MACHINE_AMD64, vgm_imports),
        )
    with zipfile.ZipFile(spc, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(
            "foo_snesapu.dll",
            make_pe(_verifier.IMAGE_FILE_MACHINE_AMD64, parent_imports),
        )
        archive.writestr(
            "spcplayer.exe",
            make_pe(_verifier.IMAGE_FILE_MACHINE_I386, player_imports),
        )
        archive.writestr(
            "SNESAPU.dll",
            make_pe(_verifier.IMAGE_FILE_MACHINE_I386, snesapu_imports),
        )
    return vgm, spc


class PrivateComponentImportContractTest(unittest.TestCase):
    def test_import_parser_reads_case_preserved_dll_names(self) -> None:
        data = make_pe(
            _verifier.IMAGE_FILE_MACHINE_I386,
            ("KERNEL32.dll", "SNESAPU.dll"),
        )
        self.assertEqual(
            _verifier.inspect_pe_imports(data),
            {"KERNEL32.dll", "SNESAPU.dll"},
        )

    def test_accepts_intended_private_process_boundaries(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vgm, spc = write_packages(Path(tmp))
            _verifier.verify_private_import_contracts(vgm, spc)

    def test_rejects_spcplayer_without_snesapu_import(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vgm, spc = write_packages(
                Path(tmp),
                player_imports=("KERNEL32.dll",),
            )
            with self.assertRaisesRegex(AssertionError, "must import sibling SNESAPU"):
                _verifier.verify_private_import_contracts(vgm, spc)

    def test_rejects_x64_parent_importing_x86_snesapu(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vgm, spc = write_packages(
                Path(tmp),
                parent_imports=("KERNEL32.dll", "SNESAPU.dll"),
            )
            with self.assertRaisesRegex(AssertionError, "x64 foo_snesapu"):
                _verifier.verify_private_import_contracts(vgm, spc)

    def test_rejects_direct_omniphony_import(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vgm, spc = write_packages(
                Path(tmp),
                vgm_imports=("KERNEL32.dll", "omniphony_source.dll"),
            )
            with self.assertRaisesRegex(AssertionError, "sibling-dynamic-loaded"):
                _verifier.verify_private_import_contracts(vgm, spc)

    def test_rejects_shared_libvgm_runtime_import(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            vgm, spc = write_packages(
                Path(tmp),
                vgm_imports=("KERNEL32.dll", "libvgm-player.dll"),
            )
            with self.assertRaisesRegex(AssertionError, "static libvgm linkage"):
                _verifier.verify_private_import_contracts(vgm, spc)


if __name__ == "__main__":
    unittest.main()
