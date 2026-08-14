#!/usr/bin/env python3
"""Finite routing-reachability controls for several SNES music-driver lineages.

This is deliberately not a generic SPC700 emulator. It protects bounded facts
established from pinned driver disassemblies about the ordinary music-routing
paths that eventually feed signed S-DSP VxVOLL/VxVOLR registers.

The receiving distinction is:

    device-capable != driver-reachable != work-observed
"""

from __future__ import annotations

import json


def gun_hazard_unsigned_route(level: int, pan: int) -> tuple[int, int]:
    """Conservative finite model of the ordinary Square Akao scalar-pan path."""
    if not 0 <= level <= 0x7F:
        raise ValueError("Gun Hazard route level must be in [0, 127]")
    if not 0 <= pan <= 0xFF:
        raise ValueError("Gun Hazard pan must be one byte")
    left = (pan * level) >> 8
    right = ((0xFF - pan) * level) >> 8
    return left, right


def wolfteam_ad_flags(argument: int) -> int:
    """Exact Star Ocean/Tales vcmd AD phase-flag result for the pinned family."""
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


def nspc_pan_semantics(raw: int) -> tuple[int, bool, bool]:
    """Pinned Nintendo N-SPC E1 semantics: pan low5 + two phase switches."""
    if not 0 <= raw <= 0xFF:
        raise ValueError("N-SPC E1 value must be one byte")
    return raw & 0x1F, bool(raw & 0x80), bool(raw & 0x40)


def apply_nspc_phase(left: int, right: int, raw: int) -> tuple[int, int]:
    if left < 0 or right < 0:
        raise ValueError("base magnitudes must be nonnegative")
    _, left_phase, right_phase = nspc_pan_semantics(raw)
    return (-left if left_phase else left, -right if right_phase else right)


def capcom_final_route_gain(pan_magnitude: int, level: int) -> int:
    """Exact final 8x8->high-byte gain stage in the Mega Man X pan path.

    The pinned driver constrains the interpolated pan-table magnitude to
    0..0x7f. Whatever earlier nonnegative level survives the volume chain is an
    8-bit multiplier. Therefore this final stage can never set the signed S-DSP
    sign bit.
    """
    if not 0 <= pan_magnitude <= 0x7F:
        raise ValueError("Capcom pan magnitude must be in [0, 127]")
    if not 0 <= level <= 0xFF:
        raise ValueError("Capcom level must be one byte")
    return (pan_magnitude * level) >> 8


def konami_final_route_gain(pan_magnitude: int, level: int) -> int:
    """Bounded final Axelay pan scaling after its nonnegative pan tables.

    The pinned physical path clamps the pre-pan level below 0x80 and uses
    0..0x7f pan-table magnitudes before normalizing by 0x7f.
    """
    if not 0 <= pan_magnitude <= 0x7F:
        raise ValueError("Konami pan magnitude must be in [0, 127]")
    if not 0 <= level <= 0x7F:
        raise ValueError("Konami pre-pan level must be in [0, 127]")
    return (pan_magnitude * level) // 0x7F if pan_magnitude and level else 0


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
    # Square Akao / Gun Hazard: exhaust the conservative ordinary pan domain.
    gh_outputs = {
        gun_hazard_unsigned_route(level, pan)
        for level in range(0x80)
        for pan in range(0x100)
    }
    assert gh_outputs
    assert all(left >= 0 and right >= 0 for left, right in gh_outputs)
    assert max(max(pair) for pair in gh_outputs) <= 0x7F

    # Wolf Team: authored vcmd AD reaches every sign quadrant.
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
    assert cycle_product(wolf_routes[0], wolf_routes[1]) == -1
    assert determinant(wolf_routes[0], wolf_routes[1]) != 0

    # Nintendo N-SPC: E1 itself carries pan plus two route-polarity bits.
    nspc_routes = {
        raw: apply_nspc_phase(*representative, raw)
        for raw in (0x0A, 0x4A, 0x8A, 0xCA)
    }
    assert set(nspc_routes.values()) == {
        (40, 24),
        (40, -24),
        (-40, 24),
        (-40, -24),
    }
    assert cycle_product(nspc_routes[0x0A], nspc_routes[0x4A]) == -1

    # Capcom / Mega Man X: exhaust the final physical pan multiplication bound.
    capcom_outputs = {
        capcom_final_route_gain(magnitude, level)
        for magnitude in range(0x80)
        for level in range(0x100)
    }
    assert min(capcom_outputs) == 0
    assert max(capcom_outputs) <= 0x7E

    # Konami / Axelay: exhaust its final bounded 0x7f-normalized pan stage.
    konami_outputs = {
        konami_final_route_gain(magnitude, level)
        for magnitude in range(0x80)
        for level in range(0x80)
    }
    assert min(konami_outputs) == 0
    assert max(konami_outputs) == 0x7F

    return {
        "square_akao_gun_hazard": {
            "ordinary_pan_states_checked": 0x80 * 0x100,
            "distinct_route_pairs": len(gh_outputs),
            "signed_route_reachable": False,
            "scope": "ordinary Akao music scalar-pan path only",
        },
        "wolf_team": {
            "vcmd_ad_routes": {str(k): v for k, v in wolf_routes.items()},
            "all_four_sign_quadrants_reachable": True,
            "unbalanced_two_voice_cycle_reachable": True,
            "scope": "Star Ocean/Tales of Phantasia driver-family vcmd AD",
        },
        "nintendo_nspc": {
            "representative_e1_routes": {hex(k): v for k, v in nspc_routes.items()},
            "all_four_sign_quadrants_reachable": True,
            "unbalanced_two_voice_cycle_reachable": True,
            "scope": "pinned A Link to the Past E1 pan semantics",
        },
        "capcom_megaman_x": {
            "final_gain_states_checked": 0x80 * 0x100,
            "maximum_physical_gain": max(capcom_outputs),
            "signed_route_reachable_through_ordinary_pan": False,
            "scope": "ordinary Mega Man X pan path final physical gain stage",
        },
        "konami_axelay": {
            "final_gain_states_checked": 0x80 * 0x80,
            "maximum_physical_gain": max(konami_outputs),
            "signed_route_reachable_through_ordinary_pan": False,
            "scope": "ordinary Axelay pan-table physical gain path",
        },
        "driver_partition": {
            "authored_signed_route_positive_controls": [
                "Nintendo N-SPC / A Link to the Past",
                "Wolf Team / Star Ocean + Tales of Phantasia",
            ],
            "ordinary_pan_nonnegative_controls": [
                "Square Akao / Front Mission: Gun Hazard",
                "Capcom / Mega Man X",
                "Konami / Axelay",
            ],
        },
        "interpretation": [
            "The same S-DSP signed-volume hardware supports materially different driver-reachable routing languages.",
            "Nintendo and Wolf Team expose route polarity in authored sequence state; the tested Square, Capcom, and Konami ordinary pan paths do not.",
            "Driver identity therefore constrains the legal acoustic state space before work-specific observations are considered.",
        ],
        "claim_boundary": [
            "This finite control protects bounded route-path facts from pinned disassemblies; it is not a complete emulator for any driver.",
            "Negative controls apply to the identified ordinary music pan paths and do not exclude SFX, debug, CPU-direct, or other DSP-write mechanisms.",
            "The Star Ocean UI surround-to-driver call chain remains incompletely established.",
            "Signed routing topology is not itself a perceptual width metric.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())