"""Bounded AKAO structural probing over reconstructed PSF1 memory.

This module deliberately stops below AKAO event execution.  It recognizes only
structural sequence candidates that satisfy invariants independently visible in
VGMTrans's reverse-engineered parser.  A candidate is not proof of exact AKAO
version, driver behavior, authored source, or runtime use.
"""

from __future__ import annotations

from dataclasses import dataclass
import struct

from .psf1 import Psf1EffectiveImage


AKAO_SIGNATURE = b"AKAO"
AKAO_V3_HEADER_SIZE = 0x40
AKAO_V3_RESERVED_WORD_OFFSETS = (0x28, 0x2C, 0x38, 0x3C)


@dataclass(frozen=True)
class AkaoV3SequenceCandidate:
    offset: int
    address: int
    declared_length: int
    sequence_id: int
    sample_set_id: int
    track_bits: int
    track_count: int
    track_addresses: tuple[int, ...]
    instrument_address: int | None
    drumkit_address: int | None
    raw_fe13_pairs: int
    warnings: tuple[str, ...]
    version_evidence: str = "v3-compatible; VGMTrans-style 3.2 heuristic only"


@dataclass(frozen=True)
class AkaoSignatureAssessment:
    offset: int
    address: int
    accepted: bool
    classification: str
    reasons: tuple[str, ...]
    sequence: AkaoV3SequenceCandidate | None = None


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def assess_akao_signature(
    memory: bytes,
    offset: int,
    *,
    memory_base: int = 0,
) -> AkaoSignatureAssessment:
    """Assess one literal AKAO signature without decoding sequence events."""

    address = memory_base + offset
    if offset < 0 or offset + 8 > len(memory) or memory[offset : offset + 4] != AKAO_SIGNATURE:
        return AkaoSignatureAssessment(
            offset=offset,
            address=address,
            accepted=False,
            classification="not-akao",
            reasons=("signature mismatch or truncated fixed prefix",),
        )

    declared_length = _u16(memory, offset + 6)
    if declared_length == 0:
        return AkaoSignatureAssessment(
            offset=offset,
            address=address,
            accepted=False,
            classification="non-sequence-signature",
            reasons=("declared sequence length is zero; this may be an AKAO sample collection",),
        )

    if offset + AKAO_V3_HEADER_SIZE > len(memory):
        return AkaoSignatureAssessment(
            offset=offset,
            address=address,
            accepted=False,
            classification="truncated-sequence-candidate",
            reasons=("v3 header exceeds reconstructed memory",),
        )

    reasons: list[str] = []
    warnings: list[str] = []
    if declared_length < AKAO_V3_HEADER_SIZE:
        reasons.append("declared length is smaller than the v3 header")
    if offset + declared_length > len(memory):
        reasons.append("declared sequence span exceeds reconstructed memory")

    for relative in AKAO_V3_RESERVED_WORD_OFFSETS:
        if _u32(memory, offset + relative) != 0:
            reasons.append(f"v3 reserved word +0x{relative:02X} is nonzero")

    track_bits = _u32(memory, offset + 0x20)
    track_count = track_bits.bit_count()
    if track_count == 0:
        reasons.append("track allocation bitmap is empty")

    pointer_table_end = AKAO_V3_HEADER_SIZE + 2 * track_count
    if declared_length < pointer_table_end:
        reasons.append("declared length does not contain the complete track pointer table")

    track_addresses: list[int] = []
    if not reasons:
        for track_index in range(track_count):
            pointer_relative = AKAO_V3_HEADER_SIZE + 2 * track_index
            target_relative = pointer_relative + _u16(memory, offset + pointer_relative)
            if target_relative < pointer_table_end:
                warnings.append(f"track {track_index} points before the end of the pointer table")
            if target_relative >= declared_length:
                reasons.append(f"track {track_index} target lies outside the declared sequence span")
                continue
            track_addresses.append(address + target_relative)

    instrument_address: int | None = None
    drumkit_address: int | None = None
    if not reasons:
        for field_relative, name in ((0x30, "instrument"), (0x34, "drumkit")):
            relative = _u32(memory, offset + field_relative)
            if relative == 0:
                continue
            target_relative = field_relative + relative
            if target_relative >= declared_length:
                reasons.append(f"{name} target lies outside the declared sequence span")
            elif name == "instrument":
                instrument_address = address + target_relative
            else:
                drumkit_address = address + target_relative

    if reasons:
        return AkaoSignatureAssessment(
            offset=offset,
            address=address,
            accepted=False,
            classification="rejected-sequence-candidate",
            reasons=tuple(reasons),
        )

    sequence_bytes = memory[offset : offset + declared_length]
    sequence = AkaoV3SequenceCandidate(
        offset=offset,
        address=address,
        declared_length=declared_length,
        sequence_id=_u16(memory, offset + 4),
        sample_set_id=_u16(memory, offset + 0x14),
        track_bits=track_bits,
        track_count=track_count,
        track_addresses=tuple(track_addresses),
        instrument_address=instrument_address,
        drumkit_address=drumkit_address,
        raw_fe13_pairs=sequence_bytes.count(b"\xFE\x13"),
        warnings=tuple(warnings),
    )
    return AkaoSignatureAssessment(
        offset=offset,
        address=address,
        accepted=True,
        classification="v3-sequence-candidate",
        reasons=(),
        sequence=sequence,
    )


def scan_akao_signatures(
    memory: bytes,
    *,
    memory_base: int = 0,
) -> tuple[AkaoSignatureAssessment, ...]:
    """Assess every literal AKAO signature in one reconstructed memory image."""

    results: list[AkaoSignatureAssessment] = []
    start = 0
    while True:
        offset = memory.find(AKAO_SIGNATURE, start)
        if offset < 0:
            break
        results.append(assess_akao_signature(memory, offset, memory_base=memory_base))
        start = offset + 1
    return tuple(results)


def scan_psf1_akao(image: Psf1EffectiveImage) -> tuple[AkaoSignatureAssessment, ...]:
    """Scan a PSF1 effective image without claiming CPU or SPU execution."""

    return scan_akao_signatures(image.memory, memory_base=image.memory_base)
