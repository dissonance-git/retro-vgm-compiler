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


DEFAULT_STRAND_MIN_CONFIDENCE = 0.75

_IDENTITY_BEARING_KINDS = {
    "source_identity",
    "instrument_program_identity",
    "authored_track_identity",
    "driver_track_identity",
    "external_identity",
}
_SUPPORT_DOMAIN = {
    "physical_slot_continuity": 0,
    "source_identity": 1,
    "instrument_program_identity": 1,
    "authored_track_identity": 1,
    "driver_track_identity": 1,
    "external_identity": 1,
    "pitch_trajectory_continuity": 2,
    "temporal_adjacency": 3,
    "articulation_continuity": 4,
    "rhythmic_role_continuity": 4,
}


def _strand_evidence_summary(
    hypothesis: dict[str, Any],
) -> tuple[set[str], bool, bool, bool]:
    evidence = hypothesis.get("evidence")
    if not isinstance(evidence, list):
        raise ValueError("strand hypothesis edge requires an evidence array")

    support_kinds: set[str] = set()
    support_domains: set[int] = set()
    identity_support = False
    strong_conflict = False
    for item in evidence:
        if not isinstance(item, dict):
            raise ValueError("strand hypothesis evidence item must be an object")
        kind = item.get("kind")
        polarity = item.get("polarity")
        confidence = item.get("confidence")
        if not isinstance(kind, str) or polarity not in {"supports", "counters"}:
            raise ValueError("strand hypothesis evidence item has invalid semantics")
        if (
            not isinstance(confidence, (int, float))
            or not math.isfinite(float(confidence))
            or float(confidence) < 0.0
            or float(confidence) > 1.0
        ):
            raise ValueError("strand hypothesis evidence item has invalid confidence")
        if polarity == "supports":
            support_kinds.add(kind)
            domain = _SUPPORT_DOMAIN.get(kind)
            if domain is not None:
                support_domains.add(domain)
            if kind in _IDENTITY_BEARING_KINDS:
                identity_support = True
        elif (
            float(confidence) >= 0.80
            and kind in {"simultaneous_conflict", "identity_discontinuity"}
        ):
            strong_conflict = True

    return (
        support_kinds,
        identity_support,
        len(support_domains) >= 2,
        strong_conflict,
    )


def assemble_strand_hypotheses(
    episodes: list[Episode],
    hypotheses: list[dict[str, Any]],
    *,
    min_confidence: float = DEFAULT_STRAND_MIN_CONFIDENCE,
) -> dict[str, Any]:
    """Conservatively assemble pairwise continuity into non-overlapping strands.

    This is intentionally not graph connected-components. A link must be
    identity-bearing, cross-domain grounded, temporally adjacent, conflict-free,
    and above the single-domain confidence ceiling. For each episode only the
    earliest admissible successor may be considered. Ambiguous successor or
    predecessor forks remain unresolved instead of being collapsed.
    """
    if (
        not math.isfinite(min_confidence)
        or min_confidence <= SINGLE_DOMAIN_CEILING
        or min_confidence > 1.0
    ):
        raise ValueError(
            "strand min_confidence must be finite, greater than the "
            "single-domain ceiling, and at most 1.0"
        )

    by_id = {episode.episode_id: episode for episode in episodes}
    if len(by_id) != len(episodes):
        raise ValueError("episode ids must be unique for strand assembly")

    candidates_by_first: dict[int, list[dict[str, Any]]] = {}
    for hypothesis in hypotheses:
        first_id = hypothesis.get("first_episode_id")
        second_id = hypothesis.get("second_episode_id")
        if not isinstance(first_id, int) or not isinstance(second_id, int):
            raise ValueError("strand hypothesis edge is missing integer episode ids")
        if first_id not in by_id or second_id not in by_id:
            raise ValueError("strand hypothesis edge references an unknown episode")
        if first_id == second_id:
            continue

        first = by_id[first_id]
        second = by_id[second_id]
        confidence = hypothesis.get("confidence")
        if not isinstance(confidence, (int, float)) or not math.isfinite(float(confidence)):
            raise ValueError("strand hypothesis edge has invalid confidence")

        if second.start_tick <= first.start_tick:
            continue
        if first.end_tick > second.start_tick:
            continue
        support_kinds, identity_support, cross_domain, strong_conflict = (
            _strand_evidence_summary(hypothesis)
        )
        if hypothesis.get("identity_bearing_support") is not identity_support:
            raise ValueError(
                "strand hypothesis identity-bearing summary disagrees with evidence"
            )
        if hypothesis.get("cross_domain_grounded") is not cross_domain:
            raise ValueError(
                "strand hypothesis cross-domain summary disagrees with evidence"
            )
        if hypothesis.get("strong_conflict_present") is not strong_conflict:
            raise ValueError(
                "strand hypothesis conflict summary disagrees with evidence"
            )

        if float(confidence) < min_confidence:
            continue
        if not identity_support:
            continue
        if not cross_domain:
            continue
        if strong_conflict:
            continue
        if "temporal_adjacency" not in support_kinds:
            continue

        candidates_by_first.setdefault(first_id, []).append(hypothesis)

    unresolved: list[dict[str, Any]] = []
    provisional: dict[int, dict[str, Any]] = {}
    for first_id, candidates in sorted(candidates_by_first.items()):
        earliest_tick = min(by_id[int(item["second_episode_id"])].start_tick for item in candidates)
        earliest = [
            item
            for item in candidates
            if by_id[int(item["second_episode_id"])].start_tick == earliest_tick
        ]
        if len(earliest) != 1:
            unresolved.append({
                "kind": "ambiguous_successor",
                "episode_id": first_id,
                "candidate_episode_ids": sorted(
                    int(item["second_episode_id"]) for item in earliest
                ),
            })
            continue
        provisional[first_id] = earliest[0]

    incoming: dict[int, list[tuple[int, dict[str, Any]]]] = {}
    for first_id, hypothesis in provisional.items():
        incoming.setdefault(int(hypothesis["second_episode_id"]), []).append(
            (first_id, hypothesis)
        )

    accepted: dict[int, dict[str, Any]] = {}
    for second_id, incoming_edges in sorted(incoming.items()):
        if len(incoming_edges) != 1:
            unresolved.append({
                "kind": "ambiguous_predecessor",
                "episode_id": second_id,
                "candidate_episode_ids": sorted(first_id for first_id, _ in incoming_edges),
            })
            continue
        first_id, hypothesis = incoming_edges[0]
        accepted[first_id] = hypothesis

    target_ids = {int(item["second_episode_id"]) for item in accepted.values()}
    starts = sorted(
        (first_id for first_id in accepted if first_id not in target_ids),
        key=lambda episode_id: (
            by_id[episode_id].start_tick,
            episode_id,
        ),
    )

    strands: list[dict[str, Any]] = []
    visited_edges: set[tuple[int, int]] = set()
    for start_id in starts:
        episode_ids = [start_id]
        links: list[dict[str, Any]] = []
        current = start_id
        seen = {start_id}
        while current in accepted:
            hypothesis = accepted[current]
            next_id = int(hypothesis["second_episode_id"])
            edge = (current, next_id)
            if edge in visited_edges or next_id in seen:
                raise ValueError("strand assembly encountered a non-forward cycle")
            visited_edges.add(edge)
            links.append({
                "first_episode_id": current,
                "second_episode_id": next_id,
                "confidence": float(hypothesis["confidence"]),
            })
            episode_ids.append(next_id)
            seen.add(next_id)
            current = next_id

        if len(episode_ids) < 2:
            continue
        strand_episodes = [by_id[episode_id] for episode_id in episode_ids]
        strands.append({
            "episode_ids": episode_ids,
            "start_tick": strand_episodes[0].start_tick,
            "end_tick": strand_episodes[-1].end_tick,
            "confidence": min(link["confidence"] for link in links),
            "links": links,
        })

    if len(visited_edges) != len(accepted):
        raise ValueError("strand assembly left accepted links unreachable")

    return {
        "model": "conservative creator-blind Genesis persistent-part strand hypotheses",
        "claim_boundary": (
            "Each strand is a conservative grouping hypothesis over bounded "
            "episodes. It is not a finalized musical-part identity and contains "
            "no creator or source-path labels."
        ),
        "policy": {
            "min_confidence": min_confidence,
            "requires_identity_bearing_support": True,
            "requires_cross_domain_grounding": True,
            "requires_temporal_adjacency": True,
            "rejects_strong_conflict": True,
            "fork_policy": "preserve_as_unresolved",
        },
        "strands": strands,
        "unresolved": unresolved,
    }


def project(
    capsule: dict[str, Any],
    *,
    max_gap_ticks: int,
    max_pitch_interval_octaves: float,
    strand_min_confidence: float | None = None,
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
    payload = {
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
    if strand_min_confidence is not None:
        payload["strand_projection"] = assemble_strand_hypotheses(
            episodes,
            hypotheses,
            min_confidence=strand_min_confidence,
        )
    return payload


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capsule", type=pathlib.Path)
    parser.add_argument("--max-gap-ticks", type=int, required=True)
    parser.add_argument("--max-pitch-interval-octaves", type=float, default=2.0)
    parser.add_argument("--strand-min-confidence", type=float)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    capsule = json.loads(args.capsule.read_text(encoding="utf-8"))
    payload = project(
        capsule,
        max_gap_ticks=args.max_gap_ticks,
        max_pitch_interval_octaves=args.max_pitch_interval_octaves,
        strand_min_confidence=args.strand_min_confidence,
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
