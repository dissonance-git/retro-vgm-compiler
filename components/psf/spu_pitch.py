"""PlayStation SPU pitch-step analysis helpers.

This module preserves two distinct low-level disagreements rather than selecting
one implementation as truth:

1. DuckStation, Beetle/Mednafen, and PSX-SPX model PMON's high-bit VxPitch
   behavior with a signed 16-bit carrier multiply.  The pinned MiSTer FPGA RTL
   instead performs an unsigned multiply.
2. DuckStation, Beetle/Mednafen, and MiSTer cap the resulting step at 0x3FFF,
   while PSX-SPX documents an over-range step as 0x4000.

For ordinary VxPitch values below 0x8000 the signed/unsigned PMON raw transforms
agree.  The arithmetic disagreement becomes observable only in the high-bit
region and must remain explicit until stronger hardware evidence settles it.

These helpers describe a physical SPU coordinate.  They do not map AKAO notes,
logical tracks, tuning fields, or musical pitch spelling to hardware voices.
"""

from __future__ import annotations

from dataclasses import dataclass


PLAYSTATION_SPU_VOICE_COUNT = 24
SPU_PITCH_UNITY = 0x1000
SPU_PITCH_IMPLEMENTATION_MAX = 0x3FFF
SPU_PITCH_DOCUMENTED_MAX = 0x4000


@dataclass(frozen=True)
class SpuRawPmonCandidates:
    signed_carrier: int
    unsigned_carrier_fpga: int
    disagree: bool


@dataclass(frozen=True)
class SpuPitchStepCandidates:
    raw_signed_carrier: int
    raw_unsigned_carrier_fpga: int
    signed_implementation_3fff: int
    signed_documented_4000: int
    unsigned_fpga_3fff: int
    arithmetic_disagreement: bool
    clamp_disagreement: bool
    pitch_modulation_enabled: bool


def _signed16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def _validate(vx_pitch: int, previous_pre_lr_sample: int | None) -> None:
    if not 0 <= vx_pitch <= 0xFFFF:
        raise ValueError("VxPitch must fit in 16 bits")
    if previous_pre_lr_sample is not None and not -0x8000 <= previous_pre_lr_sample <= 0x7FFF:
        raise ValueError("previous pre-L/R sample must fit signed 16 bits")


def spu_raw_pitch_step(vx_pitch: int, previous_pre_lr_sample: int | None = None) -> int:
    """Return the signed-carrier raw step used by three current evidence sources.

    ``previous_pre_lr_sample`` is the immediately preceding physical voice's
    mono output after sample/noise selection and ADSR, before left/right voice
    volume sweeps. Passing ``None`` means PMON is not active for this voice.

    This helper intentionally names one candidate, not universal hardware truth.
    Use :func:`spu_raw_pmon_candidates` when high-bit VxPitch behavior matters.
    """

    _validate(vx_pitch, previous_pre_lr_sample)
    if previous_pre_lr_sample is None:
        return vx_pitch

    factor = previous_pre_lr_sample + 0x8000
    return ((_signed16(vx_pitch) * factor) >> 15) & 0xFFFF


def spu_raw_pmon_candidates(vx_pitch: int, previous_pre_lr_sample: int) -> SpuRawPmonCandidates:
    """Expose signed-carrier and MiSTer unsigned-carrier PMON candidates."""

    _validate(vx_pitch, previous_pre_lr_sample)
    factor = previous_pre_lr_sample + 0x8000
    signed_candidate = ((_signed16(vx_pitch) * factor) >> 15) & 0xFFFF
    unsigned_candidate = ((vx_pitch * factor) >> 15) & 0xFFFF
    return SpuRawPmonCandidates(
        signed_carrier=signed_candidate,
        unsigned_carrier_fpga=unsigned_candidate,
        disagree=signed_candidate != unsigned_candidate,
    )


def spu_pitch_step_candidates(
    vx_pitch: int,
    previous_pre_lr_sample: int | None = None,
) -> SpuPitchStepCandidates:
    """Expose current PMON arithmetic and maximum-step hypotheses together."""

    _validate(vx_pitch, previous_pre_lr_sample)
    if previous_pre_lr_sample is None:
        signed_raw = unsigned_raw = vx_pitch
    else:
        raw = spu_raw_pmon_candidates(vx_pitch, previous_pre_lr_sample)
        signed_raw = raw.signed_carrier
        unsigned_raw = raw.unsigned_carrier_fpga

    return SpuPitchStepCandidates(
        raw_signed_carrier=signed_raw,
        raw_unsigned_carrier_fpga=unsigned_raw,
        signed_implementation_3fff=min(signed_raw, SPU_PITCH_IMPLEMENTATION_MAX),
        signed_documented_4000=(
            signed_raw if signed_raw <= SPU_PITCH_IMPLEMENTATION_MAX else SPU_PITCH_DOCUMENTED_MAX
        ),
        unsigned_fpga_3fff=min(unsigned_raw, SPU_PITCH_IMPLEMENTATION_MAX),
        arithmetic_disagreement=signed_raw != unsigned_raw,
        clamp_disagreement=signed_raw > SPU_PITCH_IMPLEMENTATION_MAX,
        pitch_modulation_enabled=previous_pre_lr_sample is not None,
    )
