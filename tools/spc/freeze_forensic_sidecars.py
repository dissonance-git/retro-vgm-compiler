#!/usr/bin/env python3
"""Freeze label-blind SPC forensic sidecars before creator identity is joined.

This tool deliberately knows nothing about soundtrack names, track titles, or
creator identities. Callers provide opaque cue ids and already-produced forensic
sidecars. The output binds exact sidecar hashes, enforces one runtime/provenance
world, rejects incomplete captures, and computes the same persistent-part motif
similarity used by model/part_motif_attribution_bridge.h.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
from dataclasses import dataclass
from typing import Any


EXPECTED_MODEL = "label-blind SPC forensic feature sidecar"
RHYTHM_ONLY_IDENTITY_CEILING = 0.55
FORBIDDEN_KEYS = {
    "artist",
    "composer",
    "candidate",
    "attribution",
    "game",
    "title",
    "track_title",
    "soundtrack",
    "gd3_artist",
    "id666_artist",
    "external_artist",
}


@dataclass(frozen=True)
class CueSidecar:
    cue_id: str
    sha256: str
    provenance: dict[str, Any]
    profiles: list[dict[str, Any]]
    diagnostics: dict[str, Any]


def _walk_keys(value: Any) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() in FORBIDDEN_KEYS:
                raise ValueError(f"label-bearing key is forbidden in frozen sidecar: {key}")
            _walk_keys(child)
    elif isinstance(value, list):
        for child in value:
            _walk_keys(child)


def _require_int(mapping: dict[str, Any], key: str) -> int:
    value = mapping.get(key)
    if not isinstance(value, int) or isinstance(value, bool):
        raise ValueError(f"{key} must be an integer")
    return value


def _require_number(mapping: dict[str, Any], key: str) -> float:
    value = mapping.get(key)
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        raise ValueError(f"{key} must be numeric")
    value = float(value)
    if not math.isfinite(value):
        raise ValueError(f"{key} must be finite")
    return value


def _validate_profile(profile: dict[str, Any]) -> None:
    rhythm = profile.get("normalized_inter_onset_intervals")
    if not isinstance(rhythm, list) or not rhythm:
        raise ValueError("motif profile requires normalized inter-onset intervals")
    for value in rhythm:
        if not isinstance(value, (int, float)) or not math.isfinite(float(value)):
            raise ValueError("motif rhythm vector must contain finite numbers")

    confidence = _require_number(profile, "evidence_confidence")
    if not 0.0 <= confidence <= 1.0:
        raise ValueError("motif evidence confidence must be in [0, 1]")

    for key in ("interval_octaves", "pitch_contour"):
        value = profile.get(key)
        if value is not None and not isinstance(value, list):
            raise ValueError(f"{key} must be a list or null")

    intervals = profile.get("interval_octaves")
    if intervals is not None:
        for value in intervals:
            if not isinstance(value, (int, float)) or not math.isfinite(float(value)):
                raise ValueError("interval vector must contain finite numbers")

    contour = profile.get("pitch_contour")
    if contour is not None and any(value not in (-1, 0, 1) for value in contour):
        raise ValueError("pitch contour values must be -1, 0, or 1")


def load_sidecar(cue_id: str, path: pathlib.Path) -> CueSidecar:
    if not cue_id or not cue_id.startswith("cue-"):
        raise ValueError("cue ids must be opaque ids beginning with 'cue-'")
    raw = path.read_bytes()
    value = json.loads(raw.decode("utf-8"))
    if not isinstance(value, dict):
        raise ValueError("forensic sidecar must be a JSON object")
    _walk_keys(value)

    if value.get("model") != EXPECTED_MODEL:
        raise ValueError("not a label-blind SPC forensic feature sidecar")

    provenance = value.get("provenance")
    capture = value.get("capture")
    replay = value.get("replay")
    features = value.get("features")
    if not all(isinstance(item, dict) for item in (provenance, capture, replay, features)):
        raise ValueError("sidecar requires provenance, capture, replay, and features objects")

    dropped = _require_int(capture, "dropped_event_count")
    overflowed = _require_int(capture, "overflowed_window_count")
    continuity_breaks = _require_int(replay, "continuity_breaks")
    if dropped != 0 or overflowed != 0 or continuity_breaks != 0:
        raise ValueError("incomplete SPC runtime capture cannot enter the frozen corpus")

    ram_write_count = _require_int(capture, "ram_write_count")
    replayed_writes = _require_int(replay, "ram_writes_applied")
    final_serial = _require_int(replay, "final_ram_write_serial")
    if ram_write_count != replayed_writes or ram_write_count != final_serial:
        raise ValueError("RAM write/replay serial accounting is not lossless")

    profiles = features.get("part_profiles")
    if not isinstance(profiles, list):
        raise ValueError("features.part_profiles must be a list")
    if _require_int(features, "part_profile_count") != len(profiles):
        raise ValueError("part_profile_count does not match emitted profile list")
    if _require_int(features, "emitted_part_count") != len(profiles):
        raise ValueError("emitted_part_count does not match emitted profile list")
    if not profiles:
        raise ValueError("cue produced no admissible persistent-part motif profiles")
    for profile in profiles:
        if not isinstance(profile, dict):
            raise ValueError("part profile must be a JSON object")
        _validate_profile(profile)

    required_provenance = (
        "retro_vgm_compiler_commit",
        "snes_spc_repository",
        "snes_spc_commit",
        "instrumentation_patch_contract",
        "device_tick_rate",
    )
    for key in required_provenance:
        if key not in provenance:
            raise ValueError(f"missing provenance field {key}")

    diagnostics = {
        "ram_write_count": ram_write_count,
        "stored_event_count": _require_int(capture, "stored_event_count"),
        "voice_episode_count": _require_int(features, "voice_episode_count"),
        "eligible_episode_count": _require_int(features, "eligible_episode_count"),
        "part_profile_count": len(profiles),
    }

    return CueSidecar(
        cue_id=cue_id,
        sha256=hashlib.sha256(raw).hexdigest(),
        provenance=dict(provenance),
        profiles=list(profiles),
        diagnostics=diagnostics,
    )


def _bounded_difference(first: list[Any], second: list[Any], weight: float) -> float:
    if len(first) != len(second) or not first:
        return 0.0
    mean = sum(abs(float(a) - float(b)) for a, b in zip(first, second)) / len(first)
    return 1.0 / (1.0 + weight * mean)


def _contour_similarity(first: list[Any], second: list[Any]) -> float:
    if len(first) != len(second) or not first:
        return 0.0
    return sum(a == b for a, b in zip(first, second)) / len(first)


def compare_profiles(first: dict[str, Any], second: dict[str, Any]) -> dict[str, Any]:
    rhythm = _bounded_difference(
        first["normalized_inter_onset_intervals"],
        second["normalized_inter_onset_intervals"],
        1.0,
    )
    evidence_confidence = min(
        float(first["evidence_confidence"]),
        float(second["evidence_confidence"]),
    )

    first_intervals = first.get("interval_octaves")
    second_intervals = second.get("interval_octaves")
    first_contour = first.get("pitch_contour")
    second_contour = second.get("pitch_contour")
    first_semantics = first.get("interval_semantics")
    second_semantics = second.get("interval_semantics")
    pitch_comparable = (
        isinstance(first_intervals, list)
        and isinstance(second_intervals, list)
        and isinstance(first_contour, list)
        and isinstance(second_contour, list)
        and isinstance(first_semantics, str)
        and bool(first_semantics)
        and first_semantics == second_semantics
    )

    interval_similarity = None
    contour_similarity = None
    if pitch_comparable:
        interval_similarity = _bounded_difference(first_intervals, second_intervals, 4.0)
        contour_similarity = _contour_similarity(first_contour, second_contour)
        combined = 0.35 * rhythm + 0.45 * interval_similarity + 0.20 * contour_similarity
        structural_identity = combined
    else:
        combined = rhythm
        structural_identity = min(combined, RHYTHM_ONLY_IDENTITY_CEILING)

    return {
        "rhythm_similarity": rhythm,
        "interval_similarity": interval_similarity,
        "contour_similarity": contour_similarity,
        "combined_similarity": combined,
        "identity_confidence": min(structural_identity, evidence_confidence),
        "pitch_comparable": pitch_comparable,
        "evidence_confidence": evidence_confidence,
    }


def compare_profile_sets(query: list[dict[str, Any]], control: list[dict[str, Any]]) -> dict[str, Any]:
    if not query or not control:
        return {
            "query_profile_count": len(query),
            "control_profile_count": len(control),
            "matched_pair_count": 0,
            "pitch_comparable_pair_count": 0,
            "matched_coverage": 0.0,
            "similarity": 0.0,
            "matches": [],
        }

    candidates: list[tuple[float, float, int, int, dict[str, Any]]] = []
    for qi, q_profile in enumerate(query):
        for ci, c_profile in enumerate(control):
            similarity = compare_profiles(q_profile, c_profile)
            candidates.append((
                -float(similarity["identity_confidence"]),
                -float(similarity["combined_similarity"]),
                qi,
                ci,
                similarity,
            ))
    candidates.sort()

    used_q: set[int] = set()
    used_c: set[int] = set()
    matches: list[dict[str, Any]] = []
    score_sum = 0.0
    pitch_comparable = 0
    for _neg_identity, _neg_combined, qi, ci, similarity in candidates:
        if qi in used_q or ci in used_c:
            continue
        used_q.add(qi)
        used_c.add(ci)
        score_sum += float(similarity["identity_confidence"])
        pitch_comparable += 1 if similarity["pitch_comparable"] else 0
        matches.append({"query_index": qi, "control_index": ci, **similarity})

    denominator = max(len(query), len(control))
    return {
        "query_profile_count": len(query),
        "control_profile_count": len(control),
        "matched_pair_count": len(matches),
        "pitch_comparable_pair_count": pitch_comparable,
        "matched_coverage": len(matches) / denominator,
        "similarity": score_sum / denominator,
        "matches": matches,
    }


def freeze_corpus(cues: list[CueSidecar]) -> dict[str, Any]:
    if len(cues) < 2:
        raise ValueError("frozen corpus requires at least two cues")
    cue_ids = [cue.cue_id for cue in cues]
    if len(set(cue_ids)) != len(cue_ids):
        raise ValueError("opaque cue ids must be unique")

    provenance = cues[0].provenance
    for cue in cues[1:]:
        if cue.provenance != provenance:
            raise ValueError("all frozen sidecars must share exact runtime provenance")

    ordered = sorted(cues, key=lambda cue: cue.cue_id)
    matrix: dict[str, dict[str, float]] = {cue.cue_id: {} for cue in ordered}
    pairs: list[dict[str, Any]] = []
    for left_index, left in enumerate(ordered):
        for right_index in range(left_index, len(ordered)):
            right = ordered[right_index]
            comparison = compare_profile_sets(left.profiles, right.profiles)
            score = float(comparison["similarity"])
            matrix[left.cue_id][right.cue_id] = score
            matrix[right.cue_id][left.cue_id] = score
            if left_index != right_index:
                pairs.append({
                    "left": left.cue_id,
                    "right": right.cue_id,
                    **comparison,
                })

    return {
        "model": "creator-blind SPC forensic corpus freeze",
        "claim_boundary": (
            "Opaque cue identities and creator-blind persistent-part motif geometry only. "
            "Soundtrack, track-title, composer, candidate, and attribution identities are joined later."
        ),
        "similarity_contract": "model/part_motif_attribution_bridge.h",
        "runtime_provenance": provenance,
        "cue_count": len(ordered),
        "cues": [
            {
                "cue_id": cue.cue_id,
                "sidecar_sha256": cue.sha256,
                "diagnostics": cue.diagnostics,
                "part_profiles": cue.profiles,
            }
            for cue in ordered
        ],
        "similarity_matrix": matrix,
        "pairwise": pairs,
    }


def _parse_cue_argument(text: str) -> tuple[str, pathlib.Path]:
    cue_id, separator, path = text.partition("=")
    if not separator or not cue_id or not path:
        raise argparse.ArgumentTypeError("cue must be formatted cue-NNN=path.json")
    return cue_id, pathlib.Path(path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cue", action="append", required=True, type=_parse_cue_argument)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()

    cues = [load_sidecar(cue_id, path) for cue_id, path in args.cue]
    frozen = freeze_corpus(cues)
    args.output.write_text(
        json.dumps(frozen, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
