"""Exact xSF envelope parsing and dependency ordering.

This module owns only the grammar shared by xSF-family containers.  It does
not assign common execution semantics to the decompressed program or reserved
sections.
"""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
from pathlib import Path, PurePosixPath, PureWindowsPath
import struct
from typing import Callable
import zlib


class XsfError(ValueError):
    """An xSF object violates the common envelope contract."""


class XsfDependencyError(XsfError):
    """An xSF dependency graph cannot be resolved safely."""


@dataclass(frozen=True)
class XsfTag:
    name: str
    value: str
    line_index: int


@dataclass(frozen=True)
class XsfObject:
    source_id: str
    sha256: str
    version: int
    reserved: bytes
    compressed_program: bytes
    program_crc32: int
    program: bytes
    tags: tuple[XsfTag, ...]

    def tag_values(self, name: str) -> tuple[str, ...]:
        folded = name.casefold()
        return tuple(tag.value for tag in self.tags if tag.name.casefold() == folded)

    def library_directives(self) -> tuple[tuple[str, str], ...]:
        by_name: dict[str, list[str]] = {}
        for tag in self.tags:
            key = tag.name.casefold()
            if key == "_lib" or (key.startswith("_lib") and key[4:].isdigit()):
                by_name.setdefault(key, []).append(tag.value)
        duplicates = sorted(key for key, values in by_name.items() if len(values) != 1)
        if duplicates:
            raise XsfDependencyError(
                f"duplicate dependency tag(s) in {self.source_id}: {', '.join(duplicates)}"
            )

        directives: list[tuple[str, str]] = []
        if "_lib" in by_name:
            directives.append(("_lib", by_name["_lib"][0]))
        numbered = sorted(
            int(key[4:]) for key in by_name if key != "_lib" and int(key[4:]) >= 2
        )
        if numbered:
            expected = list(range(2, numbered[-1] + 1))
            if numbered != expected:
                raise XsfDependencyError(
                    f"non-contiguous numbered libraries in {self.source_id}: {numbered}"
                )
            directives.extend((f"_lib{number}", by_name[f"_lib{number}"][0]) for number in numbered)
        return tuple(directives)


@dataclass(frozen=True)
class DependencyEdge:
    parent: str
    tag: str
    child: str


@dataclass(frozen=True)
class ResolvedXsf:
    root: str
    version: int
    objects: tuple[XsfObject, ...]
    edges: tuple[DependencyEdge, ...]


def _decode_tags(payload: bytes, source_id: str) -> tuple[XsfTag, ...]:
    if not payload:
        return ()
    if b"\x00" in payload:
        raise XsfError(f"NUL byte in tag section: {source_id}")
    text = payload.decode("latin-1")
    tags: list[XsfTag] = []
    for line_index, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.strip("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0b\x0c\x0d\x0e\x0f"
                              "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ")
        if not line:
            continue
        if "=" not in line:
            raise XsfError(f"invalid tag line {line_index} in {source_id}")
        name, value = line.split("=", 1)
        name = name.strip()
        value = value.strip()
        if (
            not name
            or "=" in name
            or any(ord(character) < 0x20 or ord(character) == 0x7F for character in name)
        ):
            raise XsfError(f"invalid tag name {name!r} in {source_id}")
        tags.append(XsfTag(name=name, value=value, line_index=line_index))
    return tuple(tags)


def _decompress_strict(payload: bytes, source_id: str, max_program_size: int) -> bytes:
    if not payload:
        return b""
    inflater = zlib.decompressobj()
    try:
        program = inflater.decompress(payload, max_program_size + 1)
    except zlib.error as exc:
        raise XsfError(f"invalid zlib program in {source_id}: {exc}") from exc
    if len(program) > max_program_size or inflater.unconsumed_tail:
        raise XsfError(f"decompressed program exceeds limit in {source_id}")
    try:
        program += inflater.flush()
    except zlib.error as exc:
        raise XsfError(f"invalid zlib termination in {source_id}: {exc}") from exc
    if not inflater.eof or inflater.unused_data:
        raise XsfError(f"program is not exactly one complete zlib stream: {source_id}")
    if len(program) > max_program_size:
        raise XsfError(f"decompressed program exceeds limit in {source_id}")
    return program


def parse_xsf(
    data: bytes,
    *,
    source_id: str = "<memory>",
    expected_version: int | None = None,
    max_program_size: int = 512 * 1024 * 1024,
) -> XsfObject:
    if len(data) < 16:
        raise XsfError(f"truncated xSF header: {source_id}")
    if data[:3] != b"PSF":
        raise XsfError(f"invalid xSF signature: {source_id}")
    version = data[3]
    if expected_version is not None and version != expected_version:
        raise XsfError(
            f"xSF version mismatch in {source_id}: expected 0x{expected_version:02X}, got 0x{version:02X}"
        )
    reserved_size, compressed_size, declared_crc = struct.unpack_from("<III", data, 4)
    section_end = 16 + reserved_size + compressed_size
    if section_end > len(data):
        raise XsfError(f"declared sections exceed file size: {source_id}")
    reserved = data[16 : 16 + reserved_size]
    compressed = data[16 + reserved_size : section_end]
    actual_crc = zlib.crc32(compressed) & 0xFFFFFFFF
    if actual_crc != declared_crc:
        raise XsfError(
            f"compressed-program CRC mismatch in {source_id}: "
            f"declared 0x{declared_crc:08X}, actual 0x{actual_crc:08X}"
        )
    remainder = data[section_end:]
    if remainder:
        if not remainder.startswith(b"[TAG]"):
            raise XsfError(f"unexpected trailing bytes after program: {source_id}")
        tag_payload = remainder[5:]
    else:
        tag_payload = b""
    return XsfObject(
        source_id=source_id,
        sha256=hashlib.sha256(data).hexdigest(),
        version=version,
        reserved=reserved,
        compressed_program=compressed,
        program_crc32=declared_crc,
        program=_decompress_strict(compressed, source_id, max_program_size),
        tags=_decode_tags(tag_payload, source_id),
    )


def resolve_xsf(
    root: Path,
    *,
    expected_version: int | None = None,
    read_bytes: Callable[[Path], bytes] | None = None,
    max_depth: int = 32,
) -> ResolvedXsf:
    root = root.resolve()
    library_root = root.parent
    reader = read_bytes or Path.read_bytes
    objects: list[XsfObject] = []
    edges: list[DependencyEdge] = []
    active: list[Path] = []
    discovered_version: int | None = expected_version

    def relative_source_id(path: Path) -> str:
        try:
            return path.relative_to(library_root).as_posix()
        except ValueError as exc:
            raise XsfDependencyError(f"dependency escapes root directory: {path}") from exc

    def dependency_path(parent: Path, value: str) -> Path:
        # The PSF/xSF library convention treats both slash styles as path
        # separators and resolves every _lib* value relative to the file that
        # contains the tag, not relative to the top-level root file.
        normalized = value.replace("\\", "/")
        posix_path = PurePosixPath(normalized)
        windows_path = PureWindowsPath(value)
        if posix_path.is_absolute() or windows_path.is_absolute() or windows_path.drive:
            raise XsfDependencyError(
                f"absolute xSF dependency is not allowed in {relative_source_id(parent)}: {value}"
            )
        candidate = (parent.parent / Path(*posix_path.parts)).resolve()
        # Keep dependency traversal inside the tree explicitly selected by the
        # caller. This permits standard subdirectories and nested relative
        # references while still rejecting accidental/hostile filesystem escape.
        relative_source_id(candidate)
        return candidate

    def visit(path: Path, depth: int) -> None:
        nonlocal discovered_version
        resolved = path.resolve()
        if depth > max_depth:
            raise XsfDependencyError(f"dependency depth exceeds {max_depth}: {resolved}")
        source_id = relative_source_id(resolved)
        if resolved in active:
            cycle = " -> ".join(relative_source_id(item) for item in (*active, resolved))
            raise XsfDependencyError(f"cyclic xSF dependency: {cycle}")
        if not resolved.is_file():
            raise XsfDependencyError(f"missing xSF dependency: {source_id}")
        obj = parse_xsf(
            reader(resolved),
            source_id=source_id,
            expected_version=discovered_version,
        )
        if discovered_version is None:
            discovered_version = obj.version
        directives = obj.library_directives()
        active.append(resolved)
        try:
            primary = next((value for tag, value in directives if tag == "_lib"), None)
            if primary is not None:
                child = dependency_path(resolved, primary)
                edges.append(DependencyEdge(obj.source_id, "_lib", relative_source_id(child)))
                visit(child, depth + 1)
            objects.append(obj)
            for tag, value in directives:
                if tag == "_lib":
                    continue
                child = dependency_path(resolved, value)
                edges.append(DependencyEdge(obj.source_id, tag, relative_source_id(child)))
                visit(child, depth + 1)
        finally:
            active.pop()

    visit(root, 0)
    assert discovered_version is not None
    return ResolvedXsf(
        root=root.name,
        version=discovered_version,
        objects=tuple(objects),
        edges=tuple(edges),
    )
