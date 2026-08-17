#!/usr/bin/env python3
"""Reconstruct YM2612 episodes and persistent-part evidence from cached songs.

This is the cheap post-ingestion path. It never opens VGM/VGZ and never reads
creator labels. Pairwise continuity mirrors
`components/vgm/enhancement/genesis_part_evidence.h`; physical channel identity
is supporting evidence only and can never become a persistent musical part by
itself.
"""

from __future__ import annotations

import argparse
import json
import math
import pathlib
from dataclasses import dataclass
from typing import Any

EXPECTED_SCHEMA_VERSION = 3
EXPECTED_EXTRACTOR = {"name": "creator-blind-genesis-song-capsule", "version": 3}
MODEL = "creator-blind cached Genesis persistent-part evidence"

SLOT_ONLY_CEILING = 0.35
NO_IDENTITY_CEILING = 0.64
SINGLE_DOMAIN_CEILING = 0.74
STRONG_CONFLICT_CEILING = 0.49


@dataclass(frozen=True)
class Episode:
    episode_id: int
    start_tick: int
    end_tick: int
    channel: int
    fnum: int
    block: int
    patch_full_id: int
    onset_gate_event_index: int
    end_gate_event_index: int | None

    @property
    def pitch_coordinate(self) -> float:
        return float(self.fnum) * (2.0 ** self.block)


def _columns(value: Any, required: tuple[str, ...], label: str) -> dict[str, list[Any]]:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    result: dict[str, list[Any]] = {}
    lengths: set[int] = set()
    for key in required:
        column = value.get(key)
        if not isinstance(column, list):
            raise ValueError(f"{label}.{key} must be an array")
        result[key] = column
        lengths.add(len(column))
    if len(lengths) > 1:
        raise ValueError(f"{label} columns must have equal lengths")
    return result


def reconstruct_episodes(capsule: dict[str, Any]) -> list[Episode]:
    if capsule.get("schema_version") != EXPECTED_SCHEMA_VERSION:
        raise ValueError("Genesis song capsule schema is not episode-order complete")
    if capsule.get("extractor") != EXPECTED_EXTRACTOR:
        raise ValueError("unexpected Genesis song capsule extractor")

    ym2612 = capsule.get("ym2612")
    if not isinstance(ym2612, dict):
        raise ValueError("capsule is missing YM2612 evidence")
    events = _columns(
        ym2612.get("events"),
        ("tick", "channel", "fnum", "block", "patch_full_id", "key_gate_event_index"),
        "ym2612.events",
    )
    gates = _columns(
        ym2612.get("key_gate_events"),
        ("tick", "channel", "operator_mask"),
        "ym2612.key_gate_events",
    )
    timing = capsule.get("timing")
    if not isinstance(timing, dict) or not isinstance(timing.get("duration_vgm_samples"), int):
        raise ValueError("capsule is missing VGM duration")
    duration = int(timing["duration_vgm_samples"])

    episodes: list[Episode] = []
    count = len(events["tick"])
    for index in range(count):
        start = int(events["tick"][index])
        channel = int(events["channel"][index])
        gate_index = int(events["key_gate_event_index"][index])
        if not 0 <= gate_index < len(gates["tick"]):
            raise ValueError("onset references an out-of-range key-gate event")
        if int(gates["tick"][gate_index]) != start:
            raise ValueError("onset and referenced key-gate tick disagree")
        if int(gates["channel"][gate_index]) != channel:
            raise ValueError("onset and referenced key-gate channel disagree")
        if int(gates["operator_mask"][gate_index]) != 0xF0:
            raise ValueError("ordinary full-key onset must reference a full-key gate event")

        end_gate: int | None = None
        end = duration
        for candidate in range(gate_index + 1, len(gates["tick"])):
            if int(gates["channel"][candidate]) == channel:
                end_gate = candidate
                end = int(gates["tick"][candidate])
                break
        if end < start:
            raise ValueError("key-gate order yields a negative episode span")

        episodes.append(Episode(
            episode_id=index,
            start_tick=start,
            end_tick=end,
            channel=channel,
            fnum=int(events["fnum"][index]),
            block=int(events["block"][index]),
            patch_full_id=int(events["patch_full_id"][index]),
            onset_gate_event_index=gate_index,
            end_gate_event_index=end_gate,
        ))
    return episodes


def infer_continuity(
    first: Episode,
    second: Episode,
    *,
    max_gap_ticks: int,
    max_pitch_interval_octaves: float,
) -> dict[str, Any]:
    if first.episode_id == second.episode_id:
        raise ValueError("persistent part requires two distinct episodes")
    if max_gap_ticks < 0:
        raise ValueError("max_gap_ticks must be nonnegative")
    if not math.isfinite(max_pitch_interval_octaves) or max_pitch_interval_octaves < 0.0:
        raise ValueError("max_pitch_interval_octaves must be finite and nonnegative")

    evidence: list[dict[str, Any]] = []
    support_domains: set[int] = set()
    proposed = 0.30
    identity_support = False
    slot_support = False
    non_slot_support = False
    strong_conflict = False

    def support(kind: str, domain: int, confidence: float) -> None:
        nonlocal slot_support, non_slot_support
        evidence.append({"kind": kind, "polarity": "supports", "confidence": confidence})
        support_domains.add(domain)
        if kind == "physical_slot_continuity":
            slot_support = True
        else:
            non_slot_support = True

    def counter(kind: str, confidence: float) -> None:
        nonlocal strong_conflict
        evidence.append({"kind": kind, "polarity": "counters", "confidence": confidence})
        if confidence >= 0.80 and kind in {"simultaneous_conflict", "identity_discontinuity"}:
            strong_conflict = True

    if first.channel == second.channel:
        support("physical_slot_continuity", 0, 0.55)
        proposed += 0.08

    if first.patch_full_id == second.patch_full_id:
        support("instrument_program_identity", 1, 0.95)
        identity_support = True
        proposed += 0.31
    else:
        counter("identity_discontinuity", 0.62)
        proposed -= 0.08

    gap = second.start_tick - first.end_tick
    if gap < 0:
        counter("simultaneous_conflict", 0.88)
        proposed -= 0.20
    elif max_gap_ticks > 0 and gap <= max_gap_ticks:
        closeness = 1.0 - float(gap) / float(max_gap_ticks)
        confidence = 0.70 + 0.20 * min(1.0, max(0.0, closeness))
        support("temporal_adjacency", 3, confidence)
        proposed += 0.20

    pitch_a = first.pitch_coordinate
    pitch_b = second.pitch_coordinate
    if pitch_a > 0.0 and pitch_b > 0.0:
        octaves = abs(math.log2(pitch_b / pitch_a))
        if octaves <= max_pitch_interval_octaves:
            if max_pitch_interval_octaves == 0.0:
                interval_fit = 1.0 if octaves == 0.0 else 0.0
            else:
                interval_fit = 1.0 - octaves / max_pitch_interval_octaves
            confidence = 0.58 + 0.18 * min(1.0, max(0.0, interval_fit))
            support("pitch_trajectory_continuity", 2, confidence)
            proposed += 0.13

    if not any(item["polarity"] == "supports" for item in evidence):
        raise ValueError("Genesis episodes do not contain enough positive evidence for part continuity")

    proposed = min(0.94, max(0.0, proposed))
    cross_domain = len(support_domains) >= 2
    confidence = proposed
    if slot_support and not non_slot_support:
        confidence = min(confidence, SLOT_ONLY_CEILING)
    elif not identity_support:
        confidence = min(confidence, NO_IDENTITY_CEILING)
    elif not cross_domain:
        confidence = min(confidence, SINGLE_DOMAIN_CEILING)
    if strong_conflict:
        confidence = min(confidence, STRONG_CONFLICT_CEILING)

    return {
        "first_episode_id": first.episode_id,
        "second_episode_id": second.episode_id,
        "proposed_confidence": proposed,
        "confidence": confidence,
        "identity_bearing_support": identity_support,
        "cross_domain_grounded": cross_domain,
        "strong_conflict_present": strong_conflict,
        "gap_ticks": gap,
        "evidence": evidence,
    }


def project(
    capsule: dict[str, Any],
    *,
    max_gap_ticks: int,
    max_pitch_interval_octaves: float,
) -> dict[str, Any]:
    episodes = reconstruct_episodes(capsule)
    hypotheses: list[dict[str, Any]] = []
    for first in episodes:
        for second in episodes:
            if first.episode_id == second.episode_id or second.start_tick < first.start_tick:
                continue
            try:
                hypotheses.append(infer_continuity(
                    first,
                    second,
                    max_gap_ticks=max_gap_ticks,
                    max_pitch_interval_octaves=max_pitch_interval_octaves,
                ))
            except ValueError as error:
                if "enough positive evidence" not in str(error):
                    raise
    return {
        "model": MODEL,
        "claim_boundary": (
            "Cache-only YM2612 physical episodes and pairwise persistent-part hypotheses. "
            "No pair is a finalized musical part and no creator labels are read."
        ),
        "policy": {
            "max_gap_ticks": max_gap_ticks,
            "max_pitch_interval_octaves": max_pitch_interval_octaves,
        },
        "episodes": [episode.__dict__ for episode in episodes],
        "hypotheses": hypotheses,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capsule", type=pathlib.Path)
    parser.add_argument("--max-gap-ticks", type=int, required=True)
    parser.add_argument("--max-pitch-interval-octaves", type=float, default=2.0)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    capsule = json.loads(args.capsule.read_text(encoding="utf-8"))
    payload = project(
        capsule,
        max_gap_ticks=args.max_gap_ticks,
        max_pitch_interval_octaves=args.max_pitch_interval_octaves,
    )
    text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
