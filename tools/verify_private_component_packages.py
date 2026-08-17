#!/usr/bin/env python3
"""Verify the final private foobar component archives before bundling them.

The build stages files in disposable directories, but the deletion gate is about
what is actually shipped. This verifier reopens each renamed ZIP
(`.fb2k-component`), checks the exact sibling payload expected by the runtime,
and inspects the packaged PE images themselves. On Windows it also extracts and
loads each packaged Omniphony DLL, executes its ABI version functions, and starts
the exact packaged x86 spcplayer far enough to reach its own usage path. That
proves its sibling SNESAPU DLL can be resolved by the Windows loader. It rejects
path traversal, duplicate case-insensitive names, nested layout, zero-byte
runtime files, wrong architectures, and missing runtime ABI exports.
"""

from __future__ import annotations

import argparse
import importlib.util
import os
from pathlib import Path, PurePosixPath
import struct
import subprocess
import tempfile
import zipfile


IMAGE_FILE_MACHINE_I386 = 0x014C
IMAGE_FILE_MACHINE_AMD64 = 0x8664

VGM_EXPECTED = {
    "foo_input_vgm.dll",
    "omniphony_source.dll",
}

SPC_EXPECTED = {
    "foo_snesapu.dll",
    "spcplayer.exe",
    "SNESAPU.dll",
    "omniphony_source.dll",
}

OMNIPHONY_REQUIRED_EXPORTS = {
    "omniphony_source_abi_major",
    "omniphony_source_abi_minor",
    "omniphony_source_create",
    "omniphony_source_destroy",
    "omniphony_source_reset",
    "omniphony_source_process_events_f32",
}

SNESAPU_REQUIRED_EXPORTS = {
    "EmuAPU",
    "GetAPUData",
    "LoadSPCFile",
    "SetAPUOpt",
    "SetDSPAmp",
    "SetDSPSourceCapture",
    "GetDSPSourceData",
    "SetDSPPreBrrProvider",
    "SetDSPStudioSourceProvider",
}

VGM_RUNTIME_CONTRACTS = {
    "foo_input_vgm.dll": (IMAGE_FILE_MACHINE_AMD64, frozenset()),
    "omniphony_source.dll": (
        IMAGE_FILE_MACHINE_AMD64,
        frozenset(OMNIPHONY_REQUIRED_EXPORTS),
    ),
}

SPC_RUNTIME_CONTRACTS = {
    "foo_snesapu.dll": (IMAGE_FILE_MACHINE_AMD64, frozenset()),
    "spcplayer.exe": (IMAGE_FILE_MACHINE_I386, frozenset()),
    "SNESAPU.dll": (IMAGE_FILE_MACHINE_I386, frozenset(SNESAPU_REQUIRED_EXPORTS)),
    "omniphony_source.dll": (
        IMAGE_FILE_MACHINE_AMD64,
        frozenset(OMNIPHONY_REQUIRED_EXPORTS),
    ),
}


def _u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise ValueError("truncated PE u16")
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ValueError("truncated PE u32")
    return struct.unpack_from("<I", data, offset)[0]


def _read_c_string(data: bytes, offset: int, *, limit: int = 4096) -> str:
    if offset < 0 or offset >= len(data):
        raise ValueError("PE string offset outside file")
    end_limit = min(len(data), offset + limit)
    end = data.find(b"\0", offset, end_limit)
    if end < 0:
        raise ValueError("unterminated PE export name")
    try:
        return data[offset:end].decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError("non-ASCII PE export name") from exc


def inspect_pe(data: bytes) -> tuple[int, set[str]]:
    """Return (COFF machine, named exports) from one PE image."""
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError("not an MZ executable")

    pe_offset = _u32(data, 0x3C)
    if pe_offset + 24 > len(data) or data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")

    coff = pe_offset + 4
    machine = _u16(data, coff)
    section_count = _u16(data, coff + 2)
    optional_size = _u16(data, coff + 16)
    optional = coff + 20
    if section_count == 0 or section_count > 96:
        raise ValueError("invalid PE section count")
    if optional + optional_size > len(data):
        raise ValueError("truncated PE optional header")

    magic = _u16(data, optional)
    if magic == 0x10B:  # PE32
        data_directory = optional + 96
    elif magic == 0x20B:  # PE32+
        data_directory = optional + 112
    else:
        raise ValueError(f"unsupported PE optional-header magic 0x{magic:04X}")
    if data_directory + 8 > optional + optional_size:
        raise ValueError("PE export data directory missing")

    export_rva = _u32(data, data_directory)
    export_size = _u32(data, data_directory + 4)
    section_table = optional + optional_size
    if section_table + section_count * 40 > len(data):
        raise ValueError("truncated PE section table")

    sections: list[tuple[int, int, int, int]] = []
    for index in range(section_count):
        section = section_table + index * 40
        virtual_size = _u32(data, section + 8)
        virtual_address = _u32(data, section + 12)
        raw_size = _u32(data, section + 16)
        raw_offset = _u32(data, section + 20)
        if raw_offset > len(data) or raw_size > len(data) - raw_offset:
            raise ValueError("PE section raw data outside file")
        sections.append((virtual_address, virtual_size, raw_offset, raw_size))

    def rva_to_offset(rva: int, size: int = 1) -> int:
        if size < 0:
            raise ValueError("negative PE RVA size")
        for virtual_address, virtual_size, raw_offset, raw_size in sections:
            span = max(virtual_size, raw_size)
            if virtual_address <= rva < virtual_address + span:
                delta = rva - virtual_address
                if delta > raw_size or size > raw_size - delta:
                    raise ValueError("PE RVA points outside section raw data")
                return raw_offset + delta
        raise ValueError(f"PE RVA 0x{rva:X} is not backed by a section")

    if export_rva == 0:
        return machine, set()
    if export_size == 0:
        raise ValueError("PE export directory has zero size")

    export_offset = rva_to_offset(export_rva, 40)
    name_count = _u32(data, export_offset + 24)
    names_rva = _u32(data, export_offset + 32)
    if name_count > 65536:
        raise ValueError("unreasonable PE export-name count")
    if name_count == 0:
        return machine, set()

    names_offset = rva_to_offset(names_rva, name_count * 4)
    exports: set[str] = set()
    for index in range(name_count):
        name_rva = _u32(data, names_offset + index * 4)
        name_offset = rva_to_offset(name_rva)
        exports.add(_read_c_string(data, name_offset))
    return machine, exports


def verify_archive(path: Path, expected: set[str], label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"{label} package missing: {path}")
    if not zipfile.is_zipfile(path):
        raise RuntimeError(f"{label} package is not a ZIP/fb2k-component archive: {path}")

    with zipfile.ZipFile(path, "r") as archive:
        infos = [info for info in archive.infolist() if not info.is_dir()]
        names = [PurePosixPath(info.filename).as_posix() for info in infos]

        unsafe = [
            name
            for name in names
            if not name
            or name.startswith("/")
            or ".." in PurePosixPath(name).parts
            or len(PurePosixPath(name).parts) != 1
        ]
        if unsafe:
            raise AssertionError(f"{label} package has unsafe/nested entries: {unsafe}")

        folded: dict[str, str] = {}
        duplicates: list[tuple[str, str]] = []
        for name in names:
            key = name.casefold()
            previous = folded.get(key)
            if previous is not None:
                duplicates.append((previous, name))
            else:
                folded[key] = name
        if duplicates:
            raise AssertionError(
                f"{label} package has case-insensitive duplicate entries: {duplicates}"
            )

        actual = set(names)
        if actual != expected:
            missing = sorted(expected - actual)
            extra = sorted(actual - expected)
            raise AssertionError(
                f"{label} package payload mismatch; missing={missing}, extra={extra}"
            )

        empty = sorted(info.filename for info in infos if info.file_size == 0)
        if empty:
            raise AssertionError(f"{label} package contains zero-byte runtime files: {empty}")


def verify_runtime_contracts(
    path: Path,
    contracts: dict[str, tuple[int, frozenset[str]]],
    label: str,
) -> None:
    with zipfile.ZipFile(path, "r") as archive:
        for name, (expected_machine, required_exports) in contracts.items():
            try:
                data = archive.read(name)
            except KeyError as exc:
                raise AssertionError(f"{label} runtime missing from archive: {name}") from exc
            try:
                machine, exports = inspect_pe(data)
            except ValueError as exc:
                raise AssertionError(f"{label} {name} is not a valid packaged PE: {exc}") from exc
            if machine != expected_machine:
                raise AssertionError(
                    f"{label} {name} machine mismatch: expected 0x{expected_machine:04X}, "
                    f"got 0x{machine:04X}"
                )
            missing_exports = sorted(required_exports - exports)
            if missing_exports:
                raise AssertionError(
                    f"{label} {name} missing required exports: {missing_exports}"
                )


def _load_omniphony_abi_verifier():
    path = Path(__file__).with_name("verify_omniphony_runtime_abi.py")
    spec = importlib.util.spec_from_file_location("omniphony_runtime_abi", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load Omniphony runtime ABI verifier: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def verify_packaged_omniphony_runtime(path: Path, label: str) -> tuple[int, int] | None:
    """On Windows, load the exact archived DLL and execute its ABI functions."""
    if os.name != "nt":
        return None
    verifier = _load_omniphony_abi_verifier()
    with tempfile.TemporaryDirectory(prefix="omniphony-package-abi-") as temporary:
        dll = Path(temporary) / "omniphony_source.dll"
        with zipfile.ZipFile(path, "r") as archive:
            dll.write_bytes(archive.read("omniphony_source.dll"))
        try:
            major, minor = verifier.load_and_verify(dll)
        except (AssertionError, RuntimeError) as exc:
            raise AssertionError(
                f"{label} packaged Omniphony runtime ABI validation failed: {exc}"
            ) from exc
        return major, minor


def validate_spcplayer_startup_result(returncode: int, output: str) -> None:
    normalized = output.lower()
    if returncode != 1:
        raise AssertionError(
            f"packaged spcplayer startup returned {returncode}, expected usage exit 1"
        )
    if "spcplayer" not in normalized or "usage:" not in normalized:
        raise AssertionError(
            "packaged spcplayer did not reach its own usage path; "
            f"captured output was {output!r}"
        )


def verify_packaged_spcplayer_startup(path: Path) -> bool | None:
    """On Windows, prove the archived x86 child resolves its sibling SNESAPU DLL."""
    if os.name != "nt":
        return None
    with tempfile.TemporaryDirectory(prefix="spcplayer-package-smoke-") as temporary:
        root = Path(temporary)
        with zipfile.ZipFile(path, "r") as archive:
            for name in SPC_EXPECTED:
                (root / name).write_bytes(archive.read(name))
        player = root / "spcplayer.exe"
        try:
            completed = subprocess.run(
                [str(player)],
                cwd=str(root),
                capture_output=True,
                text=True,
                check=False,
                timeout=10,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise AssertionError(f"packaged spcplayer could not start: {exc}") from exc
        validate_spcplayer_startup_result(
            completed.returncode,
            (completed.stdout or "") + (completed.stderr or ""),
        )
        return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("vgm_component", type=Path)
    parser.add_argument("spc_component", type=Path)
    args = parser.parse_args()

    vgm = args.vgm_component.resolve()
    spc = args.spc_component.resolve()
    verify_archive(vgm, VGM_EXPECTED, "VGM")
    verify_archive(spc, SPC_EXPECTED, "SPC")
    verify_runtime_contracts(vgm, VGM_RUNTIME_CONTRACTS, "VGM")
    verify_runtime_contracts(spc, SPC_RUNTIME_CONTRACTS, "SPC")
    vgm_abi = verify_packaged_omniphony_runtime(vgm, "VGM")
    spc_abi = verify_packaged_omniphony_runtime(spc, "SPC")
    spcplayer_started = verify_packaged_spcplayer_startup(spc)
    if vgm_abi is not None or spc_abi is not None:
        print(f"packaged Omniphony runtime ABI verified: VGM={vgm_abi}, SPC={spc_abi}")
    if spcplayer_started:
        print("packaged spcplayer startup and sibling SNESAPU resolution verified")
    print("private foobar component package payloads and runtime PE contracts verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
