#!/usr/bin/env python3
"""Verify the final private foobar component archives before bundling them.

The build stages files in disposable directories, but the deletion gate is about
what is actually shipped. This verifier reopens each renamed ZIP
(`.fb2k-component`), checks the exact sibling payload expected by the runtime,
and inspects the packaged PE images themselves. It validates machine type,
exports, and private import boundaries so the process split cannot silently
collapse into an accidental DLL dependency. On Windows it also extracts and
loads each packaged Omniphony DLL, executes its ABI version functions, and starts
the exact packaged x86 spcplayer far enough to reach its own usage path. That
proves its sibling SNESAPU DLL can be resolved by the Windows loader. It rejects
path traversal, duplicate case-insensitive names, nested layout, zero-byte
runtime files, wrong architectures, missing runtime ABI exports, and forbidden
private imports.
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
    "omniphony_source_set_mix_budget",
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
        raise ValueError("unterminated PE string")
    try:
        return data[offset:end].decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError("non-ASCII PE string") from exc


def _pe_layout(
    data: bytes,
) -> tuple[int, int, int, list[tuple[int, int, int, int]]]:
    """Return (machine, data-directory offset, optional end, section map)."""
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
    optional_end = optional + optional_size
    if section_count == 0 or section_count > 96:
        raise ValueError("invalid PE section count")
    if optional_end > len(data):
        raise ValueError("truncated PE optional header")

    magic = _u16(data, optional)
    if magic == 0x10B:  # PE32
        data_directory = optional + 96
    elif magic == 0x20B:  # PE32+
        data_directory = optional + 112
    else:
        raise ValueError(f"unsupported PE optional-header magic 0x{magic:04X}")

    section_table = optional_end
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
    return machine, data_directory, optional_end, sections


def _rva_to_offset(
    sections: list[tuple[int, int, int, int]],
    rva: int,
    size: int = 1,
) -> int:
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


def inspect_pe(data: bytes) -> tuple[int, set[str]]:
    """Return (COFF machine, named exports) from one PE image."""
    machine, data_directory, optional_end, sections = _pe_layout(data)
    if data_directory + 8 > optional_end:
        raise ValueError("PE export data directory missing")

    export_rva = _u32(data, data_directory)
    export_size = _u32(data, data_directory + 4)
    if export_rva == 0:
        return machine, set()
    if export_size == 0:
        raise ValueError("PE export directory has zero size")

    export_offset = _rva_to_offset(sections, export_rva, 40)
    name_count = _u32(data, export_offset + 24)
    names_rva = _u32(data, export_offset + 32)
    if name_count > 65536:
        raise ValueError("unreasonable PE export-name count")
    if name_count == 0:
        return machine, set()

    names_offset = _rva_to_offset(sections, names_rva, name_count * 4)
    exports: set[str] = set()
    for index in range(name_count):
        name_rva = _u32(data, names_offset + index * 4)
        name_offset = _rva_to_offset(sections, name_rva)
        exports.add(_read_c_string(data, name_offset))
    return machine, exports


def inspect_pe_imports(data: bytes) -> set[str]:
    """Return case-preserved imported DLL names from one PE image."""
    _machine, data_directory, optional_end, sections = _pe_layout(data)
    # Data-directory entry 1 is IMAGE_DIRECTORY_ENTRY_IMPORT.
    if data_directory + 16 > optional_end:
        raise ValueError("PE import data directory missing")
    import_rva = _u32(data, data_directory + 8)
    import_size = _u32(data, data_directory + 12)
    if import_rva == 0:
        return set()
    if import_size < 20:
        raise ValueError("PE import directory is too small")

    # The size is advisory but bounds how far we will walk malformed input.
    max_descriptors = min(max(import_size // 20, 1), 4096)
    descriptor = _rva_to_offset(sections, import_rva, 20)
    imports: set[str] = set()
    for _ in range(max_descriptors):
        if descriptor + 20 > len(data):
            raise ValueError("truncated PE import descriptor")
        original_first_thunk = _u32(data, descriptor)
        time_date_stamp = _u32(data, descriptor + 4)
        forwarder_chain = _u32(data, descriptor + 8)
        name_rva = _u32(data, descriptor + 12)
        first_thunk = _u32(data, descriptor + 16)
        if (
            original_first_thunk == 0
            and time_date_stamp == 0
            and forwarder_chain == 0
            and name_rva == 0
            and first_thunk == 0
        ):
            break
        name_offset = _rva_to_offset(sections, name_rva)
        imports.add(_read_c_string(data, name_offset))
        descriptor += 20
    else:
        raise ValueError("unterminated PE import descriptor table")
    return imports


def _validate_archive_name(name: str) -> str:
    normalized = name.replace("\\", "/")
    pure = PurePosixPath(normalized)
    if pure.is_absolute() or ".." in pure.parts or len(pure.parts) != 1:
        raise AssertionError(f"unsafe or nested archive member: {name}")
    if pure.name in ("", "."):
        raise AssertionError(f"invalid archive member: {name}")
    return pure.name



def verify_archive(path: Path, expected_names: set[str], label: str) -> dict[str, bytes]:
    """Verify only the flat sibling payload envelope and return its bytes."""
    if not path.is_file():
        raise AssertionError(f"{label} component package missing: {path}")
    if path.stat().st_size <= 0:
        raise AssertionError(f"{label} component package is empty: {path}")

    try:
        archive = zipfile.ZipFile(path, "r")
    except zipfile.BadZipFile as exc:
        raise AssertionError(f"{label} component package is not a valid ZIP: {path}") from exc

    with archive:
        infos = [info for info in archive.infolist() if not info.is_dir()]
        names: list[str] = []
        folded: set[str] = set()
        payloads: dict[str, bytes] = {}
        for info in infos:
            try:
                name = _validate_archive_name(info.filename)
            except AssertionError as exc:
                raise AssertionError(
                    f"{label} package has unsafe/nested member: {info.filename}"
                ) from exc
            key = name.casefold()
            if key in folded:
                raise AssertionError(
                    f"{label} package has duplicate case-insensitive member: {name}"
                )
            folded.add(key)
            names.append(name)
            data = archive.read(info)
            if not data:
                raise AssertionError(f"{label} package has zero-byte runtime member: {name}")
            payloads[name] = data

        if set(names) != expected_names:
            raise AssertionError(
                f"{label} payload mismatch: expected {sorted(expected_names)}, got {sorted(names)}"
            )
        return payloads


def verify_runtime_contracts(
    path: Path,
    contracts: dict[str, tuple[int, frozenset[str]]],
    label: str,
) -> None:
    """Verify PE machine/export contracts independently of runtime loading."""
    payloads = verify_archive(path, set(contracts), label)
    for name, (machine, required_exports) in contracts.items():
        try:
            actual_machine, exports = inspect_pe(payloads[name])
        except (ValueError, struct.error) as exc:
            raise AssertionError(
                f"{label} package member {name} is not a valid packaged PE: {exc}"
            ) from exc
        if actual_machine != machine:
            raise AssertionError(
                f"{label} package member {name} machine mismatch: "
                f"expected 0x{machine:04X}, got 0x{actual_machine:04X}"
            )
        missing = required_exports - exports
        if missing:
            raise AssertionError(
                f"{label} package member {name} missing required exports: {sorted(missing)}"
            )


def validate_spcplayer_startup_result(returncode: int, output: str) -> None:
    """Require the no-input child to reach its own usage path, not just load."""
    if returncode != 1:
        raise AssertionError(
            f"spcplayer expected usage exit 1 after successful startup, got {returncode}"
        )
    if "usage" not in output.casefold():
        raise AssertionError("spcplayer did not reach its own usage path")


def validate_package(
    path: Path,
    expected_names: set[str],
    contracts: dict[str, tuple[int, frozenset[str]]],
) -> None:
    if not path.is_file():
        raise AssertionError(f"component package missing: {path}")
    if path.stat().st_size <= 0:
        raise AssertionError(f"component package is empty: {path}")

    with zipfile.ZipFile(path, "r") as archive:
        infos = [info for info in archive.infolist() if not info.is_dir()]
        names: list[str] = []
        folded: set[str] = set()
        payloads: dict[str, bytes] = {}
        for info in infos:
            name = _validate_archive_name(info.filename)
            key = name.casefold()
            if key in folded:
                raise AssertionError(f"duplicate case-insensitive package member: {name}")
            folded.add(key)
            names.append(name)
            data = archive.read(info)
            if not data:
                raise AssertionError(f"zero-byte runtime member: {name}")
            payloads[name] = data

        if set(names) != expected_names:
            raise AssertionError(
                f"package members differ for {path.name}: "
                f"expected {sorted(expected_names)}, got {sorted(names)}"
            )

        for name, (machine, required_exports) in contracts.items():
            actual_machine, exports = inspect_pe(payloads[name])
            if actual_machine != machine:
                raise AssertionError(
                    f"{path.name}:{name} machine mismatch: "
                    f"expected 0x{machine:04X}, got 0x{actual_machine:04X}"
                )
            missing = required_exports - exports
            if missing:
                raise AssertionError(
                    f"{path.name}:{name} missing required exports: {sorted(missing)}"
                )

        # Private runtime pieces must be discovered explicitly at runtime or by
        # the x86 child process; they must not become normal PE imports of the
        # x64 foobar components.
        for component_name in ("foo_input_vgm.dll", "foo_snesapu.dll"):
            if component_name not in payloads:
                continue
            imports = {name.casefold() for name in inspect_pe_imports(payloads[component_name])}
            forbidden = {"omniphony_source.dll", "snesapu.dll", "spcplayer.exe"} & imports
            if forbidden:
                raise AssertionError(
                    f"{path.name}:{component_name} has forbidden private imports: "
                    f"{sorted(forbidden)}"
                )


def _load_runtime_verifier(repo_root: Path):
    verifier_path = repo_root / "tools" / "verify_omniphony_runtime_abi.py"
    if not verifier_path.is_file():
        raise AssertionError(f"Omniphony runtime verifier missing: {verifier_path}")
    spec = importlib.util.spec_from_file_location("verify_omniphony_runtime_abi", verifier_path)
    if spec is None or spec.loader is None:
        raise AssertionError(f"could not load Omniphony runtime verifier: {verifier_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def validate_windows_runtime(
    vgm_path: Path,
    spc_path: Path,
    repo_root: Path,
) -> None:
    if os.name != "nt":
        return

    verifier = _load_runtime_verifier(repo_root)
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        vgm_dir = root / "vgm"
        spc_dir = root / "spc"
        vgm_dir.mkdir()
        spc_dir.mkdir()

        with zipfile.ZipFile(vgm_path, "r") as archive:
            archive.extractall(vgm_dir)
        with zipfile.ZipFile(spc_path, "r") as archive:
            archive.extractall(spc_dir)

        vgm_version = verifier.load_and_verify(vgm_dir / "omniphony_source.dll")
        spc_version = verifier.load_and_verify(spc_dir / "omniphony_source.dll")
        if vgm_version != spc_version:
            raise AssertionError(
                "packaged Omniphony runtime versions differ: "
                f"VGM={vgm_version}, SPC={spc_version}"
            )
        if (vgm_dir / "omniphony_source.dll").read_bytes() != (
            spc_dir / "omniphony_source.dll"
        ).read_bytes():
            raise AssertionError("VGM/SPC packages do not carry the same Omniphony DLL bytes")

        # Start the exact packaged child binary so Windows resolves its imports
        # using the package directory. The no-argument path is expected to fail
        # after startup with a usage error; loader failure must be distinguishable.
        child = subprocess.run(
            [str(spc_dir / "spcplayer.exe")],
            cwd=spc_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=15,
            check=False,
        )
        output = child.stdout + "\n" + child.stderr
        validate_spcplayer_startup_result(child.returncode, output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("vgm_package", type=Path)
    parser.add_argument("spc_package", type=Path)
    args = parser.parse_args()

    vgm_path = args.vgm_package.resolve()
    spc_path = args.spc_package.resolve()
    validate_package(vgm_path, VGM_EXPECTED, VGM_RUNTIME_CONTRACTS)
    validate_package(spc_path, SPC_EXPECTED, SPC_RUNTIME_CONTRACTS)
    validate_windows_runtime(vgm_path, spc_path, Path(__file__).resolve().parents[1])
    print("private foobar component packages verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
