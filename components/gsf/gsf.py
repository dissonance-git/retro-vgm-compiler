"""GSF-specific GBA upload semantics without claiming machine execution."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import struct

from components.xsf.envelope import ResolvedXsf, XsfObject
from components.xsf.provenance import ByteContribution, OverlayBuffer


GSF_VERSION = 0x22
GBA_EWRAM_BASE = 0x02000000
GBA_EWRAM_SIZE = 0x00040000
GBA_ROM_BASE = 0x08000000
GBA_ROM_SIZE = 0x02000000
GSF_DRIVER_EVIDENCE_STATES = frozenset({
    "exact/source-established",
    "signature-supported",
    "behavioral-candidate",
    "unknown",
})


class GsfError(ValueError):
    pass


@dataclass(frozen=True)
class GsfUpload:
    source_id: str
    entry_address: int
    load_address: int
    payload_size: int
    payload: bytes
    address_space: str


@dataclass(frozen=True)
class GsfEffectiveImage:
    root: str
    memory_base: int
    image: bytes
    image_sha256: str
    selected_entry_address: int
    uploads: tuple[GsfUpload, ...]
    contributions: tuple[ByteContribution, ...]
    populated_ranges: tuple[tuple[int, int], ...]
    driver_evidence: str = "unknown"
    runtime_available: bool = False

    def provenance_at_address(self, address: int) -> ByteContribution | None:
        offset = address - self.memory_base
        for contribution in reversed(self.contributions):
            if contribution.target_start <= offset < contribution.target_end:
                return contribution
        return None

    def address_is_populated(self, address: int) -> bool:
        offset = address - self.memory_base
        return any(start <= offset < end for start, end in self.populated_ranges)


def inspect_gsf_upload(obj: XsfObject) -> GsfUpload:
    if obj.reserved:
        raise GsfError(f"GSF reserved section must be empty: {obj.source_id}")
    if len(obj.program) < 12:
        raise GsfError(f"truncated 12-byte GSF program header: {obj.source_id}")
    entry, load_address, payload_size = struct.unpack_from("<III", obj.program)
    if len(obj.program) != 12 + payload_size:
        raise GsfError(f"GSF payload size does not match program section: {obj.source_id}")
    if GBA_EWRAM_BASE <= load_address < GBA_EWRAM_BASE + GBA_EWRAM_SIZE:
        address_space = "gba-ewram"
        limit = GBA_EWRAM_BASE + GBA_EWRAM_SIZE
        entry_matches_space = GBA_EWRAM_BASE <= entry < GBA_EWRAM_BASE + GBA_EWRAM_SIZE
    elif GBA_ROM_BASE <= load_address < GBA_ROM_BASE + GBA_ROM_SIZE:
        address_space = "gba-rom"
        limit = GBA_ROM_BASE + GBA_ROM_SIZE
        entry_matches_space = GBA_ROM_BASE <= entry < GBA_ROM_BASE + GBA_ROM_SIZE
    else:
        raise GsfError(f"GSF load address is outside supported GBA spaces: {obj.source_id}")
    if load_address + payload_size > limit:
        raise GsfError(f"GSF upload exceeds {address_space}: {obj.source_id}")
    if not entry_matches_space:
        raise GsfError(
            f"GSF entry/load address spaces disagree for {obj.source_id}: "
            f"entry=0x{entry:08X}, load=0x{load_address:08X}"
        )
    return GsfUpload(
        source_id=obj.source_id,
        entry_address=entry,
        load_address=load_address,
        payload_size=payload_size,
        payload=obj.program[12:],
        address_space=address_space,
    )


def build_gsf_effective_image(
    resolved: ResolvedXsf,
    *,
    driver_evidence: str = "unknown",
) -> GsfEffectiveImage:
    if resolved.version != GSF_VERSION:
        raise GsfError(f"not GSF: version 0x{resolved.version:02X}")
    if driver_evidence not in GSF_DRIVER_EVIDENCE_STATES:
        raise GsfError(f"invalid driver evidence state: {driver_evidence}")
    uploads = tuple(inspect_gsf_upload(obj) for obj in resolved.objects)
    if not uploads:
        raise GsfError("empty GSF resolution")
    spaces = {upload.address_space for upload in uploads}
    if len(spaces) != 1:
        raise GsfError("mixed EWRAM and ROM uploads are not represented as one flat GSF image")
    entries = {upload.entry_address for upload in uploads}
    if len(entries) != 1:
        details = ", ".join(
            f"{upload.source_id}=0x{upload.entry_address:08X}" for upload in uploads
        )
        raise GsfError(f"ambiguous GSF entry state across dependency objects: {details}")
    space = next(iter(spaces))
    memory_base = GBA_EWRAM_BASE if space == "gba-ewram" else GBA_ROM_BASE
    max_size = GBA_EWRAM_SIZE if space == "gba-ewram" else GBA_ROM_SIZE
    overlay = OverlayBuffer(max_size=max_size)
    for stage, upload in enumerate(uploads):
        overlay.overlay(
            start=upload.load_address - memory_base,
            payload=upload.payload,
            source_id=upload.source_id,
            source_offset=12,
            stage_index=stage,
            role="gsf-gba-upload",
        )
    root_upload = next((item for item in uploads if item.source_id == resolved.root), None)
    if root_upload is None:
        raise GsfError("resolved GSF root object is missing")
    return GsfEffectiveImage(
        root=resolved.root,
        memory_base=memory_base,
        image=overlay.data,
        image_sha256=hashlib.sha256(overlay.data).hexdigest(),
        selected_entry_address=root_upload.entry_address,
        uploads=uploads,
        contributions=tuple(overlay.contributions),
        populated_ranges=overlay.populated_ranges(),
        driver_evidence=driver_evidence,
    )
