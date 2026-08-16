"""Structural SDAT discovery for Nintendo DS effective ROM images.

A raw `b"SDAT"` hit is not evidence that a sound archive exists.  This probe
requires the standard SDAT signature, a bounded declared size, the 0x40-byte
header contract used by the existing NCSF parser, internally consistent INFO
and FAT section references, and observed 2SF ROM provenance for the entire
candidate span.

The output is still only an archive-byte candidate.  Sequence selection and
runtime execution remain separate evidence layers.
"""

from __future__ import annotations

from dataclasses import dataclass
import struct

from components.twosf.twosf import TwoSfEffectiveState
from components.xsf.provenance import ByteContribution


SDAT_SIGNATURE = b"SDAT\xff\xfe\x00\x01"
SDAT_HEADER_SIZE = 0x40


@dataclass(frozen=True)
class SdatCandidate:
    offset: int
    size: int
    data: bytes
    provenance: tuple[ByteContribution, ...]


@dataclass(frozen=True)
class SdatCandidateAssessment:
    offset: int
    accepted: bool
    classification: str
    reasons: tuple[str, ...]
    candidate: SdatCandidate | None = None


def _rom_contributions(
    contributions: tuple[ByteContribution, ...],
) -> tuple[ByteContribution, ...]:
    return tuple(item for item in contributions if item.role == "nds-rom-map")


def _merge_observed_ranges(
    contributions: tuple[ByteContribution, ...],
) -> tuple[tuple[int, int], ...]:
    ranges: list[tuple[int, int]] = []
    for item in sorted(_rom_contributions(contributions), key=lambda x: (x.target_start, x.target_end)):
        if not ranges or item.target_start > ranges[-1][1]:
            ranges.append((item.target_start, item.target_end))
        else:
            ranges[-1] = (ranges[-1][0], max(ranges[-1][1], item.target_end))
    return tuple(ranges)


def _span_is_observed(
    start: int,
    end: int,
    ranges: tuple[tuple[int, int], ...],
) -> bool:
    cursor = start
    for left, right in ranges:
        if right <= cursor:
            continue
        if left > cursor:
            return False
        cursor = max(cursor, right)
        if cursor >= end:
            return True
    return start == end


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


def _effective_provenance(
    contributions: tuple[ByteContribution, ...],
    start: int,
    end: int,
) -> tuple[ByteContribution, ...]:
    """Return only writes that still supply final candidate bytes."""

    uncovered = [(start, end)]
    selected: list[tuple[int, ByteContribution]] = []
    indexed = list(enumerate(_rom_contributions(contributions)))
    for original_index, item in sorted(
        indexed,
        key=lambda pair: (pair[1].stage_index, pair[0]),
        reverse=True,
    ):
        if not any(item.target_start < right and item.target_end > left for left, right in uncovered):
            continue
        selected.append((original_index, item))
        uncovered = _subtract_interval(uncovered, item.target_start, item.target_end)
        if not uncovered:
            break
    return tuple(item for _, item in sorted(selected, key=lambda pair: (pair[1].stage_index, pair[0])))


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _validate_section(
    candidate: bytes,
    *,
    header_offset: int,
    expected_magic: bytes,
    label: str,
) -> str | None:
    section_offset = _u32(candidate, header_offset)
    section_declared_size = _u32(candidate, header_offset + 4)
    if section_offset < SDAT_HEADER_SIZE or section_offset + 8 > len(candidate):
        return f"{label} section offset is outside candidate"
    if candidate[section_offset : section_offset + 4] != expected_magic:
        return f"{label} section magic does not match header reference"
    actual_size = _u32(candidate, section_offset + 4)
    if actual_size < 8 or section_offset + actual_size > len(candidate):
        return f"{label} section exceeds candidate"
    if section_declared_size != actual_size:
        return f"{label} section size disagrees with SDAT header"
    return None


def assess_sdat_at(
    rom: bytes,
    offset: int,
    *,
    contributions: tuple[ByteContribution, ...] = (),
    require_observed: bool = True,
    max_size: int = 128 * 1024 * 1024,
) -> SdatCandidateAssessment:
    """Assess one possible SDAT starting at `offset`."""

    reasons: list[str] = []
    if offset < 0 or offset + 12 > len(rom):
        return SdatCandidateAssessment(offset, False, "truncated-header", ("candidate header exceeds ROM",))
    if rom[offset : offset + 8] != SDAT_SIGNATURE:
        return SdatCandidateAssessment(offset, False, "signature-mismatch", ("expected SDAT signature is absent",))

    declared_size = _u32(rom, offset + 8)
    if declared_size < SDAT_HEADER_SIZE:
        reasons.append("declared SDAT size is smaller than header")
    if declared_size > max_size:
        reasons.append("declared SDAT size exceeds bounded probe limit")
    end = offset + declared_size
    if end < offset or end > len(rom):
        reasons.append("declared SDAT span exceeds effective ROM")
    if reasons:
        return SdatCandidateAssessment(offset, False, "invalid-declared-span", tuple(reasons))

    candidate = rom[offset:end]
    if _u16(candidate, 0x0C) != SDAT_HEADER_SIZE:
        reasons.append("unsupported SDAT header size")
    # NCSF already relies on INFO and FAT as the minimum useful structural
    # evidence.  SYMB/FILE may be absent from synthetic or stripped archives.
    for header_offset, magic, label in (
        (0x18, b"INFO", "INFO"),
        (0x20, b"FAT ", "FAT"),
    ):
        error = _validate_section(
            candidate,
            header_offset=header_offset,
            expected_magic=magic,
            label=label,
        )
        if error:
            reasons.append(error)

    ranges = _merge_observed_ranges(contributions)
    if require_observed and not _span_is_observed(offset, end, ranges):
        reasons.append("SDAT span crosses unobserved 2SF ROM bytes")

    if reasons:
        return SdatCandidateAssessment(offset, False, "structurally-rejected", tuple(reasons))

    return SdatCandidateAssessment(
        offset=offset,
        accepted=True,
        classification="sdat-byte-candidate",
        reasons=(),
        candidate=SdatCandidate(
            offset=offset,
            size=declared_size,
            data=candidate,
            provenance=_effective_provenance(contributions, offset, end),
        ),
    )


def scan_sdat_candidates(
    state: TwoSfEffectiveState,
    *,
    max_size: int = 128 * 1024 * 1024,
) -> tuple[SdatCandidateAssessment, ...]:
    """Assess every SDAT signature hit in a reconstructed 2SF ROM."""

    assessments: list[SdatCandidateAssessment] = []
    start = 0
    while True:
        offset = state.rom.find(SDAT_SIGNATURE, start)
        if offset < 0:
            break
        assessments.append(
            assess_sdat_at(
                state.rom,
                offset,
                contributions=state.contributions,
                require_observed=True,
                max_size=max_size,
            )
        )
        start = offset + 1
    return tuple(assessments)


def accepted_sdat_candidates(
    state: TwoSfEffectiveState,
    *,
    max_size: int = 128 * 1024 * 1024,
) -> tuple[SdatCandidate, ...]:
    return tuple(
        assessment.candidate
        for assessment in scan_sdat_candidates(state, max_size=max_size)
        if assessment.accepted and assessment.candidate is not None
    )
