#!/usr/bin/env python3
"""Exact finite behavioral-equivalence control for the NES two-pulse DAC.

Compares two execution architectures:

  joint: historical two-pulse nonlinear DAC, P(a+b)
  split: independently rendered pulse stems summed later, P(a)+P(b)

The action alphabet is every labeled assignment A:=0..15 or B:=0..15.
Partitions begin from exact current output and refine by successor block
signatures, matching finite Moore-machine / bisimulation refinement.

The pulse-DAC equation is the current ares Famicom APU implementation at
b80f67d38312648d197762121c3a27b02c0887db.
"""

from __future__ import annotations

import json
import math
from collections import Counter
from fractions import Fraction

ARCHITECTURES = ("joint", "split")
AMPLITUDES = range(16)
ACTIONS = tuple((coord, value) for coord in ("a", "b") for value in AMPLITUDES)
STATES = tuple((arch, a, b) for arch in ARCHITECTURES for a in AMPLITUDES for b in AMPLITUDES)
INDEX = {state: i for i, state in enumerate(STATES)}


def pulse_dac(level: int) -> Fraction:
    """ares NES pulse DAC formula represented exactly as a rational."""
    if level == 0:
        return Fraction(0, 1)
    return Fraction(32768 * 9588 * level, 100 * (8128 + 100 * level))


def observe(state: tuple[str, int, int]) -> Fraction:
    arch, a, b = state
    if arch == "joint":
        return pulse_dac(a + b)
    return pulse_dac(a) + pulse_dac(b)


def transition(state: tuple[str, int, int], action: tuple[str, int]) -> tuple[str, int, int]:
    arch, a, b = state
    coord, value = action
    if coord == "a":
        return (arch, value, b)
    return (arch, a, value)


def partition(signatures):
    ids = {}
    classes = [0] * len(signatures)
    blocks = []
    for i, signature in enumerate(signatures):
        block = ids.get(signature)
        if block is None:
            block = len(blocks)
            ids[signature] = block
            blocks.append([])
        classes[i] = block
        blocks[block].append(i)
    return blocks, classes


def refine(classes):
    signatures = []
    for state in STATES:
        here = classes[INDEX[state]]
        future = tuple(classes[INDEX[transition(state, action)]] for action in ACTIONS)
        signatures.append((here, future))
    return partition(signatures)


def entropy(blocks):
    total = len(STATES)
    value = 0.0
    for block in blocks:
        p = len(block) / total
        value -= p * math.log2(p)
    return value


def architecture_pair_horizon(a: int, b: int, partitions):
    joint = INDEX[("joint", a, b)]
    split = INDEX[("split", a, b)]
    for horizon, (_, classes) in enumerate(partitions):
        if classes[joint] != classes[split]:
            return horizon
    return None


def run_control():
    blocks0, classes0 = partition([observe(state) for state in STATES])
    partitions = [(blocks0, classes0)]
    for _ in range(8):
        blocks, classes = refine(partitions[-1][1])
        partitions.append((blocks, classes))
        if len(blocks) == len(partitions[-2][0]):
            break

    class_counts = [len(blocks) for blocks, _ in partitions]
    expected = [151, 511, 512, 512]
    assert class_counts[:4] == expected, class_counts

    horizons = Counter(
        architecture_pair_horizon(a, b, partitions)
        for a in AMPLITUDES
        for b in AMPLITUDES
    )
    assert horizons == Counter({0: 225, 1: 30, 2: 1}), horizons

    h1_merged = [
        [STATES[i] for i in block]
        for block in partitions[1][0]
        if len(block) > 1
    ]
    assert h1_merged == [[("joint", 0, 0), ("split", 0, 0)]], h1_merged

    joint_11 = observe(("joint", 1, 1))
    split_11 = observe(("split", 1, 1))
    assert joint_11 != split_11

    return {
        "state_count": len(STATES),
        "action_count": len(ACTIONS),
        "partition_refinement": [
            {
                "horizon": horizon,
                "classes": len(blocks),
                "partition_entropy_bits": entropy(blocks),
                "min_class_size": min(map(len, blocks)),
                "max_class_size": max(map(len, blocks)),
            }
            for horizon, (blocks, _) in enumerate(partitions[:4])
        ],
        "same_amplitude_architecture_distinguishing_horizon": {
            "H0_current_output": horizons[0],
            "H1_one_assignment": horizons[1],
            "H2_two_assignments": horizons[2],
        },
        "unique_H1_merged_pair": h1_merged[0],
        "two_step_witness": {
            "start": [["joint", 0, 0], ["split", 0, 0]],
            "actions": [["a", 1], ["b", 1]],
            "joint_output": {
                "numerator": joint_11.numerator,
                "denominator": joint_11.denominator,
                "float": float(joint_11),
            },
            "split_output": {
                "numerator": split_11.numerator,
                "denominator": split_11.denominator,
                "float": float(split_11),
            },
            "difference_float": float(split_11 - joint_11),
        },
        "claim_boundary": [
            "Finite exact control over two idealized architectures using the ares pulse-DAC equation.",
            "This establishes obligation-relative behavioral equivalence for the declared amplitude-assignment action language.",
            "It does not claim every NES emulator analog model is identical, nor that all game-audio identity stabilizes at horizon two.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
