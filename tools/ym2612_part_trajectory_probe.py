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


def _validate_events(events: list[Any]) -> None:
    if not events:
        return
    previous_tick: int | None = None
    for event in events:
        tick = int(getattr(event, "tick"))
        channel = int(getattr(event, "channel"))
        if channel < 0 or channel > 5:
            raise ValueError("YM2612 channel must lie in [0, 5]")
        if float(getattr(event, "frequency_measure")) <= 0.0:
            raise ValueError("YM2612 trajectory observation requires positive pitch measure")
        _patch_core(event)
        if previous_tick is not None and tick <= previous_tick:
            raise ValueError("YM2612 trajectory observations must be strictly ordered")
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
                support_ticks = [int(item.tick) for item in left + right]
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
    observations = list(onsets)
    candidates = discover_trajectory_candidates(
        observations,
        motif_events=motif_events,
        minimum_gap_ratio=minimum_gap_ratio,
    )
    aligned = align_cross_trajectory_boundaries(
        candidates,
        alignment_tolerance_ticks=alignment_tolerance_ticks,
    )
    return {
        "source": source_name,
        "chip_scope": "YM2612",
        "platform_identity_consulted": False,
        "observation_count": len(observations),
        "trajectory_candidate_count": len(candidates),
        "cross_trajectory_boundary_candidate_count": len(aligned),
        "trajectory_candidates": candidates,
        "cross_trajectory_boundary_candidates": aligned,
        "shared_model_promotion": {
            "persistent_part_identity": "blocked",
            "cross_part_phrase_boundary": "blocked",
            "phrase_role": "blocked",
            "blocked_by": [
                "probe_does_not_materialize_shared_graph_voice_episodes",
                "probe_does_not_run_shared_persistent_part_conflict_arbitration",
                "cross_trajectory_candidate_is_not_cross_part_consensus",
            ],
        },
        "claim_boundary": (
            "This probe identifies conservative YM2612 trajectory and aligned-boundary "
            "targets from source-timed key-on observations. Same channel alone is never "
            "sufficient. A target requires unchanged core program, a large local timing "
            "gap, and repeated pitch-rhythm shape across that gap. Targets remain below "
            "persistent-part and phrase promotion until the shared graph admits them."
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
