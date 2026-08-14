#!/usr/bin/env python3
"""Audit signed S-DSP voice routing in SPC snapshots.

This is a snapshot-level physical-routing audit. It does not infer authored pan,
perceived azimuth, or a persistent musical part. S-DSP VxVOLL/VxVOLR are signed
8-bit gains, so the audit preserves their signs and tests two-voice routing
matrices without flattening them to a scalar pan value.
"""

from __future__ import annotations

import argparse
import json
import pathlib
from dataclasses import dataclass

SPC_SIGNATURE = b"SNES-SPC700 Sound File Data"
DSP_OFFSET = 0x10100
DSP_SIZE = 0x80
VOICE_COUNT = 8
VOICE_STRIDE = 0x10


def signed8(value: int) -> int:
    if not 0 <= value <= 0xFF:
        raise ValueError("signed8 input must be one byte")
    return value - 0x100 if value & 0x80 else value


def sign(value: int) -> int:
    return -1 if value < 0 else (1 if value > 0 else 0)


@dataclass(frozen=True)
class VoiceRoute:
    voice: int
    left: int
    right: int

    @property
    def has_negative_gain(self) -> bool:
        return self.left < 0 or self.right < 0


@dataclass(frozen=True)
class PairRouting:
    voice_a: int
    voice_b: int
    determinant: int
    full_support: bool
    cycle_product: int | None

    @property
    def rank(self) -> int:
        if not self.full_support and self.determinant == 0:
            return 0
        return 1 if self.determinant == 0 else 2


def parse_routes(data: bytes) -> list[VoiceRoute]:
    if not data.startswith(SPC_SIGNATURE):
        raise ValueError("not an SNES-SPC700 snapshot")
    if len(data) < DSP_OFFSET + DSP_SIZE:
        raise ValueError("truncated SNES-SPC700 snapshot")

    dsp = data[DSP_OFFSET : DSP_OFFSET + DSP_SIZE]
    routes: list[VoiceRoute] = []
    for voice in range(VOICE_COUNT):
        base = voice * VOICE_STRIDE
        routes.append(
            VoiceRoute(
                voice=voice,
                left=signed8(dsp[base]),
                right=signed8(dsp[base + 1]),
            )
        )
    return routes


def analyze_pair(a: VoiceRoute, b: VoiceRoute) -> PairRouting:
    determinant = a.left * b.right - a.right * b.left
    gains = (a.left, a.right, b.left, b.right)
    full_support = all(value != 0 for value in gains)
    cycle_product = None
    if full_support:
        cycle_product = 1
        for value in gains:
            cycle_product *= sign(value)
    return PairRouting(
        voice_a=a.voice,
        voice_b=b.voice,
        determinant=determinant,
        full_support=full_support,
        cycle_product=cycle_product,
    )


def analyze_snapshot(path: pathlib.Path) -> dict:
    routes = parse_routes(path.read_bytes())
    pairs = [
        analyze_pair(routes[a], routes[b])
        for a in range(VOICE_COUNT)
        for b in range(a + 1, VOICE_COUNT)
    ]

    negative_routes = [route.voice for route in routes if route.has_negative_gain]
    full_pairs = [pair for pair in pairs if pair.full_support]
    return {
        "name": str(path),
        "voices": [
            {"voice": route.voice, "left": route.left, "right": route.right}
            for route in routes
        ],
        "negative_gain_voice_count": len(negative_routes),
        "negative_gain_voices": negative_routes,
        "pair_count": len(pairs),
        "full_support_pair_count": len(full_pairs),
        "balanced_cycle_pair_count": sum(pair.cycle_product == 1 for pair in full_pairs),
        "unbalanced_cycle_pair_count": sum(pair.cycle_product == -1 for pair in full_pairs),
        "rank2_pair_count": sum(pair.determinant != 0 for pair in pairs),
    }


def analyze_collection(root: pathlib.Path) -> dict:
    files = sorted(root.rglob("*.spc")) if root.is_dir() else [root]
    if not files:
        raise ValueError("no SPC snapshots found")

    reports = [analyze_snapshot(path) for path in files]
    return {
        "snapshot_count": len(reports),
        "negative_gain_snapshot_count": sum(
            report["negative_gain_voice_count"] != 0 for report in reports
        ),
        "negative_gain_voice_count": sum(
            report["negative_gain_voice_count"] for report in reports
        ),
        "unbalanced_cycle_pair_count": sum(
            report["unbalanced_cycle_pair_count"] for report in reports
        ),
        "scope": "saved SPC DSP register images only; not a dynamic runtime trace",
        "files": reports,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=pathlib.Path)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()
    print(json.dumps(analyze_collection(args.input), indent=2 if args.pretty else None, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
