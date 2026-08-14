#!/usr/bin/env python3
"""Exact signed-routing phase control for two sources and two outputs.

For a fully connected 2x2 routing support, signs live on the four edges.
The invariant product around the K2,2 cycle is preserved by independent
source/output polarity gauges. For equal-magnitude routes it exactly controls
matrix rank; for arbitrary strictly positive route magnitudes, negative cycle
product guarantees invertibility.
"""

from __future__ import annotations

import itertools
import json

SIGNS = (-1, 1)


def determinant(signs, magnitudes=(1, 1, 1, 1)):
    s_al, s_ar, s_bl, s_br = signs
    g_al, g_ar, g_bl, g_br = magnitudes
    return s_al * g_al * s_br * g_br - s_ar * g_ar * s_bl * g_bl


def cycle_product(signs):
    result = 1
    for sign in signs:
        result *= sign
    return result


def gauge(signs, source_gauge, output_gauge):
    s_al, s_ar, s_bl, s_br = signs
    a, b = source_gauge
    left, right = output_gauge
    return (
        a * left * s_al,
        a * right * s_ar,
        b * left * s_bl,
        b * right * s_br,
    )


def gauge_orbit(signs):
    return {
        gauge(signs, source_gauge, output_gauge)
        for source_gauge in itertools.product(SIGNS, repeat=2)
        for output_gauge in itertools.product(SIGNS, repeat=2)
    }


def run_control():
    matrices = list(itertools.product(SIGNS, repeat=4))
    equal = []
    for signs in matrices:
        det = determinant(signs)
        product = cycle_product(signs)
        rank = 1 if det == 0 else 2
        equal.append(
            {
                "signs": signs,
                "cycle_product": product,
                "determinant": det,
                "rank": rank,
            }
        )

    assert sum(x["cycle_product"] == 1 and x["rank"] == 1 for x in equal) == 8
    assert sum(x["cycle_product"] == -1 and x["rank"] == 2 for x in equal) == 8
    assert all((x["cycle_product"] == 1) == (x["rank"] == 1) for x in equal)

    unseen = set(matrices)
    orbits = []
    while unseen:
        seed = next(iter(unseen))
        orbit = gauge_orbit(seed)
        orbits.append(orbit)
        unseen -= orbit

    assert len(orbits) == 2
    assert sorted(len(orbit) for orbit in orbits) == [8, 8]
    assert all(len({cycle_product(x) for x in orbit}) == 1 for orbit in orbits)
    assert {cycle_product(next(iter(orbit))) for orbit in orbits} == {-1, 1}

    # Exhaustive positive-magnitude pressure test. If cycle product is negative,
    # the two determinant terms have opposite signs, so their magnitudes add and
    # the determinant cannot vanish while all four route magnitudes are positive.
    magnitude_values = range(1, 5)
    checked = 0
    balanced_singular = 0
    balanced_invertible = 0
    unbalanced_singular = 0
    for magnitudes in itertools.product(magnitude_values, repeat=4):
        for signs in matrices:
            det = determinant(signs, magnitudes)
            product = cycle_product(signs)
            checked += 1
            if product == -1:
                unbalanced_singular += int(det == 0)
            elif det == 0:
                balanced_singular += 1
            else:
                balanced_invertible += 1

    assert unbalanced_singular == 0
    assert balanced_singular > 0
    assert balanced_invertible > 0

    return {
        "equal_magnitude_sign_matrices": 16,
        "equal_magnitude": {
            "balanced_cycle_rank1": 8,
            "unbalanced_cycle_rank2": 8,
        },
        "source_output_polarity_gauge_orbits": {
            "count": len(orbits),
            "sizes": sorted(len(orbit) for orbit in orbits),
            "complete_invariant": "cycle_product",
        },
        "positive_magnitude_pressure_test": {
            "gain_values": [1, 2, 3, 4],
            "cases_checked": checked,
            "unbalanced_cycle_singular_cases": unbalanced_singular,
            "balanced_cycle_singular_cases": balanced_singular,
            "balanced_cycle_invertible_cases": balanced_invertible,
        },
        "interpretation": [
            "Unsigned routing support does not determine linear source separability.",
            "For a fully connected 2x2 signed routing matrix, negative cycle product guarantees a nonzero determinant for strictly positive route magnitudes.",
            "With equal magnitudes, cycle product alone exactly distinguishes rank-1 from rank-2 routing.",
            "The cycle product is invariant under independent whole-source and whole-output polarity flips.",
        ],
        "claim_boundary": [
            "Exact elementary 2x2 signed-matrix/signed-graph control, not a new graph theorem.",
            "Linear invertibility is not the same as perceptual source separation in a full nonlinear/noisy device.",
            "Device use requires exact time-bearing gains, phase flags, and provenance.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
