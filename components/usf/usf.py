"""USF reserved-section patch semantics observed in LazyUSF2."""

from __future__ import annotations

from dataclasses import dataclass
import struct

from components.xsf.envelope import ResolvedXsf, XsfObject
from components.xsf.provenance import ByteContribution, OverlayBuffer


USF_VERSION = 0x21


class UsfError(ValueError):
    pass


@dataclass(frozen=True)
class UsfPatch:
    target: str
    offset: int
    payload: bytes
    reserved_offset: int


@dataclass(frozen=True)
class UsfEffectiveState:
    root: str
    rom: bytes
    save_state: bytes
    contributions: tuple[ByteContribution, ...]
    runtime_available: bool = False


def parse_usf_reserved(obj: XsfObject) -> tuple[UsfPatch, ...]:
    data = obj.reserved
    position = 0
    patches: list[UsfPatch] = []
    for target in ("n64-rom", "project64-save-state"):
        if position + 4 > len(data):
            raise UsfError(f"missing USF {target} table marker: {obj.source_id}")
        marker = data[position : position + 4]
        position += 4
        if marker == b"\x00\x00\x00\x00":
            continue
        if marker != b"SR64":
            raise UsfError(f"invalid USF {target} table marker: {obj.source_id}")
        while True:
            if position + 4 > len(data):
                raise UsfError(f"truncated USF {target} length: {obj.source_id}")
            size = struct.unpack_from("<I", data, position)[0]
            position += 4
            if size == 0:
                break
            if position + 4 + size > len(data):
                raise UsfError(f"USF {target} patch exceeds section: {obj.source_id}")
            offset = struct.unpack_from("<I", data, position)[0]
            position += 4
            payload_offset = position
            payload = data[position : position + size]
            position += size
            patches.append(
                UsfPatch(
                    target=target,
                    offset=offset,
                    payload=payload,
                    reserved_offset=payload_offset,
                )
            )
    if position != len(data):
        raise UsfError(f"trailing bytes in USF reserved section: {obj.source_id}")
    return tuple(patches)


def build_usf_effective_state(
    resolved: ResolvedXsf,
    *,
    max_rom_size: int = 64 * 1024 * 1024,
    max_save_size: int = 64 * 1024 * 1024,
) -> UsfEffectiveState:
    if resolved.version != USF_VERSION:
        raise UsfError(f"not USF: version 0x{resolved.version:02X}")
    rom = OverlayBuffer(max_size=max_rom_size)
    save = OverlayBuffer(max_size=max_save_size)
    contributions: list[ByteContribution] = []
    for stage, obj in enumerate(resolved.objects):
        if obj.program:
            raise UsfError(f"USF program section is outside the LazyUSF2 upload contract: {obj.source_id}")
        for patch in parse_usf_reserved(obj):
            target = rom if patch.target == "n64-rom" else save
            target.overlay(
                start=patch.offset,
                payload=patch.payload,
                source_id=obj.source_id,
                source_offset=patch.reserved_offset,
                stage_index=stage,
                role=patch.target,
            )
            if patch.payload:
                contributions.append(target.contributions[-1])
    return UsfEffectiveState(
        root=resolved.root,
        rom=rom.data,
        save_state=save.data,
        contributions=tuple(contributions),
    )
