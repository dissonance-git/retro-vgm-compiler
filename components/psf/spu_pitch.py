"""PlayStation SPU pitch-step analysis helpers.

This module preserves an observed disagreement at the maximum pitch-step clamp
rather than selecting one implementation as truth.  The common PMON transform
is independently visible in DuckStation, Beetle/Mednafen, and PSX-SPX; the
post-transform clamp differs by one LSB between those sources.

These helpers describe a physical SPU coordinate.  They do not map AKAO notes,
logical tracks, tuning fields, or musical pitch spelling to hardware voices.
"""

from __future__ import annotations

from dataclasses import dataclass


PLAYSTATION_SPU_VOICE_COUNT = 24
SPU_PITCH_UNITY = 0x1000
SPU_PITCH_EMULATOR_MAX = 0x3FFF
SPU_PITCH_DOCUMENTED_MAX = 0x4000


@dataclass(frozen=True)
class SpuPitchStepCandidates:
    raw_step: int
    emulator_3fff: int
    documented_4000: int
    pitch_modulation_enabled: bool


def _signed16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def spu_raw_pitch_step(vx_pitch: int, previous_pre_lr_sample: int | None = None) -> int:
    """Return the 16-bit step before the disputed maximum-step clamp.

    ``previous_pre_lr_sample`` is the immediately preceding physical voice's
    mono output after sample/noise selection and ADSR, before left/right voice
    volume sweeps.  Passing ``None`` means PMON is not active for this voice.

    The high-bit behavior intentionally sign-extends VxPitch only for the PMON
    multiply and wraps the product result back to 16 bits, matching the
    hardware-glitch form documented by PSX-SPX and implemented equivalently by
    the two pinned emulator lineages.
    """

    if not 0 <= vx_pitch <= 0xFFFF:
        raise ValueError("VxPitch must fit in 16 bits")

    if previous_pre_lr_sample is None:
        return vx_pitch
    if not -0x8000 <= previous_pre_lr_sample <= 0x7FFF:
        raise ValueError("previous pre-L/R sample must fit signed 16 bits")

    factor = previous_pre_lr_sample + 0x8000
    return ((_signed16(vx_pitch) * factor) >> 15) & 0xFFFF


def spu_pitch_step_candidates(
    vx_pitch: int,
    previous_pre_lr_sample: int | None = None,
) -> SpuPitchStepCandidates:
    """Expose both currently supported maximum-step hypotheses.

    DuckStation and Beetle/Mednafen clamp values above 0x3FFF to 0x3FFF.
    PSX-SPX documents values above 0x3FFF as becoming 0x4000.  Until a stronger
    hardware observation settles the one-LSB boundary, callers must retain both
    candidates rather than laundering the disagreement into one exact value.
    """

    raw = spu_raw_pitch_step(vx_pitch, previous_pre_lr_sample)
    return SpuPitchStepCandidates(
        raw_step=raw,
        emulator_3fff=min(raw, SPU_PITCH_EMULATOR_MAX),
        documented_4000=raw if raw <= SPU_PITCH_EMULATOR_MAX else SPU_PITCH_DOCUMENTED_MAX,
        pitch_modulation_enabled=previous_pre_lr_sample is not None,
    )
