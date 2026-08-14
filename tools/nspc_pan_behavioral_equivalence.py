#!/usr/bin/env python3
"""Exact finite quotient for the pinned A Link to the Past N-SPC E1 pan byte.

Pinned driver evidence shows this build stores the full E1 byte, projects low
five bits to scalar pan, and later consumes bits 7/6 as left/right polarity
controls. Bit 5 has no downstream read in the pinned build.

This finite control therefore asks which of the 256 raw command bytes remain
behaviorally distinct under the declared pan/mixer obligation.
"""

from __future__ import annotations

import json

RAW_VALUES = range(0x100)
DEAD_BIT = 0x20
LEFT_PHASE_BIT = 0x80
RIGHT_PHASE_BIT = 0x40
PAN_MASK = 0x1F


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


def overwrite(raw_state: int, next_e1: int) -> int:
    if not 0 <= raw_state <= 0xFF or not 0 <= next_e1 <= 0xFF:
        raise ValueError("N-SPC E1 state/action must be one byte")
    return next_e1


def run_control() -> dict:
    classes = equivalence_classes()
    assert len(classes) == 128
    assert all(len(members) == 2 for members in classes.values())

    # Every class is exactly the pair that differs only in the unused bit 5.
    for members in classes.values():
        a, b = sorted(members)
        assert a ^ b == DEAD_BIT
        assert semantics(a) == semantics(b)

    # Under the declared E1-assignment continuation family, every future E1
    # overwrites both representatives with the same raw state. Thus no future
    # E1 sequence can reveal which dead-bit representative preceded it.
    transition_checks = 0
    for members in classes.values():
        a, b = members
        for action in RAW_VALUES:
            assert semantics(overwrite(a, action)) == semantics(overwrite(b, action))
            transition_checks += 1

    return {
        "raw_state_count": 256,
        "semantic_class_count": len(classes),
        "class_size": 2,
        "dead_bit_mask": DEAD_BIT,
        "live_coordinates": {
            "scalar_pan": "bits 0..4",
            "right_route_phase": "bit 6",
            "left_route_phase": "bit 7",
        },
        "transition_family": "subsequent E1 pan-byte assignments overwrite the raw pan state",
        "transition_checks": transition_checks,
        "stable_quotient": True,
        "representative_pairs": [
            [members[0], members[1]]
            for _, members in sorted(classes.items())[:8]
        ],
        "interpretation": [
            "The pinned A Link to the Past E1 pan byte contains one source bit that is behaviorally dead for the declared pan/mixer obligation.",
            "Raw bytes that differ only by bit 5 are safely quotiented in a derived semantic view; the source byte remains preserved in provenance.",
            "This is the complementary case to dormant distinctions that become observable at longer transformation horizons: some distinctions remain unobservable under the full declared continuation family.",
        ],
        "claim_boundary": [
            "The dead-bit claim is for the pinned A Link to the Past N-SPC build and its audited $0351+x use sites, not every N-SPC variant.",
            "The quotient is a derived semantic view and does not license destructive rewriting of source bytes.",
            "The control models the E1 pan state and its downstream mixer semantics, not the full game audio driver state machine.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
