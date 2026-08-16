#!/usr/bin/env python3
"""Source-relative Sega Genesis SN76489/PSG surface semantics.

This module exists so Genesis analysis does not treat the PSG as either
"irrelevant" or "four more notes."  The chip exposes three pitched square-wave
tone channels and one noise channel, but musical role is a higher-level claim.

The surface contract is intentionally narrow:

* tone channels may contribute exact source-relative pitch coordinates;
* the noise channel never contributes a harmonic pitch class;
* FM/PSG pitch coincidence is evidence for possible doubling, not proof that two
  independent harmonic voices exist;
* in a YM2612+PSG arrangement, PSG tone is not automatically proposed as the
  bass foundation merely because it happens to be lowest at one instant;
* noise activity is percussion/texture evidence, not automatically a hi-hat.

Sonic 3 & Knuckles is a useful mixed-role control: its source contains pitched
PSG1/PSG2 material as well as PSG3 noise/percussion patterns.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import struct
from typing import Any


DEFAULT_PITCH_TOLERANCE_CENTS = 35.0
PSG_DIRECT_TONE_PITCH_CONFIDENCE = 1.0


def sn76489_clock_hz(raw: bytes) -> int:
    """Return the source-relative PSG master clock from a VGM header.

    VGM stores the SN76489 clock at 0x0C.  Bits 30/31 are chip/variant flags,
    not part of the frequency value.
    """

    if len(raw) < 0x10 or raw[:4] != b"Vgm ":
        raise ValueError("not a VGM stream")
    return struct.unpack_from("<I", raw, 0x0C)[0] & 0x3FFFFFFF


def sn76489_nominal_hz(tone_period: int, clock_hz: int) -> float | None:
    """Mirror the shared C++ Genesis nominal-pitch contract.

    The Sega PSG divides its master clock by 16 before the tone counter and a
    full square-wave cycle takes two programmed periods:

        nominal_hz = clock / (32 * period)

    A programmed period of zero is kept unresolved here, matching
    `genesis_nominal_pitch.h`; renderer-specific zero-period normalization is a
    separate synthesis behavior.
    """

    if clock_hz <= 0 or tone_period <= 0 or tone_period > 0x03FF:
        return None
    return float(clock_hz) / (32.0 * float(tone_period))


def _project_12tet(frequency_hz: float, tolerance_cents: float) -> dict[str, Any]:
    if not math.isfinite(frequency_hz) or frequency_hz <= 0.0:
        return {"resolved": False, "reason": "invalid_frequency"}
    if not math.isfinite(tolerance_cents) or tolerance_cents <= 0.0:
        raise ValueError("pitch tolerance must be finite and positive")

    exact_midi = 69.0 + 12.0 * math.log2(frequency_hz / 440.0)
    nearest_midi = int(round(exact_midi))
    cents = (exact_midi - nearest_midi) * 100.0
    if abs(cents) > tolerance_cents:
        return {
            "resolved": False,
            "reason": "outside_12tet_projection_tolerance",
            "performed_hz": frequency_hz,
            "cents_from_nearest": cents,
            "confidence": PSG_DIRECT_TONE_PITCH_CONFIDENCE,
        }
    return {
        "resolved": True,
        "performed_hz": frequency_hz,
        "midi_note": nearest_midi,
        "pitch_class": nearest_midi % 12,
        "cents_from_nearest": cents,
        "confidence": PSG_DIRECT_TONE_PITCH_CONFIDENCE,
    }


@dataclass(frozen=True)
class PsgWriteEffect:
    channel: int
    kind: str
    previous_value: int
    new_value: int


class SN76489SurfaceState:
    """Minimal latch/data state needed for musical surface analysis."""

    def __init__(self) -> None:
        # Keep the programmed state, not renderer-normalized period values.
        self.tone_periods = [0, 0, 0]
        self.attenuation = [15, 15, 15, 15]
        self.noise_control = 0
        self.latched_channel = 0
        self.latched_volume = False

    def write(self, data: int) -> PsgWriteEffect:
        data &= 0xFF
        if data & 0x80:
            self.latched_channel = (data >> 5) & 0x03
            self.latched_volume = bool(data & 0x10)
            channel = self.latched_channel
            if self.latched_volume:
                previous = self.attenuation[channel]
                self.attenuation[channel] = data & 0x0F
                return PsgWriteEffect(channel, "attenuation", previous, self.attenuation[channel])
            if channel < 3:
                previous = self.tone_periods[channel]
                self.tone_periods[channel] = (previous & 0x03F0) | (data & 0x0F)
                return PsgWriteEffect(channel, "tone_period", previous, self.tone_periods[channel])
            previous = self.noise_control
            self.noise_control = data & 0x07
            return PsgWriteEffect(channel, "noise_control", previous, self.noise_control)

        channel = self.latched_channel
        if self.latched_volume:
            previous = self.attenuation[channel]
            self.attenuation[channel] = data & 0x0F
            return PsgWriteEffect(channel, "attenuation", previous, self.attenuation[channel])
        if channel < 3:
            previous = self.tone_periods[channel]
            low = previous & 0x000F
            self.tone_periods[channel] = low | ((data & 0x3F) << 4)
            return PsgWriteEffect(channel, "tone_period", previous, self.tone_periods[channel])
        previous = self.noise_control
        self.noise_control = data & 0x07
        return PsgWriteEffect(channel, "noise_control", previous, self.noise_control)

    def tone_active(self, channel: int) -> bool:
        if not 0 <= channel < 3:
            raise IndexError(channel)
        return self.attenuation[channel] < 15

    def noise_active(self) -> bool:
        return self.attenuation[3] < 15

    def tone_pitch(
        self,
        channel: int,
        clock_hz: int,
        *,
        tolerance_cents: float = DEFAULT_PITCH_TOLERANCE_CENTS,
        ym2612_music_present: bool = True,
    ) -> dict[str, Any]:
        if not 0 <= channel < 3:
            raise IndexError(channel)
        if not self.tone_active(channel):
            return {"resolved": False, "reason": "psg_tone_muted"}

        period = self.tone_periods[channel]
        frequency = sn76489_nominal_hz(period, clock_hz)
        if frequency is None:
            return {"resolved": False, "reason": "psg_tone_period_unavailable"}

        result = _project_12tet(frequency, tolerance_cents)
        result.update({
            "device_family": "SN76489",
            "physical_channel": channel,
            "tone_period": period,
            # In mixed Genesis arrangements a low PSG pitch is not enough to
            # establish bass function. Harmonic-bass ownership must be earned
            # independently by the persistent-part/role layer.
            "surface_bass_eligible": not ym2612_music_present,
            "bass_role_prior": (
                "requires_strong_independent_evidence_in_ym2612_plus_psg_context"
                if ym2612_music_present
                else "no_mixed_fm_context_prior"
            ),
        })
        return result

    def noise_surface(self, *, ym2612_music_present: bool = True) -> dict[str, Any]:
        """Return role-safe noise-channel state.

        Noise may behave like a hi-hat, snare, kick-like transient, accent, or
        texture.  Channel identity alone establishes none of those names.
        """

        return {
            "active": self.noise_active(),
            "device_family": "SN76489",
            "physical_channel": 3,
            "noise_control": self.noise_control,
            "pitch_class_available": False,
            "candidate_role_family": "percussion_or_texture",
            "hi_hat_established": False,
            "bass_foundation_candidate": False if ym2612_music_present else None,
        }


def pitch_coincident(first: dict[str, Any], second: dict[str, Any], *, cents: float = 35.0) -> bool:
    """Return whether two resolved pitches are close enough to suggest doubling.

    Coincidence is only a *doubling candidate*.  Persistent-part correspondence,
    onset lag, duration, contour, and phrase context are still required to decide
    whether the sources are one musical line or independent voices.
    """

    if not first.get("resolved") or not second.get("resolved"):
        return False
    a = float(first["performed_hz"])
    b = float(second["performed_hz"])
    if a <= 0.0 or b <= 0.0:
        return False
    distance = abs(1200.0 * math.log2(a / b))
    return distance <= cents
