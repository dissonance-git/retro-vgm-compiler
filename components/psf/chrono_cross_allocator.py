"""Bounded oracle for reconstructed Chrono Cross AKAO/SPU allocation policy.

Evidence source:
    jdperos/chrono-cross-decomp
    7dcadfc36421c9b26466f7fdbdbaa1a1102219c6

This module is intentionally game/driver specific.  It exists so future PSF1
runtime traces can be checked against the reconstructed Chrono Cross policy.
It is not a generic AKAO allocator and must not be promoted to the common model
without independent evidence from materially different driver lineages.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence


CHRONO_LOGICAL_CHANNEL_COUNT = 32
PLAYSTATION_SPU_VOICE_COUNT = 24
VOICE_INVALID_INDEX = PLAYSTATION_SPU_VOICE_COUNT
INITIAL_SFX_VOICE_START = 12
INITIAL_SFX_VOICE_COUNT = 12
PROTECTED_ENVELOPE_SENTINEL = 0x7FFF


@dataclass(frozen=True)
class AllocationDecision:
    voice_index: int
    stole_voice: bool
    exhausted: bool


def initial_sfx_voice_indices() -> tuple[int, ...]:
    """Return the physical voices preassigned to Chrono's 12 SFX channels."""

    return tuple(range(INITIAL_SFX_VOICE_START, PLAYSTATION_SPU_VOICE_COUNT))


def apply_prevent_rekey_on_resume(active_note_mask: int, prevent_rekey_mask: int) -> int:
    """Model the FE13 state transform performed when music is pushed/suspended."""

    return active_note_mask & ~prevent_rekey_mask & 0xFFFFFFFF


def channel_mask_to_voice_mask(assignments: Sequence[int], channel_mask: int) -> int:
    """Project logical-channel membership through current physical assignments.

    Invalid/unassigned channels contribute no physical bit.  Several logical
    channels may theoretically name the same physical voice in a malformed or
    transitional snapshot; the result is therefore a mask, not a bijection.
    """

    if len(assignments) != CHRONO_LOGICAL_CHANNEL_COUNT:
        raise ValueError("Chrono allocation snapshots require exactly 32 logical channels")

    voice_mask = 0
    for channel_index, voice_index in enumerate(assignments):
        if channel_mask & (1 << channel_index):
            if 0 <= voice_index < PLAYSTATION_SPU_VOICE_COUNT:
                voice_mask |= 1 << voice_index
            elif voice_index != VOICE_INVALID_INDEX:
                raise ValueError("assigned voice index is outside the reconstructed SPU domain")
    return voice_mask


def effective_envelope_cache(
    measured_envx: Sequence[int],
    *,
    fixed_music_voice_mask: int = 0,
    active_sfx_voice_mask: int = 0,
    suspended_sfx_voice_mask: int = 0,
    cutscene_voice_mask: int = 0,
) -> tuple[int, ...]:
    """Apply Chrono's pre-allocation protection policy to cached SPU ENVX.

    The reconstructed driver writes the sentinel 0x7FFF instead of the live
    envelope for protected voices.  That sentinel makes a voice neither free
    (free means ENVX==0) nor eligible for the quietest-voice steal path (which
    accepts only ENVX values strictly below 0x7FFF).

    ``fixed_music_voice_mask`` corresponds to the driver's directly protected
    preallocated music-voice bits, not every dynamically assigned logical note.
    """

    if len(measured_envx) != PLAYSTATION_SPU_VOICE_COUNT:
        raise ValueError("PlayStation SPU envelope snapshots require exactly 24 voices")
    for value in measured_envx:
        if not 0 <= value <= 0x7FFF:
            raise ValueError("ENVX values must fit the reconstructed 15-bit positive domain")

    protected = (
        fixed_music_voice_mask
        | active_sfx_voice_mask
        | suspended_sfx_voice_mask
        | cutscene_voice_mask
    ) & 0xFFFFFF

    return tuple(
        PROTECTED_ENVELOPE_SENTINEL if protected & (1 << index) else measured_envx[index]
        for index in range(PLAYSTATION_SPU_VOICE_COUNT)
    )


def _scan_start(allocator_floor: int, force_full_scan: bool) -> int:
    if not 0 <= allocator_floor <= PLAYSTATION_SPU_VOICE_COUNT:
        raise ValueError("allocator floor must stay within the 24-voice SPU domain")
    return 0 if force_full_scan else allocator_floor


def find_free_voice(
    envx_cache: Sequence[int],
    *,
    allocator_floor: int = 0,
    force_full_scan: bool = False,
) -> int:
    """Return the first eligible silent voice, or ``VOICE_INVALID_INDEX``."""

    if len(envx_cache) != PLAYSTATION_SPU_VOICE_COUNT:
        raise ValueError("PlayStation SPU envelope snapshots require exactly 24 voices")
    start = _scan_start(allocator_floor, force_full_scan)
    for index in range(start, PLAYSTATION_SPU_VOICE_COUNT):
        if envx_cache[index] == 0:
            return index
    return VOICE_INVALID_INDEX


def steal_quietest_voice(
    envx_cache: Sequence[int],
    *,
    allocator_floor: int = 0,
    force_full_scan: bool = False,
) -> int:
    """Return the first strict minimum below 0x7FFF, or invalid if none exists."""

    if len(envx_cache) != PLAYSTATION_SPU_VOICE_COUNT:
        raise ValueError("PlayStation SPU envelope snapshots require exactly 24 voices")
    start = _scan_start(allocator_floor, force_full_scan)

    quietest = PROTECTED_ENVELOPE_SENTINEL
    selected = VOICE_INVALID_INDEX
    for index in range(start, PLAYSTATION_SPU_VOICE_COUNT):
        value = envx_cache[index]
        if value < quietest:
            quietest = value
            selected = index
    return selected


def allocate_dynamic_voice(
    envx_cache: Sequence[int],
    *,
    allocator_floor: int = 0,
    force_full_scan: bool = False,
) -> AllocationDecision:
    """Model Chrono's free-then-quietest allocation decision for one key-on."""

    free = find_free_voice(
        envx_cache,
        allocator_floor=allocator_floor,
        force_full_scan=force_full_scan,
    )
    if free != VOICE_INVALID_INDEX:
        return AllocationDecision(free, stole_voice=False, exhausted=False)

    stolen = steal_quietest_voice(
        envx_cache,
        allocator_floor=allocator_floor,
        force_full_scan=force_full_scan,
    )
    if stolen == VOICE_INVALID_INDEX:
        return AllocationDecision(VOICE_INVALID_INDEX, stole_voice=False, exhausted=True)
    return AllocationDecision(stolen, stole_voice=True, exhausted=False)
