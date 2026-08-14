#!/usr/bin/env python3
"""Exact finite quotient for the pinned A Link to the Past N-SPC E1 pan byte.

Pinned driver evidence shows this build stores the full E1 byte, projects low
five bits to scalar pan, and later consumes bits 7/6 as left/right polarity
controls. Bit 5 has no downstream read in the pinned build.

An independent modern N-SPC-compatible authoring implementation, AddmusicK,
accepts only pan positions 0..20 plus two explicit surround booleans. This lets
one finite control separate raw byte space, driver semantics, and legal modern
authoring vocabulary without pretending they are the same domain.
"""

from __future__ import annotations

import json

RAW_VALUES = range(0x100)
DEAD_BIT = 0x20
LEFT_PHASE_BIT = 0x80
RIGHT_PHASE_BIT = 0x40
PAN_MASK = 0x1F
AUTHORING_PAN_VALUES = range(21)


def semantics(raw: int) -> tuple[int, bool, bool]:
    if not 0 <= raw <= 0xFF:
        raise ValueError("N-SPC E1 value must be one byte")
    return (
        raw & PAN_MASK,
        bool(raw & LEFT_PHASE_BIT),
        bool(raw & RIGHT_PHASE_BIT),
    )


def equivalence_classes() -> dict[tuple[int, bool, bool], list[int]]:
    classes: dict[tuple[int, bool, bool], list[int]] = {}
    for raw in RAW_VALUES:
        classes.setdefault(semantics(raw), []).append(raw)
    return classes


def addmusick_authored_values() -> set[int]:
    return {
        pan | (left_phase << 7) | (right_phase << 6)
        for pan in AUTHORING_PAN_VALUES
        for left_phase in (0, 1)
        for right_phase in (0, 1)
    }


def overwrite(raw_state: int, next_e1: int) -> int:
    if not 0 <= raw_state <= 0xFF or not 0 <= next_e1 <= 0xFF:
        raise ValueError("N-SPC E1 state/action must be one byte")
    return next_e1


def run_control() -> dict:
    classes = equivalence_classes()
    assert len(classes) == 128
    assert all(len(members) == 2 for members in classes.values())

    # Every class is exactly the pair that differs only in unused bit 5.
    for members in classes.values():
        a, b = sorted(members)
        assert a ^ b == DEAD_BIT
        assert semantics(a) == semantics(b)

    # Every subsequent E1 assignment overwrites both representatives with the
    # same new raw state. No future E1 sequence can reveal dead bit 5.
    transition_checks = 0
    for members in classes.values():
        a, b = members
        for action in RAW_VALUES:
            assert semantics(overwrite(a, action)) == semantics(overwrite(b, action))
            transition_checks += 1

    authored = addmusick_authored_values()
    assert len(authored) == 21 * 4 == 84
    assert all((raw & DEAD_BIT) == 0 for raw in authored)
    authored_semantics = {semantics(raw) for raw in authored}
    assert len(authored_semantics) == 84

    driver_semantics = set(classes)
    outside_modern_authoring = driver_semantics - authored_semantics
    assert len(outside_modern_authoring) == (32 - 21) * 4 == 44
    assert all(pan >= 21 for pan, _, _ in outside_modern_authoring)

    return {
        "raw_state_count": 256,
        "driver_semantic_class_count": len(classes),
        "semantic_class_size": 2,
        "dead_bit_mask": DEAD_BIT,
        "live_coordinates": {
            "scalar_pan": "bits 0..4",
            "right_route_phase": "bit 6",
            "left_route_phase": "bit 7",
        },
        "continuation_family": "subsequent E1 pan-byte assignments overwrite the raw pan state",
        "transition_checks": transition_checks,
        "stable_driver_quotient": True,
        "modern_addmusick_authoring": {
            "legal_pan_positions": 21,
            "phase_combinations": 4,
            "raw_values_emitted": len(authored),
            "semantic_states": len(authored_semantics),
            "dead_bit_emitted": False,
        },
        "driver_semantics_outside_modern_authoring": {
            "semantic_state_count": len(outside_modern_authoring),
            "pan_indices": list(range(21, 32)),
            "note": "The pinned driver masks five pan bits without the 0..20 validation used by AddmusicK; executable decoding and authoring-language legality are therefore distinct contracts.",
        },
        "state_ladder": [
            "raw E1 byte space: 256",
            "pinned driver behavioral pan/phase quotient: 128",
            "modern AddmusicK legal authored pan/phase states: 84",
            "work-observed states: unresolved for this control",
        ],
        "representative_pairs": [
            [members[0], members[1]]
            for _, members in sorted(classes.items())[:8]
        ],
        "interpretation": [
            "The pinned A Link to the Past E1 pan byte contains one source bit that is behaviorally dead for the declared pan/mixer obligation.",
            "Raw bytes that differ only by bit 5 are safely quotiented in a derived semantic view; the source byte remains preserved in provenance.",
            "The driver can decode more pan indices than the independent modern authoring language accepts, so runtime-decodable state and authoring-legal state must remain distinct.",
            "This complements dormant-information controls: some hidden distinctions later become load-bearing, while others are proven unreachable from every declared consequence and may be compressed in a derived view.",
        ],
        "claim_boundary": [
            "The dead-bit claim is for the pinned A Link to the Past N-SPC build and its audited $0351+x use sites, not every N-SPC variant.",
            "AddmusicK is an independent modern N-SPC-compatible authoring observatory, not evidence for the exact restrictions of Nintendo's historical internal tool.",
            "Driver-decodable pan indices 21..31 are not claimed to be intentionally authored or musically meaningful; they are merely executable under the pinned code path.",
            "The quotient is a derived semantic view and does not license destructive rewriting of source bytes.",
            "The control models E1 pan state and downstream mixer semantics, not the full game audio driver state machine.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())