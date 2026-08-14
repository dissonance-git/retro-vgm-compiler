"""NCSF sequence-selection and bounded SDAT structure semantics."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import struct

from components.twosf.twosf import TwoSfEffectiveState
from components.xsf.envelope import ResolvedXsf, XsfObject
from components.xsf.provenance import ByteContribution, OverlayBuffer


NCSF_VERSION = 0x25
SDAT_SIGNATURE = b"SDAT\xff\xfe\x00\x01"


class NcsfError(ValueError):
    pass


@dataclass(frozen=True)
class NcsfObjectSelection:
    source_id: str
    sequence_index: int | None
    has_sdat: bool


@dataclass(frozen=True)
class FatReference:
    file_id: int
    offset: int
    size: int
    signature: str


@dataclass(frozen=True)
class SequenceInfo:
    index: int
    file: FatReference
    bank_index: int
    volume: int
    channel_priority: int
    player_priority: int
    player_index: int


@dataclass(frozen=True)
class PlayerInfo:
    index: int
    present: bool
    max_sequences: int | None
    raw_channel_mask: int | None
    effective_channel_mask: int
    heap_size: int | None
    default_applied: bool


@dataclass(frozen=True)
class BankInfo:
    index: int
    file: FatReference
    wave_archive_indices: tuple[int, ...]
    wave_archive_files: tuple[FatReference, ...]


@dataclass(frozen=True)
class SdatStructure:
    declared_size: int
    sequence_count: int
    selected_sequence: SequenceInfo
    selected_player: PlayerInfo
    selected_bank: BankInfo


@dataclass(frozen=True)
class NcsfEffectiveState:
    root: str
    sdat: bytes
    sdat_sha256: str
    selected_sequence_index: int
    selections: tuple[NcsfObjectSelection, ...]
    structure: SdatStructure
    contributions: tuple[ByteContribution, ...]
    runtime_available: bool = False


@dataclass(frozen=True)
class ObservableComparison:
    name: str
    twosf_value: str
    ncsf_value: str
    relation: str


@dataclass(frozen=True)
class PairedRepresentationComparison:
    work: str
    twosf_sdat_offset: int
    twosf_sdat_sha256: str
    ncsf_sdat_sha256: str
    exact_sdat_bytes_match: bool
    observables: tuple[ObservableComparison, ...]


def _u16(data: bytes, offset: int, label: str) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise NcsfError(f"truncated SDAT {label}")
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int, label: str) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise NcsfError(f"truncated SDAT {label}")
    return struct.unpack_from("<I", data, offset)[0]


def _section(data: bytes, offset: int, magic: bytes, label: str) -> tuple[int, int]:
    if offset < 0 or offset + 8 > len(data) or data[offset : offset + 4] != magic:
        raise NcsfError(f"invalid SDAT {label} section")
    size = _u32(data, offset + 4, f"{label} size")
    if size < 8 or offset + size > len(data):
        raise NcsfError(f"SDAT {label} section exceeds declared file")
    return offset, size


def _record(data: bytes, info_offset: int, info_size: int, record_index: int) -> tuple[int, tuple[int, ...]]:
    record_relative = _u32(data, info_offset + 8 + 4 * record_index, "INFO record offset")
    if record_relative == 0:
        return 0, ()
    record = info_offset + record_relative
    info_end = info_offset + info_size
    count = _u32(data, record, "INFO record count")
    if count > (info_end - record - 4) // 4:
        raise NcsfError("SDAT INFO record table exceeds INFO section")
    offsets = tuple(_u32(data, record + 4 + 4 * i, "INFO entry offset") for i in range(count))
    for relative in offsets:
        if relative and not (8 <= relative < info_size):
            raise NcsfError("SDAT INFO entry offset is outside INFO section")
    return count, offsets


def _fat_reference(data: bytes, fat_offset: int, fat_size: int, file_id: int) -> FatReference:
    count = _u32(data, fat_offset + 8, "FAT count")
    if file_id >= count or 12 + 16 * count > fat_size:
        raise NcsfError(f"SDAT FAT file id is out of range: {file_id}")
    record = fat_offset + 12 + 16 * file_id
    offset = _u32(data, record, "FAT file offset")
    size = _u32(data, record + 4, "FAT file size")
    if offset + size > len(data):
        raise NcsfError(f"SDAT FAT file exceeds declared file: {file_id}")
    signature = data[offset : offset + 4].decode("latin-1", errors="replace") if size >= 4 else ""
    return FatReference(file_id=file_id, offset=offset, size=size, signature=signature)


def parse_sdat(data: bytes, selected_sequence_index: int) -> SdatStructure:
    if len(data) < 0x40 or data[:8] != SDAT_SIGNATURE:
        raise NcsfError("NCSF program is not an SDAT with the expected magic")
    declared_size = _u32(data, 8, "declared size")
    if declared_size != len(data):
        raise NcsfError(f"SDAT declared size mismatch: {declared_size} != {len(data)}")
    if _u16(data, 0x0C, "header size") != 0x40:
        raise NcsfError("unsupported SDAT header size")
    info_offset, info_declared = _u32(data, 0x18, "INFO offset"), _u32(data, 0x1C, "INFO size")
    fat_offset, fat_declared = _u32(data, 0x20, "FAT offset"), _u32(data, 0x24, "FAT size")
    _, info_size = _section(data, info_offset, b"INFO", "INFO")
    _, fat_size = _section(data, fat_offset, b"FAT ", "FAT")
    if info_declared != info_size or fat_declared != fat_size:
        raise NcsfError("SDAT section size disagrees with header")

    sequence_count, sequence_offsets = _record(data, info_offset, info_size, 0)
    if selected_sequence_index >= sequence_count:
        raise NcsfError(
            f"NCSF sequence index {selected_sequence_index} is outside {sequence_count} entries"
        )
    sequence_relative = sequence_offsets[selected_sequence_index]
    if sequence_relative == 0:
        raise NcsfError(f"NCSF sequence index {selected_sequence_index} has no INFO entry")
    seq = info_offset + sequence_relative
    if seq + 12 > info_offset + info_size:
        raise NcsfError("selected SDAT SEQ entry is truncated")
    file_id = _u32(data, seq, "SEQ file id")
    bank_index = _u16(data, seq + 4, "SEQ bank")
    player_index = data[seq + 9]
    sequence = SequenceInfo(
        index=selected_sequence_index,
        file=_fat_reference(data, fat_offset, fat_size, file_id),
        bank_index=bank_index,
        volume=data[seq + 6],
        channel_priority=data[seq + 7],
        player_priority=data[seq + 8],
        player_index=player_index,
    )
    if sequence.file.signature != "SSEQ":
        raise NcsfError("selected SDAT SEQ FAT object is not SSEQ")

    bank_count, bank_offsets = _record(data, info_offset, info_size, 2)
    if bank_index >= bank_count or bank_offsets[bank_index] == 0:
        raise NcsfError("selected SDAT SEQ references an invalid BANK entry")
    bank_entry = info_offset + bank_offsets[bank_index]
    if bank_entry + 12 > info_offset + info_size:
        raise NcsfError("selected SDAT BANK entry is truncated")
    bank_file = _fat_reference(data, fat_offset, fat_size, _u32(data, bank_entry, "BANK file id"))
    if bank_file.signature != "SBNK":
        raise NcsfError("selected SDAT BANK FAT object is not SBNK")
    wave_count, wave_offsets = _record(data, info_offset, info_size, 3)
    wave_indices = tuple(
        value
        for value in (_u16(data, bank_entry + 4 + 2 * i, "BANK wave archive") for i in range(4))
        if value != 0xFFFF
    )
    wave_files: list[FatReference] = []
    for wave_index in wave_indices:
        if wave_index >= wave_count or wave_offsets[wave_index] == 0:
            raise NcsfError("selected SDAT BANK references an invalid WAVEARC entry")
        wave_entry = info_offset + wave_offsets[wave_index]
        packed_id = _u32(data, wave_entry, "WAVEARC file id")
        reference = _fat_reference(data, fat_offset, fat_size, packed_id & 0x00FFFFFF)
        if reference.signature != "SWAR":
            raise NcsfError("selected SDAT WAVEARC FAT object is not SWAR")
        wave_files.append(reference)
    bank = BankInfo(
        index=bank_index,
        file=bank_file,
        wave_archive_indices=wave_indices,
        wave_archive_files=tuple(wave_files),
    )

    player_count, player_offsets = _record(data, info_offset, info_size, 4)
    if player_index < player_count and player_offsets[player_index] != 0:
        player_entry = info_offset + player_offsets[player_index]
        if player_entry + 8 > info_offset + info_size:
            raise NcsfError("selected SDAT PLAYER entry is truncated")
        raw_mask = _u16(data, player_entry + 2, "PLAYER channel mask")
        player = PlayerInfo(
            index=player_index,
            present=True,
            max_sequences=data[player_entry],
            raw_channel_mask=raw_mask,
            effective_channel_mask=raw_mask or 0xFFFF,
            heap_size=_u32(data, player_entry + 4, "PLAYER heap size"),
            default_applied=raw_mask == 0,
        )
    else:
        player = PlayerInfo(
            index=player_index,
            present=False,
            max_sequences=None,
            raw_channel_mask=None,
            effective_channel_mask=0xFFFF,
            heap_size=None,
            default_applied=True,
        )
    return SdatStructure(
        declared_size=declared_size,
        sequence_count=sequence_count,
        selected_sequence=sequence,
        selected_player=player,
        selected_bank=bank,
    )


def _inspect_object(obj: XsfObject) -> NcsfObjectSelection:
    if len(obj.reserved) not in {0, 4}:
        raise NcsfError(f"NCSF reserved section must be empty or four bytes: {obj.source_id}")
    selected = struct.unpack_from("<I", obj.reserved)[0] if obj.reserved else None
    if obj.program:
        if len(obj.program) < 12 or obj.program[:8] != SDAT_SIGNATURE:
            raise NcsfError(f"NCSF program is not an SDAT: {obj.source_id}")
        declared = struct.unpack_from("<I", obj.program, 8)[0]
        if declared != len(obj.program):
            raise NcsfError(f"NCSF SDAT declared size mismatch: {obj.source_id}")
    return NcsfObjectSelection(obj.source_id, selected, bool(obj.program))


def build_ncsf_effective_state(resolved: ResolvedXsf) -> NcsfEffectiveState:
    if resolved.version != NCSF_VERSION:
        raise NcsfError(f"not NCSF: version 0x{resolved.version:02X}")
    selections = tuple(_inspect_object(obj) for obj in resolved.objects)
    overlay: OverlayBuffer | None = None
    selected_index: int | None = None
    for stage, (obj, selection) in enumerate(zip(resolved.objects, selections)):
        if obj.program:
            # Current NCSF player behavior sizes the SDAT buffer from the
            # current non-empty program. A later program replaces rather than
            # sparsely patches an earlier SDAT.
            overlay = OverlayBuffer(max_size=512 * 1024 * 1024)
            overlay.overlay(
                start=0,
                payload=obj.program,
                source_id=obj.source_id,
                source_offset=0,
                stage_index=stage,
                role="ncsf-sdat",
            )
        if selection.sequence_index is not None:
            selected_index = selection.sequence_index
    if overlay is None or not overlay.data:
        raise NcsfError("resolved NCSF dependency graph contains no SDAT")
    if selected_index is None:
        raise NcsfError("resolved NCSF dependency graph contains no sequence selection")
    structure = parse_sdat(overlay.data, selected_index)
    return NcsfEffectiveState(
        root=resolved.root,
        sdat=overlay.data,
        sdat_sha256=hashlib.sha256(overlay.data).hexdigest(),
        selected_sequence_index=selected_index,
        selections=selections,
        structure=structure,
        contributions=tuple(overlay.contributions),
    )


def compare_twosf_ncsf(
    twosf: TwoSfEffectiveState,
    ncsf: NcsfEffectiveState,
    *,
    work: str,
) -> PairedRepresentationComparison:
    candidates: list[tuple[int, bytes]] = []
    start = 0
    while True:
        offset = twosf.rom.find(SDAT_SIGNATURE, start)
        if offset < 0:
            break
        if offset + 12 <= len(twosf.rom):
            size = struct.unpack_from("<I", twosf.rom, offset + 8)[0]
            if size >= 0x40 and offset + size <= len(twosf.rom):
                candidates.append((offset, twosf.rom[offset : offset + size]))
        start = offset + 1
    if len(candidates) != 1:
        raise NcsfError(f"expected one bounded SDAT in 2SF effective ROM, found {len(candidates)}")
    offset, twosf_sdat = candidates[0]
    twosf_hash = hashlib.sha256(twosf_sdat).hexdigest()
    ncsf_hash = hashlib.sha256(ncsf.sdat).hexdigest()
    exact = twosf_sdat == ncsf.sdat
    return PairedRepresentationComparison(
        work=work,
        twosf_sdat_offset=offset,
        twosf_sdat_sha256=twosf_hash,
        ncsf_sdat_sha256=ncsf_hash,
        exact_sdat_bytes_match=exact,
        observables=(
            ObservableComparison("container-version", "0x24", "0x25", "representation-different"),
            ObservableComparison("effective-sdat-bytes", twosf_hash, ncsf_hash, "equal" if exact else "different"),
            ObservableComparison(
                "selected-sequence-index",
                "not exposed by bounded 2SF loader",
                str(ncsf.selected_sequence_index),
                "not-comparable",
            ),
            ObservableComparison(
                "machine-runtime",
                str(twosf.runtime_available).lower(),
                str(ncsf.runtime_available).lower(),
                "same-availability-not-audio-equivalence",
            ),
        ),
    )
