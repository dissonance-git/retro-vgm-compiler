#!/usr/bin/env python3
"""Symbolic causal-closure control for S-DSP FIR feedback topology.

This control deliberately separates two questions:

1. work-observed echo state: exact EDL/EFB/FIR bytes in a committed SPC snapshot;
2. structural feedback closure: how the support of a one-position perturbation
   expands when repeatedly passed through an eight-tap FIR relation.

The polynomial calculation is exact over integer FIR coefficients. It models
ideal linear causal-path support before S-DSP fixed-point shifts, saturation,
quantization, and possible baseline-dependent cancellation. It is therefore a
structural closure control, not a rendered waveform predictor.
"""

from __future__ import annotations

import json
import pathlib

SPC_SIGNATURE = b"SNES-SPC700 Sound File Data"
DSP_OFFSET = 0x10100
DSP_SIZE = 0x80
S_DSP_SAMPLE_RATE = 32000


def signed8(value: int) -> int:
    return value - 0x100 if value & 0x80 else value


def parse_echo_state(path: pathlib.Path) -> dict:
    data = path.read_bytes()
    if not data.startswith(SPC_SIGNATURE):
        raise ValueError("not an SNES-SPC700 snapshot")
    if len(data) < DSP_OFFSET + DSP_SIZE:
        raise ValueError("truncated SNES-SPC700 snapshot")
    dsp = data[DSP_OFFSET : DSP_OFFSET + DSP_SIZE]
    edl = dsp[0x7D] & 0x0F
    return {
        "efb": signed8(dsp[0x0D]),
        "eon": dsp[0x4D],
        "esa": dsp[0x6D],
        "edl": edl,
        "delay_sample_frames": edl * 512,
        "nominal_delay_ms": (edl * 512 * 1000.0 / S_DSP_SAMPLE_RATE),
        "fir": [signed8(dsp[0x0F + 0x10 * tap]) for tap in range(8)],
    }


def convolve_integer(lhs: list[int], rhs: list[int]) -> list[int]:
    out = [0] * (len(lhs) + len(rhs) - 1)
    for i, a in enumerate(lhs):
        for j, b in enumerate(rhs):
            out[i + j] += a * b
    return out


def fir_power(coefficients: list[int], generation: int) -> list[int]:
    if len(coefficients) != 8:
        raise ValueError("S-DSP FIR control requires exactly eight coefficients")
    if generation < 0:
        raise ValueError("generation must be nonnegative")
    result = [1]
    for _ in range(generation):
        result = convolve_integer(result, coefficients)
    return result


def support_indices(coefficients: list[int], generation: int) -> list[int]:
    return [i for i, value in enumerate(fir_power(coefficients, generation)) if value != 0]


def support_summary(coefficients: list[int], generations: int = 7) -> list[dict]:
    rows = []
    for generation in range(1, generations + 1):
        support = support_indices(coefficients, generation)
        rows.append(
            {
                "generation": generation,
                "nonzero_support_count": len(support),
                "support_span": 0 if not support else support[-1] - support[0] + 1,
                "full_eight_tap_ceiling": 7 * generation + 1,
            }
        )
    return rows


# Pinned A Link to the Past N-SPC FIR table at loveemu/vgm-disasm
# e96c5b35649f8e814cac3c31b65cedc07b52d76d.
NSPC_FIR_PRESETS = {
    0: [127, 0, 0, 0, 0, 0, 0, 0],
    1: [88, -65, -37, -16, -2, 7, 12, 12],
    2: [12, 33, 43, 43, 19, -2, -13, -7],
    3: [52, 51, 0, -39, -27, 1, -4, -21],
}


def run_control(repo_root: pathlib.Path | None = None) -> dict:
    if repo_root is None:
        repo_root = pathlib.Path(__file__).resolve().parents[1]

    naval_path = (
        repo_root
        / "tests"
        / "corpus"
        / "front-mission-gun-hazard"
        / "1.33 - Naval Fortress.spc"
    )
    naval = parse_echo_state(naval_path)

    # Work-observed negative control: this snapshot uses one nonzero FIR tap.
    assert naval["efb"] == 43
    assert naval["eon"] == 0x6F
    assert naval["edl"] == 7
    assert naval["fir"] == [126, 0, 0, 0, 0, 0, 0, 0]
    naval_support = support_summary(naval["fir"])
    assert [row["nonzero_support_count"] for row in naval_support] == [1] * 7

    nspc = {preset: support_summary(coefficients) for preset, coefficients in NSPC_FIR_PRESETS.items()}
    assert [row["nonzero_support_count"] for row in nspc[0]] == [1] * 7
    assert [row["nonzero_support_count"] for row in nspc[1]] == [8, 15, 22, 29, 36, 43, 50]
    assert [row["nonzero_support_count"] for row in nspc[2]] == [8, 15, 22, 29, 36, 43, 50]
    assert [row["nonzero_support_count"] for row in nspc[3]] == [7, 15, 22, 29, 36, 43, 50]

    return {
        "work_observed_control": {
            "work": "Front Mission: Gun Hazard - Naval Fortress",
            "scope": "saved SPC DSP register image",
            "echo_state": naval,
            "symbolic_feedback_support": naval_support,
        },
        "nspc_driver_fir_controls": {
            str(preset): {
                "coefficients": coefficients,
                "symbolic_feedback_support": nspc[preset],
            }
            for preset, coefficients in NSPC_FIR_PRESETS.items()
        },
        "result": {
            "single_tap_feedback_broadens_support": False,
            "nspc_preset_1_reaches_full_7k_plus_1_support": True,
            "nspc_preset_2_reaches_full_7k_plus_1_support": True,
            "nspc_preset_3_reaches_full_support_from_generation_2": True,
        },
        "interpretation": [
            "A one-position intervention does not imply a large causal-support closure merely because feedback exists; a single-tap relation remains one-position-wide across feedback generations.",
            "Multi-tap FIR structure can expand the symbolic support of the same one-position perturbation over successive feedback generations.",
            "The closure burden is therefore a property of retained relation topology, not just intervention size or the existence of a feedback loop.",
            "This is an audio-side finite analogue of Helix closure-amplification controls, not a new DSP theorem.",
        ],
        "claim_boundary": [
            "Polynomial support is exact for the ideal integer FIR path relation, not the final S-DSP fixed-point waveform after shifts, clipping, even-value truncation, baseline interaction, and coefficient cancellation at later device stages.",
            "The Gun Hazard FIR/EFB/EDL values are work-observed in one saved SPC state; the N-SPC FIR presets are driver-reachable table structures, not proof that A Link to the Past uses every preset in a specific cue.",
            "A nonzero FIR path coefficient establishes a causal path, not audibility of the eventual contribution.",
        ],
    }


def main() -> int:
    print(json.dumps(run_control(), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
