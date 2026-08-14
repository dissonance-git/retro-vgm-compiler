"""PSF1-specific PS-X EXE reconstruction without claiming runtime execution."""

from __future__ import annotations

from dataclasses import dataclass
import struct

from components.xsf.envelope import ResolvedXsf, XsfObject
from components.xsf.provenance import ByteContribution, OverlayBuffer


PSF1_VERSION = 0x01
PS_X_EXE_HEADER_SIZE = 0x800
PSF1_MAX_TEXT_SIZE = 2_033_664


class Psf1Error(ValueError):
    pass


@dataclass(frozen=True)
class Psf1Executable:
    source_id: str
    initial_pc: int
    text_start: int
    text_size: int
    initial_sp: int
    region_marker: str


@dataclass(frozen=True)
class Psf1EffectiveImage:
    root: str
    memory_base: int
    memory: bytes
    entry: Psf1Executable
    executables: tuple[Psf1Executable, ...]
    contributions: tuple[ByteContribution, ...]
    runtime_available: bool = False

    def provenance_at_address(self, address: int) -> ByteContribution | None:
        offset = address - self.memory_base
        for contribution in reversed(self.contributions):
            if contribution.target_start <= offset < contribution.target_end:
                return contribution
        return None


def inspect_ps_x_exe(obj: XsfObject) -> Psf1Executable:
    data = obj.program
    if len(data) < PS_X_EXE_HEADER_SIZE or data[:8] != b"PS-X EXE":
        raise Psf1Error(f"PSF1 program is not a PS-X EXE: {obj.source_id}")
    initial_pc = struct.unpack_from("<I", data, 0x10)[0]
    text_start = struct.unpack_from("<I", data, 0x18)[0]
    text_size = struct.unpack_from("<I", data, 0x1C)[0]
    initial_sp = struct.unpack_from("<I", data, 0x30)[0]
    if text_size > PSF1_MAX_TEXT_SIZE:
        raise Psf1Error(f"PS-X EXE text exceeds PSF1 maximum: {obj.source_id}")
    if PS_X_EXE_HEADER_SIZE + text_size > len(data):
        raise Psf1Error(f"truncated PS-X EXE text: {obj.source_id}")
    if text_start + text_size > 0x1_0000_0000:
        raise Psf1Error(f"PS-X EXE text address overflow: {obj.source_id}")
    marker_bytes = data[0x4C:PS_X_EXE_HEADER_SIZE].split(b"\x00", 1)[0]
    return Psf1Executable(
        source_id=obj.source_id,
        initial_pc=initial_pc,
        text_start=text_start,
        text_size=text_size,
        initial_sp=initial_sp,
        region_marker=marker_bytes.decode("latin-1", errors="replace"),
    )


def build_psf1_effective_image(
    resolved: ResolvedXsf,
    *,
    max_address_span: int = 32 * 1024 * 1024,
) -> Psf1EffectiveImage:
    if resolved.version != PSF1_VERSION:
        raise Psf1Error(f"not PSF1: version 0x{resolved.version:02X}")
    inspected = tuple(inspect_ps_x_exe(obj) for obj in resolved.objects)
    if not inspected:
        raise Psf1Error("empty PSF1 resolution")
    minimum = min(exe.text_start for exe in inspected)
    maximum = max(exe.text_start + exe.text_size for exe in inspected)
    if maximum - minimum > max_address_span:
        raise Psf1Error("effective PSF1 address span exceeds safety bound")
    overlay = OverlayBuffer(max_size=max_address_span)
    for stage, (obj, exe) in enumerate(zip(resolved.objects, inspected)):
        overlay.overlay(
            start=exe.text_start - minimum,
            payload=obj.program[PS_X_EXE_HEADER_SIZE : PS_X_EXE_HEADER_SIZE + exe.text_size],
            source_id=obj.source_id,
            source_offset=PS_X_EXE_HEADER_SIZE,
            stage_index=stage,
            role="ps-x-exe-text",
        )
    entry = next((exe for exe in inspected if exe.source_id == resolved.root), inspected[-1])
    return Psf1EffectiveImage(
        root=resolved.root,
        memory_base=minimum,
        memory=overlay.data,
        entry=entry,
        executables=inspected,
        contributions=tuple(overlay.contributions),
    )
