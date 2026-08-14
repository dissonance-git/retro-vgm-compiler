"""2SF-specific map semantics observed in maintained vio2sf descendants."""

from __future__ import annotations

from dataclasses import dataclass
import struct
import zlib

from components.xsf.envelope import ResolvedXsf, XsfObject
from components.xsf.provenance import ByteContribution, OverlayBuffer


TWOSF_VERSION = 0x24


class TwoSfError(ValueError):
    pass


@dataclass(frozen=True)
class TwoSfMap:
    offset: int
    payload: bytes
    trailing_size: int


@dataclass(frozen=True)
class SaveRecord:
    compressed_size: int
    declared_crc32: int
    compressed_crc_matches: bool
    decompressed_crc_matches: bool
    mapping: TwoSfMap
    reserved_offset: int


@dataclass(frozen=True)
class TwoSfEffectiveState:
    root: str
    rom: bytes
    save_state: bytes
    rom_allocated_size: int
    save_records: tuple[SaveRecord, ...]
    contributions: tuple[ByteContribution, ...]
    runtime_available: bool = False


def parse_twosf_map(data: bytes, source_id: str) -> TwoSfMap:
    if len(data) < 8:
        raise TwoSfError(f"truncated 2SF map: {source_id}")
    offset, size = struct.unpack_from("<II", data)
    if 8 + size > len(data):
        raise TwoSfError(f"2SF map payload exceeds section: {source_id}")
    return TwoSfMap(offset=offset, payload=data[8 : 8 + size], trailing_size=len(data) - 8 - size)


def parse_save_records(obj: XsfObject) -> tuple[SaveRecord, ...]:
    reserved = obj.reserved
    records: list[SaveRecord] = []
    position = 0
    while position < len(reserved):
        if len(reserved) - position < 12:
            raise TwoSfError(f"truncated 2SF reserved record: {obj.source_id}")
        signature = reserved[position : position + 4]
        compressed_size, declared_crc = struct.unpack_from("<II", reserved, position + 4)
        end = position + 12 + compressed_size
        if end > len(reserved):
            raise TwoSfError(f"2SF reserved record exceeds section: {obj.source_id}")
        compressed = reserved[position + 12 : end]
        if signature != b"SAVE":
            raise TwoSfError(f"unknown 2SF reserved record {signature!r}: {obj.source_id}")
        try:
            decompressed = zlib.decompress(compressed)
        except zlib.error as exc:
            raise TwoSfError(f"invalid 2SF SAVE zlib stream: {obj.source_id}: {exc}") from exc
        records.append(
            SaveRecord(
                compressed_size=compressed_size,
                declared_crc32=declared_crc,
                compressed_crc_matches=(zlib.crc32(compressed) & 0xFFFFFFFF) == declared_crc,
                decompressed_crc_matches=(zlib.crc32(decompressed) & 0xFFFFFFFF) == declared_crc,
                mapping=parse_twosf_map(decompressed, obj.source_id),
                reserved_offset=position,
            )
        )
        position = end
    return tuple(records)


def _next_power_of_two(value: int) -> int:
    if value <= 0:
        return 0
    return 1 << (value - 1).bit_length()


def build_twosf_effective_state(
    resolved: ResolvedXsf,
    *,
    max_rom_size: int = 512 * 1024 * 1024,
    max_save_size: int = 64 * 1024 * 1024,
) -> TwoSfEffectiveState:
    if resolved.version != TWOSF_VERSION:
        raise TwoSfError(f"not 2SF: version 0x{resolved.version:02X}")
    rom = OverlayBuffer(max_size=max_rom_size)
    save = OverlayBuffer(max_size=max_save_size)
    save_records: list[SaveRecord] = []
    for stage, obj in enumerate(resolved.objects):
        if obj.program:
            mapping = parse_twosf_map(obj.program, obj.source_id)
            rom.overlay(
                start=mapping.offset,
                payload=mapping.payload,
                source_id=obj.source_id,
                source_offset=8,
                stage_index=stage,
                role="nds-rom-map",
            )
        for record in parse_save_records(obj):
            save.overlay(
                start=record.mapping.offset,
                payload=record.mapping.payload,
                source_id=obj.source_id,
                source_offset=record.reserved_offset + 12,
                stage_index=stage,
                role="nds-save-state-map",
            )
            save_records.append(record)
    contributions = tuple((*rom.contributions, *save.contributions))
    return TwoSfEffectiveState(
        root=resolved.root,
        rom=rom.data,
        save_state=save.data,
        rom_allocated_size=_next_power_of_two(len(rom.data)),
        save_records=tuple(save_records),
        contributions=contributions,
    )
