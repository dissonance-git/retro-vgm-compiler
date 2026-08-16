#!/usr/bin/env python3
"""Blind Sonic 3 / Genesis mixed YM2612 + SN76489 harmonic probe.

This is the mixed-chip successor to `sonic3_harmonic_probe.py`'s FM-only
surface control.  It deliberately keeps chip identity separate from musical
role:

* YM2612 performed-pitch hypotheses enter the surface using the existing
  operator-network model;
* SN76489 tone channels enter as pitched square-wave sources;
* SN76489 noise never enters harmonic pitch-class inference;
* same-pitch FM/PSG activity is counted as a possible doubling relation rather
  than blindly treated as two independent harmonic voices;
* in the Genesis YM2612+PSG context, PSG tone does not establish bass function
  merely by being the lowest physical pitch. Persistent-part harmonic ownership
  must earn that role later.

The probe also emits creator-blind PSG deployment summaries. These are surface
orchestration measurements, not composer labels. They are intended to pressure-
test cross-soundtrack hypotheses such as whether melodic PSG deployment recurs
in a creator's independently attributed work.

No composer/artist tags or curated Sonic 3 attribution labels are read.
"""

from __future__ import annotations

import argparse
import collections
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
from typing import Any


def _load_sibling(module_name: str, file_name: str):
    path = Path(__file__).with_name(file_name)
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


fm = _load_sibling("sonic3_harmonic_probe_fm_control", "sonic3_harmonic_probe.py")
psg = _load_sibling("genesis_psg_semantics", "genesis_psg_semantics.py")

DEFAULT_PITCH_TOLERANCE_CENTS = fm.DEFAULT_PITCH_TOLERANCE_CENTS
DEFAULT_PRESENCE_FLOOR_RATIO = fm.DEFAULT_PRESENCE_FLOOR_RATIO


def _safe_ratio(numerator: int | float, denominator: int | float) -> float:
    return float(numerator) / float(denominator) if denominator else 0.0


def _mixed_triad_state(projected: list[dict[str, Any]]) -> dict[str, Any] | None:
    if len(projected) < 3:
        return None
    unique = {int(item["pitch_class"]) for item in projected}
    candidates = fm._triad_candidates(unique)
    if not candidates:
        return None
    if len(candidates) != 1:
        qualities = sorted({str(item["quality"]) for item in candidates})
        if qualities == ["augmented"]:
            return {
                "label": "augmented:root_ambiguous",
                "root": None,
                "quality": "augmented",
                "inversion": "unknown",
                "root_ambiguous": True,
                "bass_source": "unresolved",
            }
        return None

    candidate = candidates[0]
    root = int(candidate["root"])
    quality = str(candidate["quality"])
    offsets = fm.TRIAD_TEMPLATES[quality]

    # Musical bass is not identical to lowest hardware pitch. In Genesis mixed
    # FM+PSG arrangements, PSG tone needs independent harmonic-bass ownership
    # before it may define inversion. The surface probe therefore uses only
    # sources that are bass-eligible under the current hardware-context prior.
    bass_candidates = [
        item for item in projected
        if bool(item.get("surface_bass_eligible", True))
    ]
    inversion = "unknown"
    bass_source = "unresolved"
    if bass_candidates:
        lowest = min(bass_candidates, key=lambda item: float(item["performed_hz"]))
        lowest_pc = int(lowest["pitch_class"])
        relative = (lowest_pc - root) % 12
        inversion = (
            "root_position" if relative == offsets[0]
            else "first_inversion" if relative == offsets[1]
            else "second_inversion" if relative == offsets[2]
            else "unknown"
        )
        bass_source = str(lowest.get("device_family", "unknown"))

    return {
        "label": f"{root}:{quality}",
        "root": root,
        "quality": quality,
        "inversion": inversion,
        "root_ambiguous": False,
        "bass_source": bass_source,
    }


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

    ym_clock_hz = fm._ym2612_clock_hz(raw)
    psg_clock_hz = psg.sn76489_clock_hz(raw)
    if ym_clock_hz <= 0:
        raise ValueError("mixed Sonic 3 harmonic probe requires a YM2612 master clock")

    ym_state = fm.base.GenesisAuditState()
    psg_state = psg.SN76489SurfaceState()
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

    active_fm_voice_ticks = 0
    resolved_fm_voice_ticks = 0
    active_psg_tone_voice_ticks = 0
    resolved_psg_tone_voice_ticks = 0
    projected_confidence_ticks = 0.0
    polyphonic_ticks = 0
    triadic_ticks = 0
    missing_fundamental_voice_ticks = 0

    fm_only_pitch_ticks = 0
    psg_only_pitch_ticks = 0
    mixed_pitch_ticks = 0
    psg_same_pitch_doubling_candidate_voice_ticks = 0
    psg_noncoincident_pitch_voice_ticks = 0
    psg_lowest_but_bass_ineligible_ticks = 0

    psg_noise_active_ticks = 0
    psg_noise_control_writes = 0
    psg_noise_attenuation_writes = 0
    psg_noise_onsets = 0

    def accumulate(start_tick: int, end_tick: int) -> None:
        nonlocal active_fm_voice_ticks, resolved_fm_voice_ticks
        nonlocal active_psg_tone_voice_ticks, resolved_psg_tone_voice_ticks
        nonlocal projected_confidence_ticks, polyphonic_ticks, triadic_ticks
        nonlocal missing_fundamental_voice_ticks
        nonlocal fm_only_pitch_ticks, psg_only_pitch_ticks, mixed_pitch_ticks
        nonlocal psg_same_pitch_doubling_candidate_voice_ticks
        nonlocal psg_noncoincident_pitch_voice_ticks, psg_lowest_but_bass_ineligible_ticks
        nonlocal psg_noise_active_ticks

        duration = end_tick - start_tick
        if duration <= 0:
            return

        fm_projected: list[dict[str, Any]] = []
        for channel, active in enumerate(active_full_key):
            if not active:
                continue
            active_fm_voice_ticks += duration
            pitch = fm._performed_pitch(
                ym_state,
                channel,
                ym_clock_hz,
                lfo_enabled=lfo_enabled,
                tolerance_cents=pitch_tolerance_cents,
            )
            if not pitch.get("resolved"):
                unresolved_voice_ticks[f"ym2612:{pitch.get('reason', 'unknown')}"] += duration
                continue
            pitch = dict(pitch)
            pitch.update({
                "device_family": "YM2612",
                "physical_channel": channel,
                "surface_bass_eligible": True,
                "bass_role_prior": "allowed_but_not_established",
            })
            fm_projected.append(pitch)
            resolved_fm_voice_ticks += duration
            projected_confidence_ticks += float(pitch["confidence"]) * duration
            if bool(pitch["missing_fundamental"]):
                missing_fundamental_voice_ticks += duration

        psg_projected: list[dict[str, Any]] = []
        if psg_clock_hz > 0:
            for channel in range(3):
                if not psg_state.tone_active(channel):
                    continue
                active_psg_tone_voice_ticks += duration
                pitch = psg_state.tone_pitch(
                    channel,
                    psg_clock_hz,
                    tolerance_cents=pitch_tolerance_cents,
                    ym2612_music_present=True,
                )
                if not pitch.get("resolved"):
                    unresolved_voice_ticks[f"sn76489:{pitch.get('reason', 'unknown')}"] += duration
                    continue
                psg_projected.append(pitch)
                resolved_psg_tone_voice_ticks += duration
                projected_confidence_ticks += float(pitch["confidence"]) * duration

                coincident = any(
                    psg.pitch_coincident(pitch, fm_pitch, cents=pitch_tolerance_cents)
                    for fm_pitch in fm_projected
                )
                if coincident:
                    psg_same_pitch_doubling_candidate_voice_ticks += duration
                else:
                    psg_noncoincident_pitch_voice_ticks += duration

            if psg_state.noise_active():
                psg_noise_active_ticks += duration

        if fm_projected and psg_projected:
            mixed_pitch_ticks += duration
        elif fm_projected:
            fm_only_pitch_ticks += duration
        elif psg_projected:
            psg_only_pitch_ticks += duration

        if fm_projected and psg_projected:
            lowest_fm = min(float(item["performed_hz"]) for item in fm_projected)
            lowest_psg = min(float(item["performed_hz"]) for item in psg_projected)
            if lowest_psg < lowest_fm:
                psg_lowest_but_bass_ineligible_ticks += duration

        projected = fm_projected + psg_projected

        # Count pitch-class *presence* once per stable interval. Doubling should
        # not overweight a tonal center merely because two hardware sources play
        # the same pitch class.
        for pitch_class in {int(item["pitch_class"]) for item in projected}:
            pitch_class_duration[pitch_class] += duration

        if len(projected) >= 2:
            polyphonic_ticks += duration
        triad = _mixed_triad_state(projected)
        fm._append_timeline_state(timeline, triad, start_tick, end_tick)
        if triad is None:
            return
        triadic_ticks += duration
        triad_duration[str(triad["label"])] += duration
        triad_quality_duration[str(triad["quality"])] += duration
        inversion_duration[str(triad["inversion"])] += duration
        if triad["root"] is not None:
            root_duration[int(triad["root"])] += duration

    groups = fm._commands_grouped(raw)
    previous_tick = 0
    for tick, commands in groups:
        accumulate(previous_tick, tick)

        for command in commands:
            if command.opcode == 0x50:
                before_noise_active = psg_state.noise_active()
                effect = psg_state.write(command.args[0])
                after_noise_active = psg_state.noise_active()
                if effect.channel == 3 and effect.kind == "noise_control":
                    psg_noise_control_writes += 1
                if effect.channel == 3 and effect.kind == "attenuation":
                    psg_noise_attenuation_writes += 1
                if not before_noise_active and after_noise_active:
                    psg_noise_onsets += 1
                continue

            if command.opcode not in (0x52, 0x53):
                continue
            register, value = command.args
            port = 0 if command.opcode == 0x52 else 1

            if port == 0 and register == 0x22:
                lfo_enabled = bool(value & 0x08)

            ym_state.update(port, register, value)

            if port == 0 and register == 0x28:
                encoded_channel = value & 0x07
                if encoded_channel not in fm.CHANNEL_MAP:
                    continue
                channel = fm.CHANNEL_MAP[encoded_channel]
                key_mask = value & 0xF0
                if key_mask == 0xF0:
                    active_full_key[channel] = True
                elif key_mask == 0x00:
                    active_full_key[channel] = False
                else:
                    active_full_key[channel] = False
                    partial_key_events += 1

        previous_tick = tick

    total_samples = max(fm._header_total_samples(raw), previous_tick)
    accumulate(previous_tick, total_samples)

    active_pitch_voice_ticks = active_fm_voice_ticks + active_psg_tone_voice_ticks
    resolved_pitch_voice_ticks = resolved_fm_voice_ticks + resolved_psg_tone_voice_ticks
    mean_pitch_confidence = (
        projected_confidence_ticks / resolved_pitch_voice_ticks
        if resolved_pitch_voice_ticks > 0 else 0.0
    )
    projection_coverage = (
        resolved_pitch_voice_ticks / active_pitch_voice_ticks
        if active_pitch_voice_ticks > 0 else 0.0
    )

    psg_deployment_signature = {
        "psg_tone_channel_equivalent_density": _safe_ratio(
            resolved_psg_tone_voice_ticks, total_samples),
        "fm_channel_equivalent_density": _safe_ratio(
            resolved_fm_voice_ticks, total_samples),
        "psg_share_of_resolved_pitched_voice_time": _safe_ratio(
            resolved_psg_tone_voice_ticks, resolved_pitch_voice_ticks),
        "psg_same_pitch_fm_shadow_fraction": _safe_ratio(
            psg_same_pitch_doubling_candidate_voice_ticks,
            resolved_psg_tone_voice_ticks,
        ),
        "psg_noncoincident_pitched_fraction": _safe_ratio(
            psg_noncoincident_pitch_voice_ticks,
            resolved_psg_tone_voice_ticks,
        ),
        "psg_noise_activity_ratio": _safe_ratio(psg_noise_active_ticks, total_samples),
        "mixed_fm_psg_overlap_ratio": _safe_ratio(mixed_pitch_ticks, total_samples),
    }

    tonal_candidates = fm._tonal_candidates(
        dict(pitch_class_duration),
        dict(root_duration),
        mean_pitch_confidence=mean_pitch_confidence,
        presence_floor_ratio=presence_floor_ratio,
    )
    top_candidate = tonal_candidates[0] if tonal_candidates else None
    transitions = fm._transition_records(timeline)

    root_motion_histogram: collections.Counter[str] = collections.Counter()
    quality_transition_histogram: collections.Counter[str] = collections.Counter()
    for transition in transitions:
        motion = transition["directed_root_motion_semitones"]
        if motion is not None:
            root_motion_histogram[str(motion)] += 1
        quality_transition_histogram[
            f"{transition['first_quality']}>{transition['second_quality']}"
        ] += 1

    function_shapes = fm._surface_function_shapes(transitions, top_candidate)
    noise_surface = psg_state.noise_surface(ym2612_music_present=True)

    return {
        "file": source_name,
        "ym2612_clock_hz": ym_clock_hz,
        "sn76489_clock_hz": psg_clock_hz,
        "duration_ticks": total_samples,
        "duration_seconds": total_samples / 44100.0 if total_samples > 0 else 0.0,
        "pitch_role": "performed_hypothesis",
        "surface_scope": "physical-channel YM2612 + SN76489 tone execution",

        # Keep the old FM metric name as a compatibility alias while exposing
        # the now-correct mixed-source accounting explicitly.
        "active_full_key_voice_ticks": active_fm_voice_ticks,
        "active_fm_full_key_voice_ticks": active_fm_voice_ticks,
        "resolved_fm_performed_pitch_voice_ticks": resolved_fm_voice_ticks,
        "active_psg_tone_voice_ticks": active_psg_tone_voice_ticks,
        "resolved_psg_tone_pitch_voice_ticks": resolved_psg_tone_voice_ticks,
        "active_pitch_voice_ticks": active_pitch_voice_ticks,
        "resolved_performed_pitch_voice_ticks": resolved_pitch_voice_ticks,
        "performed_pitch_projection_coverage": projection_coverage,
        "mean_performed_pitch_confidence": mean_pitch_confidence,
        "missing_fundamental_voice_ticks": missing_fundamental_voice_ticks,
        "partial_key_events_excluded": partial_key_events,
        "unresolved_voice_ticks_by_reason": {
            key: unresolved_voice_ticks[key] for key in sorted(unresolved_voice_ticks)
        },

        "surface_device_mix_ticks": {
            "fm_only": fm_only_pitch_ticks,
            "psg_only": psg_only_pitch_ticks,
            "mixed_fm_psg": mixed_pitch_ticks,
        },
        "psg_surface_deployment_signature": psg_deployment_signature,
        "psg_role_evidence": {
            "same_pitch_fm_doubling_candidate_voice_ticks":
                psg_same_pitch_doubling_candidate_voice_ticks,
            "noncoincident_pitched_voice_ticks": psg_noncoincident_pitch_voice_ticks,
            "lowest_pitch_but_bass_ineligible_ticks": psg_lowest_but_bass_ineligible_ticks,
            "mixed_context_bass_policy":
                "PSG tone does not establish bass_foundation from register alone when YM2612 music is present; strong persistent-part harmonic-bass ownership would be required to overturn the prior.",
            "doubling_policy":
                "same-pitch FM/PSG coincidence is a doubling candidate only; onset lag, contour, duration, phrase context, and persistent-part correspondence remain unresolved here.",
        },
        "psg_noise_surface": {
            **noise_surface,
            "active_ticks": psg_noise_active_ticks,
            "noise_control_writes": psg_noise_control_writes,
            "attenuation_writes": psg_noise_attenuation_writes,
            "observed_onsets": psg_noise_onsets,
            "role_policy":
                "noise contributes percussion/texture evidence only; hi-hat/snare/kick/accent identity requires temporal or authored-source grounding.",
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
            "psg_part_role": "candidate_only",
            "psg_noise_percussion_identity": "candidate_only",
            "psg_creator_grammar": "blocked_until_persistent_part_role_and_cross_soundtrack_grounding",
            "blocked_by": [
                "physical_channel_is_not_persistent_musical_part",
                "fm_psg_pitch_coincidence_is_not_yet_doubling_identity",
                "psg_noise_channel_is_not_a_named_drum_role",
                "surface_pitch_collection_is_not_structural_pitch_collection",
                "no_cross_origin_tonal_center_grounding",
                "no_persistent_part_voice_leading",
                "no_cross_part_phrase_boundary_or_cadential_arrival",
            ],
        },
        "claim_boundary": (
            "This lane observes both YM2612 and SN76489 pitched execution. PSG tone may be "
            "an independent musical line, a doubling/shadow, accompaniment, or ornament; chip "
            "identity alone does not decide the role. PSG deployment ratios are creator-blind "
            "surface measurements that may motivate a composer-grammar test only after persistent "
            "part/role grounding and independent soundtrack evidence. PSG noise is excluded from "
            "harmonic pitch and contributes only percussion/texture evidence until timing/source "
            "context earns a more specific role. Key, function, cadence, modulation, and creator "
            "identity remain unearned."
        ),
    }


def audit_file(
    path: Path,
    *,
    pitch_tolerance_cents: float = DEFAULT_PITCH_TOLERANCE_CENTS,
    presence_floor_ratio: float = DEFAULT_PRESENCE_FLOOR_RATIO,
) -> dict[str, Any]:
    return audit_bytes(
        fm._read_vgm(path),
        source_name=path.name,
        pitch_tolerance_cents=pitch_tolerance_cents,
        presence_floor_ratio=presence_floor_ratio,
    )


def harmonic_signature(track: dict[str, Any]) -> dict[str, float]:
    return fm.harmonic_signature(track)


def _cosine(lhs: dict[str, float], rhs: dict[str, float]) -> float:
    return fm._cosine(lhs, rhs)


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


def _soundtrack_psg_surface_summary(tracks: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = collections.defaultdict(list)
    for track in tracks:
        grouped[str(track["soundtrack_id"])].append(track)

    result: dict[str, dict[str, Any]] = {}
    for soundtrack_id, items in sorted(grouped.items()):
        total_duration = sum(int(item["duration_ticks"]) for item in items)
        resolved_psg = sum(int(item["resolved_psg_tone_pitch_voice_ticks"]) for item in items)
        resolved_fm = sum(int(item["resolved_fm_performed_pitch_voice_ticks"]) for item in items)
        doubling = sum(
            int(item["psg_role_evidence"]["same_pitch_fm_doubling_candidate_voice_ticks"])
            for item in items
        )
        noncoincident = sum(
            int(item["psg_role_evidence"]["noncoincident_pitched_voice_ticks"])
            for item in items
        )
        noise_ticks = sum(int(item["psg_noise_surface"]["active_ticks"]) for item in items)
        mixed_ticks = sum(int(item["surface_device_mix_ticks"]["mixed_fm_psg"]) for item in items)
        pitched_total = resolved_psg + resolved_fm

        result[soundtrack_id] = {
            "track_count": len(items),
            "tracks_with_psg_tone": sum(
                int(item["resolved_psg_tone_pitch_voice_ticks"] > 0) for item in items),
            "tracks_with_psg_noncoincident_pitch": sum(
                int(item["psg_role_evidence"]["noncoincident_pitched_voice_ticks"] > 0)
                for item in items),
            "tracks_with_psg_fm_shadow_candidate": sum(
                int(item["psg_role_evidence"]["same_pitch_fm_doubling_candidate_voice_ticks"] > 0)
                for item in items),
            "tracks_with_psg_noise": sum(
                int(item["psg_noise_surface"]["active_ticks"] > 0) for item in items),
            "weighted_surface_signature": {
                "psg_tone_channel_equivalent_density": _safe_ratio(resolved_psg, total_duration),
                "fm_channel_equivalent_density": _safe_ratio(resolved_fm, total_duration),
                "psg_share_of_resolved_pitched_voice_time": _safe_ratio(psg, pitched_total)
                if (psg := resolved_psg) else 0.0,
                "psg_same_pitch_fm_shadow_fraction": _safe_ratio(doubling, resolved_psg),
                "psg_noncoincident_pitched_fraction": _safe_ratio(noncoincident, resolved_psg),
                "psg_noise_activity_ratio": _safe_ratio(noise_ticks, total_duration),
                "mixed_fm_psg_overlap_ratio": _safe_ratio(mixed_ticks, total_duration),
            },
            "interpretation_boundary": (
                "Noncoincident PSG pitch is not yet melodic-part truth, and same-pitch overlap is not yet doubling truth. "
                "These soundtrack-level ratios remain blind surface evidence until persistent-part and role inference."
            ),
        }
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
        "model": "blind Genesis VGM mixed YM2612+SN76489 surface-harmony pressure test",
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
            "Nearest harmonic neighbors compare mixed-chip surface harmonic signatures only. "
            "PSG deployment summaries remain creator-blind and cannot establish a composer or arranger role without later role-scoped evidence."
        ),
        "tracks": tracks,
        "soundtrack_psg_surface_summary": _soundtrack_psg_surface_summary(tracks),
        "top_surface_harmonic_neighbors": _neighbors(
            tracks,
            max(0, neighbor_count),
            cross_soundtrack_only=cross_soundtrack_only,
        ),
    }


def _midi_to_psg_period(note: int, clock_hz: int) -> int:
    frequency = fm._midi_to_frequency(note)
    period = int(round(clock_hz / (32.0 * frequency)))
    if not 1 <= period <= 0x03FF:
        raise ValueError("synthetic pitch cannot be represented by SN76489 tone period")
    return period


def _synthetic_mixed_vgm(*, with_noise: bool = True) -> bytes:
    ym_clock_hz = 7_670_454
    psg_clock_hz = 3_579_545
    wait = 4410
    commands = bytearray()

    def ym(reg: int, value: int) -> None:
        commands.extend((0x52, reg & 0xFF, value & 0xFF))

    def psg_write(value: int) -> None:
        commands.extend((0x50, value & 0xFF))

    def pause(samples: int) -> None:
        commands.append(0x61)
        commands.extend(struct.pack("<H", samples))

    for channel in range(3):
        for slot in range(4):
            ym(0x30 + channel + slot * 4, 0x01)
        ym(0xB0 + channel, 0x00)
        ym(0xB4 + channel, 0x00)

    chords = [
        (60, 64, 67),
        (65, 69, 72),
        (67, 71, 74),
        (60, 64, 67),
    ]

    if with_noise:
        psg_write(0xE7)
        psg_write(0xF2)

    for chord_index, notes in enumerate(chords):
        if chord_index:
            for channel in range(3):
                ym(0x28, channel)

        for channel, note in enumerate(notes):
            fnum, block = fm._fnum_block_for_frequency(
                fm._midi_to_frequency(note),
                ym_clock_hz,
            )
            ym(0xA0 + channel, fnum & 0xFF)
            ym(0xA4 + channel, ((block & 0x07) << 3) | ((fnum >> 8) & 0x07))
        for channel in range(3):
            ym(0x28, 0xF0 | channel)

        # PSG0 shadows FM0 at the same pitch. PSG1 supplies another pitched
        # source on alternating chords. Whether that second source is a true
        # independent part remains deliberately unresolved at this surface stage.
        period0 = _midi_to_psg_period(notes[0], psg_clock_hz)
        psg_write(0x80 | (period0 & 0x0F))
        psg_write((period0 >> 4) & 0x3F)
        psg_write(0x90)

        if chord_index % 2 == 0:
            period1 = _midi_to_psg_period(notes[1], psg_clock_hz)
            psg_write(0xA0 | (period1 & 0x0F))
            psg_write((period1 >> 4) & 0x3F)
            psg_write(0xB4)
        else:
            psg_write(0xBF)

        pause(wait)

    for channel in range(3):
        ym(0x28, channel)
    psg_write(0x9F)
    psg_write(0xBF)
    if with_noise:
        psg_write(0xFF)
    commands.append(0x66)

    header = bytearray(0x40)
    header[:4] = b"Vgm "
    struct.pack_into("<I", header, 8, 0x00000150)
    struct.pack_into("<I", header, 0x0C, psg_clock_hz)
    struct.pack_into("<I", header, 0x18, len(chords) * wait)
    struct.pack_into("<I", header, 0x2C, ym_clock_hz)
    raw = header + commands
    struct.pack_into("<I", raw, 4, len(raw) - 4)
    return bytes(raw)


def _synthetic_self_test() -> dict[str, Any]:
    mixed = audit_bytes(_synthetic_mixed_vgm(), source_name="synthetic-mixed-cfgc.vgm")
    triads = mixed["surface_triad_duration_ticks"]
    if not all(key in triads for key in ("0:major", "5:major", "7:major")):
        raise AssertionError("mixed FM+PSG C/F/G/C triads were not recovered")
    if mixed["active_psg_tone_voice_ticks"] <= 0:
        raise AssertionError("synthetic mixed stream did not expose PSG tone activity")
    if mixed["psg_role_evidence"]["same_pitch_fm_doubling_candidate_voice_ticks"] <= 0:
        raise AssertionError("synthetic FM/PSG pitch doubling was not detected")
    if mixed["psg_noise_surface"]["active_ticks"] <= 0:
        raise AssertionError("synthetic PSG noise activity was not observed")
    if mixed["psg_noise_surface"]["hi_hat_established"]:
        raise AssertionError("noise channel identity illegally became a hi-hat label")
    if mixed["shared_model_promotion"]["key_class"] != "blocked":
        raise AssertionError("mixed surface probe illegally promoted a key class")
    return {
        "top_candidate": mixed["top_surface_tonal_candidates"][0],
        "psg_deployment_signature": mixed["psg_surface_deployment_signature"],
        "psg_role_evidence": mixed["psg_role_evidence"],
        "psg_noise_surface": mixed["psg_noise_surface"],
        "promotion_status": mixed["shared_model_promotion"],
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
