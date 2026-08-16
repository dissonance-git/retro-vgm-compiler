#!/usr/bin/env python3
"""Label-blind Sonic 3 / Genesis VGM surface-harmony probe.

This is an executable real-corpus pressure test for the harmonic model, not a
replacement for the shared C++ semantic ladder. It mirrors the same YM2612
nominal-frequency and static operator-network periodicity contracts, then asks
what harmonic information is visible before persistent-part, structural-pitch,
phrase, key, function, cadence, and tonal-region promotion has been earned.

The probe deliberately reports two things:

1. surface evidence that can be measured from VGM execution now;
2. promotion gates that remain blocked under the stronger shared model.

No composer/artist tags or curated Sonic 3 attribution labels are read.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import importlib.util
import json
import math
from pathlib import Path
import statistics
import struct
import sys
from typing import Any, Iterable


def _load_base():
    path = Path(__file__).with_name("vgm_creator_feature_audit.py")
    spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


base = _load_base()

YM2612_DIRECT_PERIODICITY_CONFIDENCE = 0.86
YM2612_MISSING_FUNDAMENTAL_CEILING = 0.68
SURFACE_TONAL_CENTER_CEILING = 0.45
DEFAULT_PITCH_TOLERANCE_CENTS = 35.0
DEFAULT_PRESENCE_FLOOR_RATIO = 0.01

CARRIER_MASKS = (0b1000, 0b1000, 0b1000, 0b1000, 0b1010, 0b1110, 0b1110, 0b1111)
CHANNEL_MAP = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}

MODE_TEMPLATES: dict[str, tuple[int, ...]] = {
    "ionian": (0, 2, 4, 5, 7, 9, 11),
    "dorian": (0, 2, 3, 5, 7, 9, 10),
    "phrygian": (0, 1, 3, 5, 7, 8, 10),
    "lydian": (0, 2, 4, 6, 7, 9, 11),
    "mixolydian": (0, 2, 4, 5, 7, 9, 10),
    "aeolian": (0, 2, 3, 5, 7, 8, 10),
    "locrian": (0, 1, 3, 5, 6, 8, 10),
}

TRIAD_TEMPLATES: dict[str, tuple[int, int, int]] = {
    "major": (0, 4, 7),
    "minor": (0, 3, 7),
    "diminished": (0, 3, 6),
    "augmented": (0, 4, 8),
}


def _read_vgm(path: Path) -> bytes:
    data = path.read_bytes()
    return gzip.decompress(data) if path.suffix.lower() == ".vgz" else data


def _ym2612_clock_hz(raw: bytes) -> int:
    if len(raw) < 0x30 or raw[:4] != b"Vgm ":
        raise ValueError("not a VGM stream")
    # VGM reserves the upper clock bits for chip flags. Frequency conversion
    # uses the actual master clock value only.
    return struct.unpack_from("<I", raw, 0x2C)[0] & 0x3FFFFFFF


def _header_total_samples(raw: bytes) -> int:
    if len(raw) < 0x1C:
        return 0
    return struct.unpack_from("<I", raw, 0x18)[0]


def _ym2612_nominal_hz(fnum: int, block: int, clock_hz: int) -> float | None:
    if clock_hz <= 0 or fnum <= 0 or fnum > 0x7FF or block < 0 or block > 7:
        return None
    return clock_hz * fnum * (2.0 ** block) / (144.0 * (2.0 ** 21))


def _operator_half_units(multiple_register: int) -> int:
    value = multiple_register & 0x0F
    return 1 if value == 0 else value * 2


def _performed_pitch(
    state,
    channel: int,
    clock_hz: int,
    *,
    lfo_enabled: bool,
    tolerance_cents: float,
) -> dict[str, Any]:
    onset = state.onset(0, channel)
    if onset is None:
        return {"resolved": False, "reason": "ordinary_channel_pitch_unavailable"}

    algorithm = onset.algorithm
    if algorithm < 0 or algorithm >= len(CARRIER_MASKS):
        return {"resolved": False, "reason": "invalid_algorithm"}

    multiple_registers: list[int] = []
    detune_present = False
    for slot in range(4):
        dt_mult = int(state.operator[channel][slot][0])
        multiple_registers.append(dt_mult & 0x0F)
        detune_present = detune_present or bool((dt_mult >> 4) & 0x07)

    if detune_present:
        return {"resolved": False, "reason": "operator_detune_present"}
    if lfo_enabled and onset.fms != 0:
        return {"resolved": False, "reason": "phase_modulation_active"}

    nominal = _ym2612_nominal_hz(onset.fnum, onset.block, clock_hz)
    if nominal is None:
        return {"resolved": False, "reason": "nominal_pitch_unavailable"}

    half_units = [_operator_half_units(value) for value in multiple_registers]
    gcd_half_units = 0
    for value in half_units:
        gcd_half_units = math.gcd(gcd_half_units, value)
    if gcd_half_units <= 0:
        return {"resolved": False, "reason": "operator_periodicity_unavailable"}

    periodicity_ratio = gcd_half_units / 2.0
    performed_hz = nominal * periodicity_ratio

    carrier_half_units = [
        half_units[index]
        for index in range(4)
        if CARRIER_MASKS[algorithm] & (1 << index)
    ]
    direct_carrier = gcd_half_units in carrier_half_units
    confidence = YM2612_DIRECT_PERIODICITY_CONFIDENCE
    missing_fundamental = not direct_carrier
    if missing_fundamental:
        confidence = min(confidence, YM2612_MISSING_FUNDAMENTAL_CEILING)

    exact_midi = 69.0 + 12.0 * math.log2(performed_hz / 440.0)
    nearest_midi = int(round(exact_midi))
    cents = (exact_midi - nearest_midi) * 100.0
    if abs(cents) > tolerance_cents:
        return {
            "resolved": False,
            "reason": "outside_12tet_projection_tolerance",
            "performed_hz": performed_hz,
            "cents_from_nearest": cents,
            "confidence": confidence,
            "missing_fundamental": missing_fundamental,
        }

    return {
        "resolved": True,
        "performed_hz": performed_hz,
        "midi_note": nearest_midi,
        "pitch_class": nearest_midi % 12,
        "cents_from_nearest": cents,
        "confidence": confidence,
        "missing_fundamental": missing_fundamental,
        "periodicity_ratio": periodicity_ratio,
    }


def _triad_candidates(pitch_classes: Iterable[int]) -> list[dict[str, Any]]:
    observed = {int(value) % 12 for value in pitch_classes}
    if len(observed) != 3:
        return []
    candidates: list[dict[str, Any]] = []
    for root in range(12):
        for quality, offsets in TRIAD_TEMPLATES.items():
            candidate = {(root + offset) % 12 for offset in offsets}
            if candidate == observed:
                candidates.append({"root": root, "quality": quality})
    return candidates


def _triad_state(projected: list[dict[str, Any]]) -> dict[str, Any] | None:
    if len(projected) < 3:
        return None
    unique = {int(item["pitch_class"]) for item in projected}
    candidates = _triad_candidates(unique)
    if not candidates:
        return None
    if len(candidates) != 1:
        # Augmented triads are pitch-class-root ambiguous. Preserve the sonority
        # class but refuse root-dependent transitions.
        qualities = sorted({str(item["quality"]) for item in candidates})
        if qualities == ["augmented"]:
            return {
                "label": "augmented:root_ambiguous",
                "root": None,
                "quality": "augmented",
                "inversion": "unknown",
                "root_ambiguous": True,
            }
        return None

    candidate = candidates[0]
    root = int(candidate["root"])
    quality = str(candidate["quality"])
    lowest = min(projected, key=lambda item: float(item["performed_hz"]))
    lowest_pc = int(lowest["pitch_class"])
    offsets = TRIAD_TEMPLATES[quality]
    relative = (lowest_pc - root) % 12
    inversion = (
        "root_position" if relative == offsets[0]
        else "first_inversion" if relative == offsets[1]
        else "second_inversion" if relative == offsets[2]
        else "unknown"
    )
    return {
        "label": f"{root}:{quality}",
        "root": root,
        "quality": quality,
        "inversion": inversion,
        "root_ambiguous": False,
    }


def _mode_quality(mode: str, degree_index: int) -> str | None:
    scale = MODE_TEMPLATES[mode]
    root = scale[degree_index]
    observed = {
        0,
        (scale[(degree_index + 2) % 7] - root) % 12,
        (scale[(degree_index + 4) % 7] - root) % 12,
    }
    for quality, offsets in TRIAD_TEMPLATES.items():
        if set(offsets) == observed:
            return quality
    return None


def _tonal_candidates(
    pitch_class_duration: dict[int, int],
    root_duration: dict[int, int],
    *,
    mean_pitch_confidence: float,
    presence_floor_ratio: float,
) -> list[dict[str, Any]]:
    total_pitch = float(sum(pitch_class_duration.values()))
    total_root = float(sum(root_duration.values()))
    if total_pitch <= 0.0:
        return []

    floor = total_pitch * presence_floor_ratio
    present = {pc for pc, value in pitch_class_duration.items() if value >= floor}
    candidates: list[dict[str, Any]] = []
    for center in range(12):
        root_support = (root_duration.get(center, 0) / total_root) if total_root > 0.0 else 0.0
        for mode, relative in MODE_TEMPLATES.items():
            members = {(center + value) % 12 for value in relative}
            in_collection = sum(pitch_class_duration.get(pc, 0) for pc in members) / total_pitch
            coverage = len(members & present) / 7.0
            collection_score = in_collection * coverage
            ranking_score = 0.70 * collection_score + 0.30 * root_support
            candidates.append({
                "center_pitch_class": center,
                "mode": mode,
                "pitch_collection_fit": in_collection,
                "template_coverage": coverage,
                "root_duration_support": root_support,
                "ranking_score": ranking_score,
                "evidence_confidence_ceiling": min(
                    SURFACE_TONAL_CENTER_CEILING,
                    mean_pitch_confidence,
                ),
                "key_class_resolved": False,
                "promotion_blocked_by": [
                    "surface_performance_collection_not_structural_pitch_collection",
                    "tonal_center_not_cross_origin_grounded",
                ],
            })
    candidates.sort(
        key=lambda item: (
            -float(item["ranking_score"]),
            -float(item["pitch_collection_fit"]),
            int(item["center_pitch_class"]),
            str(item["mode"]),
        )
    )
    return candidates


def _degree_for_root(center: int, mode: str, root: int) -> int | None:
    relative = (root - center) % 12
    scale = MODE_TEMPLATES[mode]
    for index, value in enumerate(scale):
        if value == relative:
            return index + 1
    return None


def _surface_function_shapes(
    transitions: list[dict[str, Any]],
    best_candidate: dict[str, Any] | None,
) -> dict[str, Any]:
    counts = collections.Counter()
    if best_candidate is None:
        return {
            "candidate_available": False,
            "counts": {},
            "functional_tendency_resolved": False,
            "cadence_resolved": False,
        }

    center = int(best_candidate["center_pitch_class"])
    mode = str(best_candidate["mode"])
    for transition in transitions:
        first_root = transition.get("first_root")
        second_root = transition.get("second_root")
        if first_root is None or second_root is None:
            continue
        first_degree = _degree_for_root(center, mode, int(first_root))
        second_degree = _degree_for_root(center, mode, int(second_root))
        if first_degree is None or second_degree is None:
            continue
        first_quality = str(transition["first_quality"])
        second_quality = str(transition["second_quality"])
        expected_first = _mode_quality(mode, first_degree - 1)
        expected_second = _mode_quality(mode, second_degree - 1)
        if first_quality != expected_first or second_quality != expected_second:
            counts["altered_or_non_diatonic_degree_transition"] += 1
            continue
        if first_degree in (2, 4) and second_degree in (5, 7):
            counts["predominant_to_dominant_shape"] += 1
        if first_degree in (5, 7) and second_degree == 1:
            counts["dominant_to_tonic_shape"] += 1
        if first_degree == 5 and second_degree == 1:
            counts["v_to_i_surface_shape"] += 1

    return {
        "candidate_available": True,
        "provisional_center_pitch_class": center,
        "provisional_mode": mode,
        "counts": {key: counts[key] for key in sorted(counts)},
        "functional_tendency_resolved": False,
        "cadence_resolved": False,
        "promotion_blocked_by": [
            "key_class_not_resolved_under_shared_model",
            "persistent_part_voice_leading_not_grounded",
            "cross_part_phrase_arrival_not_grounded",
        ],
    }


def _commands_grouped(raw: bytes) -> list[tuple[int, list[Any]]]:
    groups: list[tuple[int, list[Any]]] = []
    for command in base.command_stream(raw):
        if groups and groups[-1][0] == command.tick:
            groups[-1][1].append(command)
        else:
            groups.append((command.tick, [command]))
    return groups


def _append_timeline_state(
    timeline: list[dict[str, Any]],
    state: dict[str, Any] | None,
    start_tick: int,
    end_tick: int,
) -> None:
    if end_tick <= start_tick:
        return
    key = None if state is None else state["label"]
    if timeline and timeline[-1]["label"] == key and timeline[-1]["end_tick"] == start_tick:
        timeline[-1]["end_tick"] = end_tick
        timeline[-1]["duration_ticks"] += end_tick - start_tick
        return
    timeline.append({
        "label": key,
        "start_tick": start_tick,
        "end_tick": end_tick,
        "duration_ticks": end_tick - start_tick,
        "root": None if state is None else state["root"],
        "quality": None if state is None else state["quality"],
        "inversion": None if state is None else state["inversion"],
    })


def _transition_records(timeline: list[dict[str, Any]]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    previous: dict[str, Any] | None = None
    for item in timeline:
        if item["label"] is None:
            previous = None
            continue
        if previous is not None and previous["label"] != item["label"]:
            first_root = previous["root"]
            second_root = item["root"]
            motion = None
            if first_root is not None and second_root is not None:
                motion = (int(second_root) - int(first_root)) % 12
            result.append({
                "tick": item["start_tick"],
                "first": previous["label"],
                "second": item["label"],
                "first_root": first_root,
                "second_root": second_root,
                "first_quality": previous["quality"],
                "second_quality": item["quality"],
                "directed_root_motion_semitones": motion,
            })
        previous = item
    return result


def audit_bytes(
    raw: bytes,
    *,
    source_name: str,
    pitch_tolerance_cents: float = DEFAULT_PITCH_TOLERANCE_CENTS,
    presence_floor_ratio: float = DEFAULT_PRESENCE_FLOOR_RATIO,
) -> dict[str, Any]:
    if not math.isfinite(pitch_tolerance_cents) or pitch_tolerance_cents <= 0.0:
        raise ValueError("pitch tolerance must be finite and positive")
    if not math.isfinite(presence_floor_ratio) or not 0.0 < presence_floor_ratio < 1.0:
        raise ValueError("presence-floor ratio must lie in (0, 1)")

    clock_hz = _ym2612_clock_hz(raw)
    if clock_hz <= 0:
        raise ValueError("VGM has no YM2612 master clock")

    state = base.GenesisAuditState()
    active_full_key = [False] * 6
    lfo_enabled = False
    partial_key_events = 0

    pitch_class_duration: collections.Counter[int] = collections.Counter()
    triad_duration: collections.Counter[str] = collections.Counter()
    triad_quality_duration: collections.Counter[str] = collections.Counter()
    root_duration: collections.Counter[int] = collections.Counter()
    inversion_duration: collections.Counter[str] = collections.Counter()
    unresolved_voice_ticks: collections.Counter[str] = collections.Counter()
    timeline: list[dict[str, Any]] = []

    active_voice_ticks = 0
    resolved_voice_ticks = 0
    projected_confidence_ticks = 0.0
    polyphonic_ticks = 0
    triadic_ticks = 0
    missing_fundamental_voice_ticks = 0

    def accumulate(start_tick: int, end_tick: int) -> None:
        nonlocal active_voice_ticks, resolved_voice_ticks, projected_confidence_ticks
        nonlocal polyphonic_ticks, triadic_ticks, missing_fundamental_voice_ticks
        duration = end_tick - start_tick
        if duration <= 0:
            return

        projected: list[dict[str, Any]] = []
        for channel, active in enumerate(active_full_key):
            if not active:
                continue
            active_voice_ticks += duration
            pitch = _performed_pitch(
                state,
                channel,
                clock_hz,
                lfo_enabled=lfo_enabled,
                tolerance_cents=pitch_tolerance_cents,
            )
            if not pitch.get("resolved"):
                unresolved_voice_ticks[str(pitch.get("reason", "unknown"))] += duration
                continue
            projected.append(pitch)
            resolved_voice_ticks += duration
            projected_confidence_ticks += float(pitch["confidence"]) * duration
            if bool(pitch["missing_fundamental"]):
                missing_fundamental_voice_ticks += duration
            pitch_class_duration[int(pitch["pitch_class"])] += duration

        if len(projected) >= 2:
            polyphonic_ticks += duration
        triad = _triad_state(projected)
        _append_timeline_state(timeline, triad, start_tick, end_tick)
        if triad is None:
            return
        triadic_ticks += duration
        triad_duration[str(triad["label"])] += duration
        triad_quality_duration[str(triad["quality"])] += duration
        inversion_duration[str(triad["inversion"])] += duration
        if triad["root"] is not None:
            root_duration[int(triad["root"])] += duration

    groups = _commands_grouped(raw)
    previous_tick = 0
    for tick, commands in groups:
        accumulate(previous_tick, tick)

        for command in commands:
            if command.opcode not in (0x52, 0x53):
                continue
            register, value = command.args
            port = 0 if command.opcode == 0x52 else 1

            if port == 0 and register == 0x22:
                lfo_enabled = bool(value & 0x08)

            state.update(port, register, value)

            if port == 0 and register == 0x28:
                encoded_channel = value & 0x07
                if encoded_channel not in CHANNEL_MAP:
                    continue
                channel = CHANNEL_MAP[encoded_channel]
                key_mask = value & 0xF0
                if key_mask == 0xF0:
                    active_full_key[channel] = True
                elif key_mask == 0x00:
                    active_full_key[channel] = False
                else:
                    # Release-envelope contribution from partial operator masks
                    # is not reconstructed here. Remove the channel from the
                    # clean harmonic surface rather than guess.
                    active_full_key[channel] = False
                    partial_key_events += 1

        previous_tick = tick

    total_samples = max(_header_total_samples(raw), previous_tick)
    accumulate(previous_tick, total_samples)

    mean_pitch_confidence = (
        projected_confidence_ticks / resolved_voice_ticks
        if resolved_voice_ticks > 0 else 0.0
    )
    projection_coverage = (
        resolved_voice_ticks / active_voice_ticks
        if active_voice_ticks > 0 else 0.0
    )
    tonal_candidates = _tonal_candidates(
        dict(pitch_class_duration),
        dict(root_duration),
        mean_pitch_confidence=mean_pitch_confidence,
        presence_floor_ratio=presence_floor_ratio,
    )
    top_candidate = tonal_candidates[0] if tonal_candidates else None
    transitions = _transition_records(timeline)

    root_motion_histogram: collections.Counter[str] = collections.Counter()
    quality_transition_histogram: collections.Counter[str] = collections.Counter()
    for transition in transitions:
        motion = transition["directed_root_motion_semitones"]
        if motion is not None:
            root_motion_histogram[str(motion)] += 1
        quality_transition_histogram[
            f"{transition['first_quality']}>{transition['second_quality']}"
        ] += 1

    function_shapes = _surface_function_shapes(transitions, top_candidate)

    return {
        "file": source_name,
        "ym2612_clock_hz": clock_hz,
        "duration_ticks": total_samples,
        "duration_seconds": total_samples / 44100.0 if total_samples > 0 else 0.0,
        "pitch_role": "performed_hypothesis",
        "surface_scope": "physical-channel FM execution",
        "active_full_key_voice_ticks": active_voice_ticks,
        "resolved_performed_pitch_voice_ticks": resolved_voice_ticks,
        "performed_pitch_projection_coverage": projection_coverage,
        "mean_performed_pitch_confidence": mean_pitch_confidence,
        "missing_fundamental_voice_ticks": missing_fundamental_voice_ticks,
        "partial_key_events_excluded": partial_key_events,
        "unresolved_voice_ticks_by_reason": {
            key: unresolved_voice_ticks[key] for key in sorted(unresolved_voice_ticks)
        },
        "surface_pitch_class_duration_ticks": {
            str(key): pitch_class_duration[key] for key in sorted(pitch_class_duration)
        },
        "surface_polyphonic_ticks": polyphonic_ticks,
        "surface_triadic_ticks": triadic_ticks,
        "surface_triad_duration_ticks": {
            key: triad_duration[key] for key in sorted(triad_duration)
        },
        "surface_triad_quality_duration_ticks": {
            key: triad_quality_duration[key] for key in sorted(triad_quality_duration)
        },
        "surface_root_duration_ticks": {
            str(key): root_duration[key] for key in sorted(root_duration)
        },
        "surface_inversion_duration_ticks": {
            key: inversion_duration[key] for key in sorted(inversion_duration)
        },
        "surface_triad_transitions": transitions,
        "directed_root_motion_histogram": {
            key: root_motion_histogram[key] for key in sorted(root_motion_histogram)
        },
        "quality_transition_histogram": {
            key: quality_transition_histogram[key]
            for key in sorted(quality_transition_histogram)
        },
        "top_surface_tonal_candidates": tonal_candidates[:8],
        "surface_function_shapes": function_shapes,
        "shared_model_promotion": {
            "tonal_center": "ranked_surface_candidate_only",
            "key_class": "blocked",
            "functional_tendency": "blocked",
            "cadence_class": "blocked",
            "tonicization_or_modulation": "blocked",
            "blocked_by": [
                "physical_channel_is_not_persistent_musical_part",
                "surface_pitch_collection_is_not_structural_pitch_collection",
                "no_cross_origin_tonal_center_grounding",
                "no_persistent_part_voice_leading",
                "no_cross_part_phrase_boundary_or_cadential_arrival",
            ],
        },
        "claim_boundary": (
            "This lane mirrors the shared YM2612 performed-pitch hypothesis and observes "
            "surface FM verticalities on physical channels. It ranks tonal and functional "
            "shapes for pressure testing only. Physical channel identity is not persistent "
            "musical-part identity; sounding pitch classes are not automatically structural "
            "harmony; key, function, cadence, modulation, and composer identity remain unearned."
        ),
    }


def audit_file(
    path: Path,
    *,
    pitch_tolerance_cents: float = DEFAULT_PITCH_TOLERANCE_CENTS,
    presence_floor_ratio: float = DEFAULT_PRESENCE_FLOOR_RATIO,
) -> dict[str, Any]:
    return audit_bytes(
        _read_vgm(path),
        source_name=path.name,
        pitch_tolerance_cents=pitch_tolerance_cents,
        presence_floor_ratio=presence_floor_ratio,
    )


def _normalized(counter: dict[str, Any]) -> dict[str, float]:
    values = {str(key): float(value) for key, value in counter.items()}
    total = sum(values.values())
    if total <= 0.0:
        return {key: 0.0 for key in values}
    return {key: value / total for key, value in values.items()}


def harmonic_signature(track: dict[str, Any]) -> dict[str, float]:
    features: dict[str, float] = {}
    for key, value in _normalized(track.get("surface_triad_quality_duration_ticks", {})).items():
        features[f"quality:{key}"] = value
    for key, value in _normalized(track.get("directed_root_motion_histogram", {})).items():
        features[f"root_motion:{key}"] = value
    for key, value in _normalized(track.get("quality_transition_histogram", {})).items():
        features[f"quality_transition:{key}"] = value

    candidates = track.get("top_surface_tonal_candidates")
    best_mode_scores: dict[str, float] = {}
    if isinstance(candidates, list):
        for item in candidates:
            if not isinstance(item, dict):
                continue
            mode = str(item.get("mode"))
            score = float(item.get("ranking_score", 0.0))
            best_mode_scores[mode] = max(best_mode_scores.get(mode, 0.0), score)
    for mode, score in sorted(best_mode_scores.items()):
        features[f"mode_shape:{mode}"] = score

    shape = track.get("surface_function_shapes")
    if isinstance(shape, dict):
        counts = shape.get("counts")
        if isinstance(counts, dict):
            for key, value in _normalized(counts).items():
                features[f"function_shape:{key}"] = value
    return features


def _cosine(lhs: dict[str, float], rhs: dict[str, float]) -> float:
    keys = set(lhs) | set(rhs)
    if not keys:
        return 0.0
    dot = sum(lhs.get(key, 0.0) * rhs.get(key, 0.0) for key in keys)
    left = math.sqrt(sum(value * value for value in lhs.values()))
    right = math.sqrt(sum(value * value for value in rhs.values()))
    if left == 0.0 or right == 0.0:
        return 0.0
    return dot / (left * right)


def _track_id(track: dict[str, Any]) -> str:
    return f"{track['soundtrack_id']}::{track['file']}"


def _neighbors(
    tracks: list[dict[str, Any]],
    limit: int,
    *,
    cross_soundtrack_only: bool,
) -> dict[str, list[dict[str, Any]]]:
    signatures = [harmonic_signature(track) for track in tracks]
    result: dict[str, list[dict[str, Any]]] = {}
    for index, track in enumerate(tracks):
        candidates: list[tuple[float, str, str]] = []
        for other_index, other in enumerate(tracks):
            if index == other_index:
                continue
            if cross_soundtrack_only and track["soundtrack_id"] == other["soundtrack_id"]:
                continue
            score = _cosine(signatures[index], signatures[other_index])
            candidates.append((score, str(other["soundtrack_id"]), str(other["file"])))
        candidates.sort(key=lambda item: (-item[0], item[1], item[2]))
        result[_track_id(track)] = [
            {"soundtrack_id": soundtrack, "file": file_name, "score": score}
            for score, soundtrack, file_name in candidates[:limit]
        ]
    return result


def audit_soundtracks(
    corpora: list[Path],
    *,
    neighbor_count: int = 5,
    cross_soundtrack_only: bool = True,
    pitch_tolerance_cents: float = DEFAULT_PITCH_TOLERANCE_CENTS,
    presence_floor_ratio: float = DEFAULT_PRESENCE_FLOOR_RATIO,
) -> dict[str, Any]:
    if not corpora:
        raise ValueError("at least one corpus directory is required")

    tracks: list[dict[str, Any]] = []
    soundtracks: list[str] = []
    seen: set[str] = set()
    for corpus in corpora:
        soundtrack_id = corpus.name
        if soundtrack_id in seen:
            raise ValueError(f"duplicate soundtrack identity {soundtrack_id!r}")
        seen.add(soundtrack_id)
        soundtracks.append(soundtrack_id)
        paths = sorted(
            path for path in corpus.iterdir()
            if path.is_file() and path.suffix.lower() in (".vgm", ".vgz")
        )
        if not paths:
            raise ValueError(f"no VGM/VGZ files found in {corpus}")
        for path in paths:
            track = audit_file(
                path,
                pitch_tolerance_cents=pitch_tolerance_cents,
                presence_floor_ratio=presence_floor_ratio,
            )
            track["soundtrack_id"] = soundtrack_id
            track["track_id"] = f"{soundtrack_id}::{path.name}"
            tracks.append(track)

    return {
        "model": "blind Genesis VGM surface-harmony pressure test",
        "soundtracks": sorted(soundtracks),
        "track_count": len(tracks),
        "pitch_tolerance_cents": pitch_tolerance_cents,
        "presence_floor_ratio": presence_floor_ratio,
        "cross_soundtrack_only": cross_soundtrack_only,
        "label_policy": (
            "Only corpus-directory soundtrack identity and VGM execution are read. "
            "Composer/artist metadata and curated Sonic 3 attribution labels are excluded."
        ),
        "claim_boundary": (
            "Nearest harmonic neighbors compare surface harmonic signatures only. They are "
            "candidates for deeper persistent-part/phrase analysis, not composer evidence."
        ),
        "tracks": tracks,
        "top_surface_harmonic_neighbors": _neighbors(
            tracks,
            max(0, neighbor_count),
            cross_soundtrack_only=cross_soundtrack_only,
        ),
    }


def _midi_to_frequency(note: int) -> float:
    return 440.0 * (2.0 ** ((note - 69) / 12.0))


def _fnum_block_for_frequency(frequency: float, clock_hz: int) -> tuple[int, int]:
    best: tuple[float, int, int] | None = None
    for block in range(8):
        exact = frequency * 144.0 * (2.0 ** 21) / (clock_hz * (2.0 ** block))
        fnum = int(round(exact))
        if not 1 <= fnum <= 0x7FF:
            continue
        error = abs(_ym2612_nominal_hz(fnum, block, clock_hz) - frequency)
        if best is None or error < best[0]:
            best = (error, fnum, block)
    if best is None:
        raise ValueError("synthetic frequency cannot be represented by YM2612 FNUM/BLOCK")
    return best[1], best[2]


def _synthetic_vgm(*, detune: bool = False) -> bytes:
    clock_hz = 7_670_454
    wait = 4410
    commands = bytearray()

    def ym(reg: int, value: int) -> None:
        commands.extend((0x52, reg & 0xFF, value & 0xFF))

    def pause(samples: int) -> None:
        commands.append(0x61)
        commands.extend(struct.pack("<H", samples))

    # Three ordinary algorithm-0 channels with a direct 1x carrier and no PM.
    for channel in range(3):
        for slot in range(4):
            reg = 0x30 + channel + slot * 4
            multiple = 0x01
            if detune and channel == 0 and slot == 0:
                multiple |= 0x10
            ym(reg, multiple)
        ym(0xB0 + channel, 0x00)
        ym(0xB4 + channel, 0x00)

    chords = [
        (60, 64, 67),  # C
        (65, 69, 72),  # F
        (67, 71, 74),  # G
        (60, 64, 67),  # C return
    ]
    for chord_index, notes in enumerate(chords):
        if chord_index:
            for channel in range(3):
                ym(0x28, channel)
        for channel, note in enumerate(notes):
            fnum, block = _fnum_block_for_frequency(_midi_to_frequency(note), clock_hz)
            ym(0xA0 + channel, fnum & 0xFF)
            ym(0xA4 + channel, ((block & 0x07) << 3) | ((fnum >> 8) & 0x07))
        for channel in range(3):
            ym(0x28, 0xF0 | channel)
        pause(wait)
    for channel in range(3):
        ym(0x28, channel)
    commands.append(0x66)

    header = bytearray(0x40)
    header[:4] = b"Vgm "
    struct.pack_into("<I", header, 8, 0x00000150)
    struct.pack_into("<I", header, 0x18, len(chords) * wait)
    struct.pack_into("<I", header, 0x2C, clock_hz)
    raw = header + commands
    struct.pack_into("<I", raw, 4, len(raw) - 4)
    return bytes(raw)


def _synthetic_self_test() -> dict[str, Any]:
    clean = audit_bytes(_synthetic_vgm(), source_name="synthetic-cfgc.vgm")
    triads = clean["surface_triad_duration_ticks"]
    if not all(key in triads for key in ("0:major", "5:major", "7:major")):
        raise AssertionError("synthetic C/F/G/C triads were not recovered")
    candidates = clean["top_surface_tonal_candidates"]
    if not candidates or candidates[0]["center_pitch_class"] != 0 or candidates[0]["mode"] != "ionian":
        raise AssertionError("synthetic C/F/G/C field did not rank C Ionian first")
    shapes = clean["surface_function_shapes"]["counts"]
    if shapes.get("predominant_to_dominant_shape", 0) < 1:
        raise AssertionError("synthetic IV->V functional shape was not observed")
    if shapes.get("dominant_to_tonic_shape", 0) < 1:
        raise AssertionError("synthetic V->I functional shape was not observed")
    if clean["shared_model_promotion"]["key_class"] != "blocked":
        raise AssertionError("surface probe illegally promoted a key class")

    detuned = audit_bytes(_synthetic_vgm(detune=True), source_name="synthetic-detuned.vgm")
    if detuned["performed_pitch_projection_coverage"] >= clean["performed_pitch_projection_coverage"]:
        raise AssertionError("detune did not reduce performed-pitch projection coverage")

    return {
        "clean_top_candidate": candidates[0],
        "clean_projection_coverage": clean["performed_pitch_projection_coverage"],
        "detuned_projection_coverage": detuned["performed_pitch_projection_coverage"],
        "surface_function_shapes": shapes,
        "promotion_status": clean["shared_model_promotion"],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpora", type=Path, nargs="*")
    parser.add_argument("--json", type=Path)
    parser.add_argument("--neighbors", type=int, default=5)
    parser.add_argument("--include-within-soundtrack", action="store_true")
    parser.add_argument("--pitch-tolerance-cents", type=float, default=DEFAULT_PITCH_TOLERANCE_CENTS)
    parser.add_argument("--presence-floor-ratio", type=float, default=DEFAULT_PRESENCE_FLOOR_RATIO)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        result: dict[str, Any] = {"self_test": _synthetic_self_test()}
    else:
        if not args.corpora:
            parser.error("at least one corpus directory is required unless --self-test is used")
        result = audit_soundtracks(
            args.corpora,
            neighbor_count=args.neighbors,
            cross_soundtrack_only=not args.include_within_soundtrack,
            pitch_tolerance_cents=args.pitch_tolerance_cents,
            presence_floor_ratio=args.presence_floor_ratio,
        )

    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
