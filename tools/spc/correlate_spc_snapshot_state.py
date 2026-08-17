#!/usr/bin/env python3
"""Correlate ordered SPC snapshots against saved machine state.

This is a provenance/mapping instrument, not an attribution classifier. It reads
only execution-state bytes from conventional SPC snapshots. ID666/header text is
never parsed or used as evidence.

The primary use is packs whose files have an external ordinal (for example
``01 - BGM 1.spc`` ... ``25 - BGM 25.spc``). The tool asks whether some saved
SPC700/DSP state variable tracks that ordinal exactly or nearly exactly. An exact
sequence is strong evidence for a driver selector/state counter, but does not by
itself prove what that variable means in the original game's sound test.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import pathlib
import re
from dataclasses import dataclass
from typing import Iterable


SPC_SIGNATURE_PREFIX = b"SNES-SPC700 Sound File Data"
SPC_MIN_FILE_SIZE = 0x10180
SPC_RAM_OFFSET = 0x100
SPC_RAM_SIZE = 0x10000
SPC_DSP_OFFSET = 0x10100
SPC_DSP_SIZE = 0x80

DEFAULT_ORDINAL_RE = re.compile(r"^(\d+)")


@dataclass(frozen=True)
class Snapshot:
    ordinal: int
    path: pathlib.Path
    sha256: str
    cpu_pc: int
    cpu_registers: dict[str, int]
    ram: bytes
    dsp: bytes


def parse_ordinal(path: pathlib.Path, pattern: re.Pattern[str]) -> int:
    match = pattern.search(path.name)
    if match is None:
        raise ValueError(f"could not recover ordinal from filename: {path.name}")
    value = int(match.group(1), 10)
    if value < 0 or value > 65535:
        raise ValueError(f"ordinal outside supported range: {value}")
    return value


def load_snapshot(path: pathlib.Path, ordinal: int) -> Snapshot:
    data = path.read_bytes()
    if len(data) < SPC_MIN_FILE_SIZE:
        raise ValueError(f"truncated SPC snapshot: {path}")
    if not data.startswith(SPC_SIGNATURE_PREFIX):
        raise ValueError(f"not an SPC snapshot: {path}")

    # The common parser in components/spc/spc_snapshot.h uses these same exact
    # layout offsets. Deliberately skip 0x2C..0xFF because that area can contain
    # ID666/text metadata and is not machine-state evidence.
    cpu_pc = data[0x25] | (data[0x26] << 8)
    cpu_registers = {
        "a": data[0x27],
        "x": data[0x28],
        "y": data[0x29],
        "psw": data[0x2A],
        "sp": data[0x2B],
    }
    return Snapshot(
        ordinal=ordinal,
        path=path,
        sha256=hashlib.sha256(data).hexdigest(),
        cpu_pc=cpu_pc,
        cpu_registers=cpu_registers,
        ram=data[SPC_RAM_OFFSET : SPC_RAM_OFFSET + SPC_RAM_SIZE],
        dsp=data[SPC_DSP_OFFSET : SPC_DSP_OFFSET + SPC_DSP_SIZE],
    )


def load_ordered_snapshots(
    directory: pathlib.Path,
    *,
    ordinal_pattern: re.Pattern[str] = DEFAULT_ORDINAL_RE,
    expected_count: int | None = None,
) -> list[Snapshot]:
    files = sorted(directory.glob("*.spc"))
    if not files:
        raise ValueError(f"no SPC files found in {directory}")
    snapshots = [load_snapshot(path, parse_ordinal(path, ordinal_pattern)) for path in files]
    snapshots.sort(key=lambda item: item.ordinal)

    ordinals = [item.ordinal for item in snapshots]
    if len(set(ordinals)) != len(ordinals):
        raise ValueError("SPC fixture ordinals must be unique")
    if expected_count is not None and len(snapshots) != expected_count:
        raise ValueError(
            f"expected {expected_count} SPC snapshots, found {len(snapshots)}"
        )
    if ordinals != list(range(ordinals[0], ordinals[0] + len(ordinals))):
        raise ValueError("SPC fixture ordinals must form one contiguous sequence")
    return snapshots


def best_modulo_delta(values: list[int], ordinals: list[int], modulus: int) -> tuple[int, int]:
    if len(values) != len(ordinals):
        raise ValueError("value/ordinal length mismatch")
    counts = collections.Counter((value - ordinal) % modulus for value, ordinal in zip(values, ordinals))
    delta, matches = counts.most_common(1)[0]
    return delta, matches


def neighborhood_stability(
    snapshots: list[Snapshot],
    address: int,
    *,
    radius: int = 8,
) -> float:
    lo = max(0, address - radius)
    hi = min(SPC_RAM_SIZE, address + radius + 1)
    total = 0
    stable = 0
    for offset in range(lo, hi):
        if offset == address:
            continue
        total += 1
        first = snapshots[0].ram[offset]
        stable += int(all(snapshot.ram[offset] == first for snapshot in snapshots[1:]))
    return stable / total if total else 0.0


def describe_vector(
    *,
    space: str,
    location: str,
    values: list[int],
    ordinals: list[int],
    modulus: int,
    neighborhood_stability_value: float | None = None,
) -> dict[str, object]:
    delta, matches = best_modulo_delta(values, ordinals, modulus)
    one_based_matches = sum(value == ordinal for value, ordinal in zip(values, ordinals))
    zero_based_matches = sum(value == ordinal - 1 for value, ordinal in zip(values, ordinals))
    result: dict[str, object] = {
        "space": space,
        "location": location,
        "values": values,
        "unique_value_count": len(set(values)),
        "one_based_match_count": one_based_matches,
        "zero_based_match_count": zero_based_matches,
        "best_additive_delta_modulo": delta,
        "best_additive_match_count": matches,
        "best_additive_match_fraction": matches / len(values),
        "exact_one_based": one_based_matches == len(values),
        "exact_zero_based": zero_based_matches == len(values),
        "exact_additive": matches == len(values),
    }
    if neighborhood_stability_value is not None:
        result["neighbor_stability"] = neighborhood_stability_value
    return result


def iter_candidate_vectors(snapshots: list[Snapshot]) -> Iterable[dict[str, object]]:
    ordinals = [snapshot.ordinal for snapshot in snapshots]

    for register in ("a", "x", "y", "psw", "sp"):
        values = [snapshot.cpu_registers[register] for snapshot in snapshots]
        yield describe_vector(
            space="cpu_u8",
            location=register,
            values=values,
            ordinals=ordinals,
            modulus=256,
        )

    pc_values = [snapshot.cpu_pc for snapshot in snapshots]
    yield describe_vector(
        space="cpu_u16",
        location="pc",
        values=pc_values,
        ordinals=ordinals,
        modulus=65536,
    )

    for address in range(SPC_RAM_SIZE):
        values = [snapshot.ram[address] for snapshot in snapshots]
        if len(set(values)) <= 1:
            continue
        yield describe_vector(
            space="ram_u8",
            location=f"0x{address:04X}",
            values=values,
            ordinals=ordinals,
            modulus=256,
            neighborhood_stability_value=neighborhood_stability(snapshots, address),
        )

    for address in range(SPC_RAM_SIZE - 1):
        values = [
            snapshot.ram[address] | (snapshot.ram[address + 1] << 8)
            for snapshot in snapshots
        ]
        if len(set(values)) <= 1:
            continue
        yield describe_vector(
            space="ram_u16_le",
            location=f"0x{address:04X}",
            values=values,
            ordinals=ordinals,
            modulus=65536,
        )

    for address in range(SPC_DSP_SIZE):
        values = [snapshot.dsp[address] for snapshot in snapshots]
        if len(set(values)) <= 1:
            continue
        yield describe_vector(
            space="dsp_u8",
            location=f"0x{address:02X}",
            values=values,
            ordinals=ordinals,
            modulus=256,
        )


def candidate_sort_key(item: dict[str, object]) -> tuple[float, float, float, int, str, str]:
    exact_rank = 1.0 if item["exact_one_based"] else 0.9 if item["exact_zero_based"] else 0.8 if item["exact_additive"] else 0.0
    stability = float(item.get("neighbor_stability", 0.0))
    fraction = float(item["best_additive_match_fraction"])
    unique = int(item["unique_value_count"])
    return (exact_rank, fraction, stability, unique, str(item["space"]), str(item["location"]))


def correlate(snapshots: list[Snapshot], *, top: int = 64) -> dict[str, object]:
    if top <= 0:
        raise ValueError("top must be positive")
    ordinals = [snapshot.ordinal for snapshot in snapshots]
    variable_state_vector_count = 0
    exact: list[dict[str, object]] = []
    near: list[dict[str, object]] = []
    minimum_unique_for_near = max(4, len(snapshots) // 2)

    # Stream the full 64 KiB + u16 + DSP search. Real packs may differ across
    # large sequence-data regions, so retaining every variable vector would turn
    # a small forensic report into an unnecessary memory spike.
    for item in iter_candidate_vectors(snapshots):
        variable_state_vector_count += 1
        is_exact = bool(
            item["exact_one_based"]
            or item["exact_zero_based"]
            or item["exact_additive"]
        )
        if is_exact:
            exact.append(item)
        elif (
            float(item["best_additive_match_fraction"]) >= 0.80
            and int(item["unique_value_count"]) >= minimum_unique_for_near
        ):
            near.append(item)

    exact.sort(key=candidate_sort_key, reverse=True)
    near.sort(key=candidate_sort_key, reverse=True)

    exact_ram_one_based = [
        item for item in exact
        if item["space"] == "ram_u8" and item["exact_one_based"]
    ]
    exact_ram_zero_based = [
        item for item in exact
        if item["space"] == "ram_u8" and item["exact_zero_based"]
    ]

    return {
        "model": "SPC snapshot ordered-state correlator",
        "claim_boundary": (
            "Saved machine-state correlation only. ID666/header text is excluded. "
            "An ordinal-tracking state locus supports an internal selector/index hypothesis but does not by itself prove the original game sound-test semantic mapping or any creator attribution."
        ),
        "snapshot_count": len(snapshots),
        "ordinals": ordinals,
        "snapshots": [
            {
                "ordinal": snapshot.ordinal,
                "path": snapshot.path.as_posix(),
                "sha256": snapshot.sha256,
            }
            for snapshot in snapshots
        ],
        "summary": {
            "variable_state_vector_count": variable_state_vector_count,
            "exact_sequence_candidate_count": len(exact),
            "exact_one_based_ram_u8_count": len(exact_ram_one_based),
            "exact_zero_based_ram_u8_count": len(exact_ram_zero_based),
            "sequential_selector_locus_found": bool(exact_ram_one_based or exact_ram_zero_based),
        },
        "exact_sequence_candidates": exact[:top],
        "near_sequence_candidates": near[:top],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-dir", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--expected-count", type=int)
    parser.add_argument("--ordinal-regex", default=DEFAULT_ORDINAL_RE.pattern)
    parser.add_argument("--top", type=int, default=64)
    args = parser.parse_args()

    pattern = re.compile(args.ordinal_regex)
    snapshots = load_ordered_snapshots(
        args.input_dir,
        ordinal_pattern=pattern,
        expected_count=args.expected_count,
    )
    result = correlate(snapshots, top=args.top)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
