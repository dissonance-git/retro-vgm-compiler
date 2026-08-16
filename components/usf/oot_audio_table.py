"""Bounded Ocarina of Time audio-table discovery over reconstructed USF ROM.

The ZeldaRET decompilation establishes the binary AudioTable structure, but the
location of that table and the base of the sequence-data segment vary with ROM
layout/version.  This module therefore requires both offsets from an external,
version-specific witness instead of hard-coding one retail revision.

A reconstructed USF ROM is sparse evidence.  Zero-filled gaps created while
applying SR64 patches are not observed ROM bytes, so all accepted table and
sequence spans must also be covered by USF byte provenance.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
import struct

from components.xsf.provenance import ByteContribution

from .usf import UsfEffectiveState


OOT_AUDIO_TABLE_HEADER_SIZE = 0x10
OOT_AUDIO_TABLE_ENTRY_SIZE = 0x10


class OotAudioTableError(ValueError):
    pass


class OotAudioStorageMedium(IntEnum):
    RAM = 0
    UNKNOWN = 1
    CART = 2
    DISK_DRIVE = 3


class OotAudioCachePolicy(IntEnum):
    LOAD_PERMANENT = 0
    LOAD_PERSISTENT = 1
    LOAD_TEMPORARY = 2
    LOAD_EITHER = 3
    LOAD_EITHER_NOSYNC = 4


@dataclass(frozen=True)
class OotAudioTableEntry:
    entry_id: int
    rom_addr: int
    size: int
    medium: OotAudioStorageMedium
    cache_policy: OotAudioCachePolicy
    short_data1: int
    short_data2: int
    short_data3: int
    table_offset: int


@dataclass(frozen=True)
class OotAudioTable:
    offset: int
    num_entries: int
    unknown_medium_param: int
    rom_addr: int
    entries: tuple[OotAudioTableEntry, ...]


@dataclass(frozen=True)
class OotSequenceCandidate:
    sequence_id: int
    data_start: int
    data_end: int
    data: bytes
    table_entry: OotAudioTableEntry
    provenance: tuple[ByteContribution, ...]


@dataclass(frozen=True)
class OotSequenceEntryAssessment:
    sequence_id: int
    accepted: bool
    classification: str
    reasons: tuple[str, ...]
    candidate: OotSequenceCandidate | None = None


def _merge_ranges(contributions: tuple[ByteContribution, ...]) -> tuple[tuple[int, int], ...]:
    ranges: list[tuple[int, int]] = []
    for item in sorted(contributions, key=lambda value: (value.target_start, value.target_end)):
        if item.role != "n64-rom":
            continue
        if not ranges or item.target_start > ranges[-1][1]:
            ranges.append((item.target_start, item.target_end))
        else:
            ranges[-1] = (ranges[-1][0], max(ranges[-1][1], item.target_end))
    return tuple(ranges)


def _span_is_observed(
    start: int,
    end: int,
    observed_ranges: tuple[tuple[int, int], ...] | None,
) -> bool:
    if observed_ranges is None:
        return True
    if start < 0 or end < start:
        return False
    if start == end:
        return True
    cursor = start
    for range_start, range_end in sorted(observed_ranges):
        if range_end <= cursor:
            continue
        if range_start > cursor:
            return False
        cursor = max(cursor, range_end)
        if cursor >= end:
            return True
    return False


def _subtract_interval(
    intervals: list[tuple[int, int]],
    cover_start: int,
    cover_end: int,
) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for start, end in intervals:
        if cover_end <= start or cover_start >= end:
            result.append((start, end))
            continue
        if start < cover_start:
            result.append((start, min(end, cover_start)))
        if cover_end < end:
            result.append((max(start, cover_end), end))
    return result


def _effective_provenance_for_span(
    contributions: tuple[ByteContribution, ...],
    start: int,
    end: int,
) -> tuple[ByteContribution, ...]:
    """Return only contributions that still supply final bytes in the span.

    OverlayBuffer records every write.  Walking writes in reverse stage order
    and subtracting covered intervals prevents fully shadowed library patches
    from being reported as effective sequence provenance.
    """

    if start >= end:
        return ()
    uncovered = [(start, end)]
    selected: list[ByteContribution] = []
    ordered = sorted(
        (item for item in contributions if item.role == "n64-rom"),
        key=lambda item: item.stage_index,
        reverse=True,
    )
    for item in ordered:
        if not any(item.target_start < right and item.target_end > left for left, right in uncovered):
            continue
        selected.append(item)
        uncovered = _subtract_interval(uncovered, item.target_start, item.target_end)
        if not uncovered:
            break
    return tuple(reversed(selected))


def parse_oot_audio_table(
    rom: bytes,
    table_offset: int,
    *,
    max_entries: int = 512,
    observed_ranges: tuple[tuple[int, int], ...] | None = None,
) -> OotAudioTable:
    """Parse one Zelda64 AudioTable at a caller-established ROM offset."""

    if table_offset < 0:
        raise OotAudioTableError("OoT audio table offset cannot be negative")
    header_end = table_offset + OOT_AUDIO_TABLE_HEADER_SIZE
    if header_end > len(rom):
        raise OotAudioTableError("OoT audio table header exceeds reconstructed ROM")
    if not _span_is_observed(table_offset, header_end, observed_ranges):
        raise OotAudioTableError("OoT audio table header crosses unobserved USF ROM bytes")

    num_entries, unknown_medium_param, rom_addr = struct.unpack_from(">hhI", rom, table_offset)
    if num_entries < 0 or num_entries > max_entries:
        raise OotAudioTableError("OoT audio table entry count exceeds bounded parser contract")
    if any(rom[table_offset + 8 : header_end]):
        raise OotAudioTableError("OoT audio table reserved header bytes are nonzero")

    table_end = header_end + num_entries * OOT_AUDIO_TABLE_ENTRY_SIZE
    if table_end > len(rom):
        raise OotAudioTableError("OoT audio table entries exceed reconstructed ROM")
    if not _span_is_observed(header_end, table_end, observed_ranges):
        raise OotAudioTableError("OoT audio table entries cross unobserved USF ROM bytes")

    entries: list[OotAudioTableEntry] = []
    for entry_id in range(num_entries):
        offset = header_end + entry_id * OOT_AUDIO_TABLE_ENTRY_SIZE
        raw = struct.unpack_from(">IIbbhhh", rom, offset)
        entry_rom_addr, size, medium_raw, cache_raw, short1, short2, short3 = raw
        try:
            medium = OotAudioStorageMedium(medium_raw)
        except ValueError as exc:
            raise OotAudioTableError(
                f"OoT audio table entry {entry_id} has unknown storage medium {medium_raw}"
            ) from exc
        try:
            cache_policy = OotAudioCachePolicy(cache_raw)
        except ValueError as exc:
            raise OotAudioTableError(
                f"OoT audio table entry {entry_id} has unknown cache policy {cache_raw}"
            ) from exc
        entries.append(
            OotAudioTableEntry(
                entry_id=entry_id,
                rom_addr=entry_rom_addr,
                size=size,
                medium=medium,
                cache_policy=cache_policy,
                short_data1=short1,
                short_data2=short2,
                short_data3=short3,
                table_offset=offset,
            )
        )

    return OotAudioTable(
        offset=table_offset,
        num_entries=num_entries,
        unknown_medium_param=unknown_medium_param,
        rom_addr=rom_addr,
        entries=tuple(entries),
    )


def assess_oot_sequence_entries(
    rom: bytes,
    table: OotAudioTable,
    *,
    sequence_data_base: int,
    observed_ranges: tuple[tuple[int, int], ...] | None = None,
    contributions: tuple[ByteContribution, ...] = (),
) -> tuple[OotSequenceEntryAssessment, ...]:
    """Project non-empty cart entries into bounded sequence byte candidates.

    `sequence_data_base` is intentionally caller supplied.  ZeldaRET's
    extraction tooling applies each entry's `romAddr` relative to a segment
    offset, so inferring that base from an arbitrary USF set would overstate the
    available evidence.
    """

    if sequence_data_base < 0:
        raise OotAudioTableError("OoT sequence data base cannot be negative")

    assessments: list[OotSequenceEntryAssessment] = []
    for entry in table.entries:
        if entry.size == 0:
            assessments.append(
                OotSequenceEntryAssessment(
                    sequence_id=entry.entry_id,
                    accepted=False,
                    classification="zero-size-entry",
                    reasons=(
                        "entry contains no directly extractable byte span; table-index indirection/alias semantics require separate evidence",
                    ),
                )
            )
            continue
        if entry.medium is not OotAudioStorageMedium.CART:
            assessments.append(
                OotSequenceEntryAssessment(
                    sequence_id=entry.entry_id,
                    accepted=False,
                    classification="non-cart-entry",
                    reasons=(
                        f"entry storage medium is {entry.medium.name}; ROM extraction is not established for this entry",
                    ),
                )
            )
            continue

        start = sequence_data_base + entry.rom_addr
        end = start + entry.size
        reasons: list[str] = []
        if start < sequence_data_base or end < start:
            reasons.append("sequence address arithmetic overflowed or wrapped")
        if end > len(rom):
            reasons.append("sequence span exceeds reconstructed ROM")
        elif not _span_is_observed(start, end, observed_ranges):
            reasons.append("sequence span crosses unobserved USF ROM bytes")

        if reasons:
            assessments.append(
                OotSequenceEntryAssessment(
                    sequence_id=entry.entry_id,
                    accepted=False,
                    classification="rejected-sequence-span",
                    reasons=tuple(reasons),
                )
            )
            continue

        candidate = OotSequenceCandidate(
            sequence_id=entry.entry_id,
            data_start=start,
            data_end=end,
            data=rom[start:end],
            table_entry=entry,
            provenance=_effective_provenance_for_span(contributions, start, end),
        )
        assessments.append(
            OotSequenceEntryAssessment(
                sequence_id=entry.entry_id,
                accepted=True,
                classification="sequence-byte-candidate",
                reasons=(),
                candidate=candidate,
            )
        )

    return tuple(assessments)


def scan_usf_oot_sequence_entries(
    state: UsfEffectiveState,
    *,
    table_offset: int,
    sequence_data_base: int,
    max_entries: int = 512,
) -> tuple[OotSequenceEntryAssessment, ...]:
    """Discover OoT sequence byte candidates without claiming MML execution."""

    observed_ranges = _merge_ranges(state.contributions)
    table = parse_oot_audio_table(
        state.rom,
        table_offset,
        max_entries=max_entries,
        observed_ranges=observed_ranges,
    )
    return assess_oot_sequence_entries(
        state.rom,
        table,
        sequence_data_base=sequence_data_base,
        observed_ranges=observed_ranges,
        contributions=state.contributions,
    )
