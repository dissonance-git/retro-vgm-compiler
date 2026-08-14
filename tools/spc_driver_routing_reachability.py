#!/usr/bin/env python3
"""Finite routing-reachability control for two SNES music-driver lineages.

This does not emulate either driver. It encodes only the bounded routing facts
established from pinned disassemblies:

* Front Mission: Gun Hazard / Square Akao v4: ordinary music panning reaches
  S-DSP voice L/R through nonnegative byte-scale magnitude products.
* Star Ocean / Wolf Team: sequence command AD controls two per-voice sign bits
  that the physical VOL(L)/VOL(R) writer consumes as left/right negations.

The purpose is to protect the distinction:

    hardware-capable != driver-reachable != work-observed
"""

from __future__ import annotations

import json


def gun_hazard_unsigned_route(level: int, pan: int) -> tuple[int, int]:
    """Conservative finite model of the ordinary Akao scalar-pan magnitude path.

    The exact driver has additional volume/tremolo/global scaling before this
    stage, but the resulting level is bounded to the nonnegative signed-byte
    range. Using complementary 8-bit pan coefficients is enough to prove that
    this path cannot set the S-DSP sign bit.
    """
    if not 0 <= level <= 0x7F:
        raise ValueError("Gun Hazard route level must be in [0, 127]")
    if not 0 <= pan <= 0xFF:
        raise ValueError("Gun Hazard pan must be one byte")
    left = (pan * level) >> 8
    right = ((0xFF - pan) * level) >> 8
    return left, right


def wolfteam_ad_flags(argument: int) -> int:
    """Exact vcmd AD phase-flag result for the observed Wolf Team build family.

    Bit 5 is consumed as right-route negation and bit 6 as left-route negation
    by the Star Ocean/Tales of Phantasia physical voice-volume path.
    """
    if not 0 <= argument <= 0xFF:
        raise ValueError("Wolf Team AD argument must be one byte")
    flags = 0
    if argument != 0:
        if argument != 1:
            flags |= 0x40
        if argument != 2:
            flags |= 0x20
    return flags


def apply_wolfteam_phase(left: int, right: int, flags: int) -> tuple[int, int]:
    if left < 0 or right < 0:
        raise ValueError("base magnitudes must be nonnegative")
    if flags & 0x40:
        left = -left
    if flags & 0x20:
        right = -right
    return left, right


def cycle_product(a: tuple[int, int], b: tuple[int, int]) -> int | None:
    gains = (*a, *b)
    if any(value == 0 for value in gains):
        return None
    product = 1
    for value in gains:
        product *= -1 if value < 0 else 1
    return product


def determinant(a: tuple[int, int], b: tuple[int, int]) -> int:
    return a[0] * b[1] - a[1] * b[0]


def run_control() -> dict:
    # Exhaust every ordinary bounded Gun Hazard scalar-pan coordinate.
    gh_outputs = {
        gun_hazard_unsigned_route(level, pan)
        for level in range(0x80)
        for pan in range(0x100)
    }
    assert gh_outputs
    assert all(left >= 0 and right >= 0 for left, right in gh_outputs)
    assert max(max(pair) for pair in gh_outputs) <= 0x7F

    # The four phase quadrants are all reachable through Star Ocean vcmd AD.
    representative = (40, 24)
    wolf_routes = {
        argument: apply_wolfteam_phase(
            *representative,
            wolfteam_ad_flags(argument),
        )
        for argument in range(4)
    }
    assert wolf_routes == {
        0: (40, 24),
        1: (40, -24),
        2: (-40, 24),
        3: (-40, -24),
    }

    # Two simultaneous positive-base voices can therefore realize the
    # unbalanced K2,2 cycle that was only hardware-theoretical in Gun Hazard.
    voice_a = wolf_routes[0]
    voice_b = wolf_routes[1]
    assert cycle_product(voice_a, voice_b) == -1
    assert determinant(voice_a, voice_b) != 0

    return {
        "gun_hazard": {
            "ordinary_pan_states_checked": 0x80 * 0x100,
            "distinct_route_pairs": len(gh_outputs),
            "negative_route_reachable": False,
            "scope": "ordinary Akao music scalar-pan path only",
        },
        "wolf_team": {
            "vcmd_ad_arguments": {
                str(argument): {
                    "flags": wolfteam_ad_flags(argument),
                    "route": wolf_routes[argument],
                }
                for argument in range(4)
            },
            "all_four_sign_quadrants_reachable": True,
            "unbalanced_two_voice_cycle_reachable": True,
            "scope": "Star Ocean/Tales of Phantasia driver-family phase flags",
        },
        "interpretation": [
            "S-DSP signed-gain hardware capability does not imply that every music driver exposes that capability.",
            "Gun Hazard ordinary music panning is confined to a nonnegative routing manifold.",
            "The Wolf Team sequence vocabulary can move a voice between all four signed stereo quadrants.",
            "Driver vocabulary therefore defines a work-relevant reachable subset of device state space.",
        ],
        "claim_boundary": [
            "This finite control protects routing reachability proven from pinned disassemblies; it is not a complete driver emulator.",
            "The Star Ocean in-game output option called surround is community-documented, but its exact UI-to-vcmd-AD causal path is not established here.",
            "Signed routing topology is not itself a perceptual width metric.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
