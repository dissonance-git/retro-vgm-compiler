#!/usr/bin/env python3
"""Conservative YM2612 persistent-part trajectory pressure probe.

This is a real-corpus observatory below the shared persistent-part and phrase
models. It never equates a physical FM channel with a persistent musical part.

The probe searches for one deliberately narrow pattern:

    same YM2612 physical channel
    + unchanged core instrument program
    + a large inter-onset gap
    + repeated transposition/tempo-tolerant pitch-rhythm motif across that gap
    -> bounded persistent-part trajectory candidate

Two such candidates on distinct channels with aligned gap boundaries become a
cross-trajectory phrase-boundary target. They still do not become persistent
parts or global phrase boundaries until the shared graph materializes bounded
physical voice episodes, admits identity evidence, and checks conflicts.

No platform/system identity or creator metadata is read.
"""

from __future__ import annotations

import argparse
import collections
import importlib.util
import json
import math
from pathlib import Path
import statistics
import sys
from typing import Any, Iterable


def _load_motif_probe():
    path = Path(__file__).with_name("vgm_motif_probe.py")
    spec = importlib.util.spec_from_file_location("vgm_motif_probe", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


motif_probe = _load_motif_probe()


def _patch_core(event: Any) -> str:
    value = getattr(event, "patch_core", None)
    if not isinstance(value, str) or not value:
        raise ValueError("YM2612 trajectory observation requires patch_core")
    return value


def _validate_event_fields(event: Any) -> None:
    channel = int(getattr(event, "channel"))
    if channel < 0 or channel > 5:
        raise ValueError("YM2612 channel must lie in [0, 5]")
    if float(getattr(event, "frequency_measure")) <= 0.0:
        raise ValueError("YM2612 trajectory observation requires positive pitch measure")
    _patch_core(event)


def _analysis_identity(event: Any) -> tuple[str, float]:
    _validate_event_fields(event)
    return (_patch_core(event), float(getattr(event, "frequency_measure")))


def normalize_same_tick_observations(
    onsets: Iterable[Any],
) -> tuple[list[Any], dict[str, int]]:
    """Project exact key-on commands into unambiguous analysis observations.

    Multiple identical commands on one channel at one source tick collapse into
    one observation. If same-tick commands disagree on program or pitch, all of
    them are excluded from this probe as ambiguous. Exact commands remain in the
    source artifact; this projection does not rewrite them.
    """
    by_channel_tick: dict[tuple[int, int], list[Any]] = collections.defaultdict(list)
    raw = list(onsets)
    for event in raw:
        _validate_event_fields(event)
        by_channel_tick[(int(event.channel), int(event.tick))].append(event)

    normalized: list[Any] = []
    duplicate_collapses = 0
    ambiguous_exclusions = 0
    for key in sorted(by_channel_tick):
        group = by_channel_tick[key]
        identities = {_analysis_identity(event) for event in group}
        if len(identities) == 1:
            normalized.append(group[0])
            duplicate_collapses += len(group) - 1
        else:
            ambiguous_exclusions += len(group)

    normalized.sort(key=lambda item: (int(item.channel), int(item.tick)))
    return normalized, {
        "raw_source_onsets": len(raw),
        "analysis_observations": len(normalized),
        "same_tick_duplicate_collapses": duplicate_collapses,
        "same_tick_ambiguous_exclusions": ambiguous_exclusions,
    }


def _validate_events(events: list[Any]) -> None:
    previous_tick: int | None = None
    for event in events:
        _validate_event_fields(event)
        tick = int(getattr(event, "tick"))
        if previous_tick is not None and tick <= previous_tick:
            raise ValueError("normalized YM2612 trajectory observations must be strictly ordered")
        previous_tick = tick


def _contiguous_patch_runs(events: list[Any]) -> list[list[Any]]:
    _validate_events(events)
    if not events:
        return []
    runs: list[list[Any]] = []
    current = [events[0]]
    current_patch = _patch_core(events[0])
    for event in events[1:]:
        patch = _patch_core(event)
        if patch != current_patch:
            runs.append(current)
            current = [event]
            current_patch = patch
        else:
            current.append(event)
    runs.append(current)
    return runs


def _median_positive_gap(events: list[Any]) -> float:
    gaps = [
        int(current.tick) - int(previous.tick)
        for previous, current in zip(events, events[1:])
        if int(current.tick) > int(previous.tick)
    ]
    if not gaps:
        return 0.0
    return float(statistics.median(gaps))


def _candidate_id(channel: int, patch: str, boundary_tick: int) -> str:
    return f"ym2612-channel-{channel}-patch-{patch[:8]}-boundary-{boundary_tick}"


def discover_trajectory_candidates(
    onsets: Iterable[Any],
    *,
    motif_events: int = 3,
    minimum_gap_ratio: float = 2.0,
) -> list[dict[str, Any]]:
    if motif_events < 3:
        raise ValueError("trajectory motif windows require at least three events")
    if not math.isfinite(minimum_gap_ratio) or minimum_gap_ratio <= 1.0:
        raise ValueError("trajectory minimum gap ratio must be finite and greater than one")

    by_channel: dict[int, list[Any]] = collections.defaultdict(list)
    for event in onsets:
        by_channel[int(event.channel)].append(event)

    candidates: list[dict[str, Any]] = []
    for channel in sorted(by_channel):
        channel_events = sorted(by_channel[channel], key=lambda item: int(item.tick))
        for run in _contiguous_patch_runs(channel_events):
            if len(run) < motif_events * 2:
                continue
            median_gap = _median_positive_gap(run)
            if median_gap <= 0.0:
                continue

            patch = _patch_core(run[0])
            for boundary_index in range(motif_events, len(run) - motif_events + 1):
                left = run[boundary_index - motif_events : boundary_index]
                right = run[boundary_index : boundary_index + motif_events]
                gap = int(right[0].tick) - int(left[-1].tick)
                if gap <= 0:
                    continue
                gap_ratio = gap / median_gap
                if gap_ratio < minimum_gap_ratio:
                    continue

                left_signature = motif_probe._window_signature(left)
                right_signature = motif_probe._window_signature(right)
                if left_signature is None or right_signature is None:
                    continue
                if left_signature != right_signature:
                    continue

                boundary_tick = int(right[0].tick)
                left_support_ticks = [int(item.tick) for item in left]
                right_support_ticks = [int(item.tick) for item in right]
                support_ticks = left_support_ticks + right_support_ticks
                candidates.append({
                    "candidate_id": _candidate_id(channel, patch, boundary_tick),
                    "chip": "YM2612",
                    "channel": channel,
                    "patch_core": patch,
                    "boundary_tick": boundary_tick,
                    "gap_ticks": gap,
                    "local_median_gap_ticks": median_gap,
                    "gap_ratio": gap_ratio,
                    "motif_events_per_side": motif_events,
                    "motif_signature": left_signature,
                    "left_support_ticks": left_support_ticks,
                    "right_support_ticks": right_support_ticks,
                    "support_ticks": support_ticks,
                    "evidence": {
                        "physical_slot_continuity": True,
                        "instrument_program_identity": True,
                        "repeated_pitch_rhythm_motif": True,
                        "temporal_gap": True,
                    },
                    "persistent_part_status": "candidate_only",
                    "persistent_part_promoted": False,
                    "promotion_blocked_by": [
                        "bounded_physical_voice_episodes_not_materialized_in_shared_graph",
                        "competing_identity_links_not_conflict_checked",
                        "physical_channel_identity_is_not_musical_part_identity",
                    ],
                })

    candidates.sort(
        key=lambda item: (
            int(item["boundary_tick"]),
            int(item["channel"]),
            str(item["patch_core"]),
        )
    )
    return candidates


def describe_trajectory_support_relations(
    candidates: Iterable[dict[str, Any]],
) -> list[dict[str, Any]]:
    """Describe shared support without prematurely declaring identity conflict.

    A source observation may participate in two adjacent recurrence links that
    compose into a longer trajectory candidate. Shared support is therefore an
    ambiguity/provenance fact first, not a conflict verdict.
    """
    ordered = sorted(
        (dict(item) for item in candidates),
        key=lambda item: (
            int(item["channel"]),
            int(item["boundary_tick"]),
            str(item["candidate_id"]),
        ),
    )
    relations: list[dict[str, Any]] = []
    for index, first in enumerate(ordered):
        first_channel = int(first["channel"])
        first_support = set(int(tick) for tick in first["support_ticks"])
        first_left = [int(tick) for tick in first.get("left_support_ticks", [])]
        first_right = [int(tick) for tick in first.get("right_support_ticks", [])]
        for second in ordered[index + 1 :]:
            if int(second["channel"]) != first_channel:
                continue
            second_support = set(int(tick) for tick in second["support_ticks"])
            shared = sorted(first_support & second_support)
            if not shared:
                continue

            second_left = [int(tick) for tick in second.get("left_support_ticks", [])]
            second_right = [int(tick) for tick in second.get("right_support_ticks", [])]
            composable = (
                first_right == second_left
                or second_right == first_left
            )
            relations.append({
                "first_candidate_id": str(first["candidate_id"]),
                "second_candidate_id": str(second["candidate_id"]),
                "channel": first_channel,
                "shared_support_ticks": shared,
                "relation_kind": (
                    "composable_link_chain_candidate"
                    if composable
                    else "support_reuse_unresolved"
                ),
                "persistent_part_identity_established": False,
                "identity_conflict_established": False,
            })

    relations.sort(
        key=lambda item: (
            int(item["channel"]),
            str(item["first_candidate_id"]),
            str(item["second_candidate_id"]),
        )
    )
    return relations


def summarize_trajectory_link_components(
    candidates: Iterable[dict[str, Any]],
    support_relations: Iterable[dict[str, Any]],
) -> list[dict[str, Any]]:
    """Group recurrence links only where explicit composable support connects them."""
    candidate_by_id = {
        str(item["candidate_id"]): dict(item)
        for item in candidates
    }
    adjacency: dict[str, set[str]] = {
        candidate_id: set()
        for candidate_id in candidate_by_id
    }
    composable_edges: set[tuple[str, str]] = set()
    unresolved_touch: set[str] = set()

    for relation in support_relations:
        first_id = str(relation["first_candidate_id"])
        second_id = str(relation["second_candidate_id"])
        if first_id not in candidate_by_id or second_id not in candidate_by_id:
            raise ValueError("trajectory support relation references an unknown candidate")
        pair = tuple(sorted((first_id, second_id)))
        if relation["relation_kind"] == "composable_link_chain_candidate":
            adjacency[first_id].add(second_id)
            adjacency[second_id].add(first_id)
            composable_edges.add(pair)
        elif relation["relation_kind"] == "support_reuse_unresolved":
            unresolved_touch.update((first_id, second_id))
        else:
            raise ValueError("unknown trajectory support-relation kind")

    def candidate_key(candidate_id: str) -> tuple[int, int, str]:
        item = candidate_by_id[candidate_id]
        return (
            int(item["channel"]),
            int(item["boundary_tick"]),
            candidate_id,
        )

    components: list[dict[str, Any]] = []
    visited: set[str] = set()
    for seed in sorted(candidate_by_id, key=candidate_key):
        if seed in visited:
            continue
        stack = [seed]
        member_ids: list[str] = []
        visited.add(seed)
        while stack:
            current = stack.pop()
            member_ids.append(current)
            for neighbor in sorted(adjacency[current], key=candidate_key, reverse=True):
                if neighbor in visited:
                    continue
                visited.add(neighbor)
                stack.append(neighbor)

        member_ids.sort(key=candidate_key)
        member_set = set(member_ids)
        component_edges = sorted(
            pair
            for pair in composable_edges
            if pair[0] in member_set and pair[1] in member_set
        )
        members = [candidate_by_id[candidate_id] for candidate_id in member_ids]
        components.append({
            "candidate_ids": member_ids,
            "supporting_channels": sorted({int(item["channel"]) for item in members}),
            "boundary_ticks": [int(item["boundary_tick"]) for item in members],
            "candidate_count": len(member_ids),
            "composable_connection_count": len(component_edges),
            "chained": len(component_edges) > 0,
            "has_unresolved_support_reuse": any(
                candidate_id in unresolved_touch
                for candidate_id in member_ids
            ),
            "status": "recurrence_link_component_candidate",
            "persistent_part_identity_established": False,
        })

    components.sort(
        key=lambda item: (
            item["boundary_ticks"][0] if item["boundary_ticks"] else 0,
            item["supporting_channels"],
            item["candidate_ids"],
        )
    )
    return components


def align_cross_trajectory_boundaries(
    candidates: Iterable[dict[str, Any]],
    *,
    alignment_tolerance_ticks: int = 0,
) -> list[dict[str, Any]]:
    if alignment_tolerance_ticks < 0:
        raise ValueError("alignment tolerance must be nonnegative")

    ordered = sorted(
        (dict(item) for item in candidates),
        key=lambda item: (
            int(item["boundary_tick"]),
            int(item["channel"]),
            str(item["candidate_id"]),
        ),
    )

    groups: list[list[dict[str, Any]]] = []
    current: list[dict[str, Any]] = []
    anchor: int | None = None
    for item in ordered:
        tick = int(item["boundary_tick"])
        if current and anchor is not None and abs(tick - anchor) > alignment_tolerance_ticks:
            groups.append(current)
            current = []
            anchor = None
        if not current:
            anchor = tick
        current.append(item)
    if current:
        groups.append(current)

    results: list[dict[str, Any]] = []
    for group in groups:
        by_channel: dict[int, dict[str, Any]] = {}
        for item in group:
            channel = int(item["channel"])
            incumbent = by_channel.get(channel)
            if incumbent is None or float(item["gap_ratio"]) > float(incumbent["gap_ratio"]):
                by_channel[channel] = item
        if len(by_channel) < 2:
            continue

        selected = [by_channel[channel] for channel in sorted(by_channel)]
        ticks = [int(item["boundary_tick"]) for item in selected]
        representative = min(ticks) + (max(ticks) - min(ticks)) // 2
        results.append({
            "representative_tick": representative,
            "alignment_span_ticks": [min(ticks), max(ticks)],
            "supporting_channels": [int(item["channel"]) for item in selected],
            "supporting_candidate_ids": [str(item["candidate_id"]) for item in selected],
            "candidate_count": len(selected),
            "cross_trajectory_grounded": True,
            "cross_part_phrase_boundary_status": "candidate_target_only",
            "cross_part_phrase_boundary_promoted": False,
            "promotion_blocked_by": [
                "supporting_trajectories_are_not_yet_materialized_persistent_parts",
                "shared_phrase_boundary_consensus_not_executed",
            ],
        })
    return results


def analyze_onsets(
    onsets: Iterable[Any],
    *,
    source_name: str,
    motif_events: int = 3,
    minimum_gap_ratio: float = 2.0,
    alignment_tolerance_ticks: int = 0,
) -> dict[str, Any]:
    if not source_name:
        raise ValueError("trajectory probe requires a source name")
    observations, normalization = normalize_same_tick_observations(onsets)
    candidates = discover_trajectory_candidates(
        observations,
        motif_events=motif_events,
        minimum_gap_ratio=minimum_gap_ratio,
    )
    support_relations = describe_trajectory_support_relations(candidates)
    link_components = summarize_trajectory_link_components(candidates, support_relations)
    aligned = align_cross_trajectory_boundaries(
        candidates,
        alignment_tolerance_ticks=alignment_tolerance_ticks,
    )
    return {
        "source": source_name,
        "chip_scope": "YM2612",
        "platform_identity_consulted": False,
        "source_onset_observation_count": normalization["raw_source_onsets"],
        "observation_count": normalization["analysis_observations"],
        "same_tick_duplicate_collapses": normalization["same_tick_duplicate_collapses"],
        "same_tick_ambiguous_exclusions": normalization["same_tick_ambiguous_exclusions"],
        "trajectory_candidate_count": len(candidates),
        "trajectory_support_relation_count": len(support_relations),
        "composable_link_chain_candidate_count": sum(
            item["relation_kind"] == "composable_link_chain_candidate"
            for item in support_relations
        ),
        "unresolved_support_reuse_count": sum(
            item["relation_kind"] == "support_reuse_unresolved"
            for item in support_relations
        ),
        "trajectory_link_component_count": len(link_components),
        "chained_link_component_count": sum(
            bool(item["chained"])
            for item in link_components
        ),
        "largest_link_component_candidate_count": max(
            (int(item["candidate_count"]) for item in link_components),
            default=0,
        ),
        "cross_trajectory_boundary_candidate_count": len(aligned),
        "trajectory_candidates": candidates,
        "trajectory_support_relations": support_relations,
        "trajectory_link_components": link_components,
        "cross_trajectory_boundary_candidates": aligned,
        "shared_model_promotion": {
            "persistent_part_identity": "blocked",
            "cross_part_phrase_boundary": "blocked",
            "phrase_role": "blocked",
            "blocked_by": [
                "probe_does_not_materialize_shared_graph_voice_episodes",
                "probe_does_not_run_shared_persistent_part_conflict_arbitration",
                "candidate_support_reuse_is_not_itself_an_identity_conflict",
                "recurrence_link_components_are_not_persistent_musical_parts",
                "cross_trajectory_candidate_is_not_cross_part_consensus",
            ],
        },
        "claim_boundary": (
            "This probe identifies conservative YM2612 trajectory and aligned-boundary "
            "targets from source-timed key-on observations. Identical same-channel, same-tick "
            "observations may collapse for analysis; conflicting same-tick observations are "
            "excluded as ambiguous while exact commands remain preserved. Same channel alone is never "
            "sufficient. A target requires unchanged core program, a large local timing "
            "gap, and repeated pitch-rhythm shape across that gap. Shared candidate support "
            "is reported explicitly; adjacent links may compose and are never relabeled as an "
            "identity conflict by this probe. Explicitly composable links may be grouped into "
            "recurrence-link components, but those components are still not persistent musical "
            "parts. Targets remain below persistent-part and phrase promotion until the shared "
            "graph admits and arbitrates them."
        ),
    }


def audit_file(
    path: Path,
    *,
    motif_events: int = 3,
    minimum_gap_ratio: float = 2.0,
    alignment_tolerance_ticks: int = 0,
) -> dict[str, Any]:
    onsets = motif_probe._collect_onsets(path)
    return analyze_onsets(
        onsets,
        source_name=path.name,
        motif_events=motif_events,
        minimum_gap_ratio=minimum_gap_ratio,
        alignment_tolerance_ticks=alignment_tolerance_ticks,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    parser.add_argument("--motif-events", type=int, default=3)
    parser.add_argument("--minimum-gap-ratio", type=float, default=2.0)
    parser.add_argument("--alignment-tolerance-ticks", type=int, default=0)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    result = audit_file(
        args.path,
        motif_events=args.motif_events,
        minimum_gap_ratio=args.minimum_gap_ratio,
        alignment_tolerance_ticks=args.alignment_tolerance_ticks,
    )
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
