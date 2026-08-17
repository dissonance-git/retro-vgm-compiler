#!/usr/bin/env python3
"""Freeze creator-blind persistent-part motif profiles before identity is joined.

SPC forensic sidecars remain a first-class input, but the frozen comparison
surface is representation-neutral. Each representation family must share one
exact provenance world internally; different source families may coexist in the
same opaque cue matrix. Creator, soundtrack, and track identities are forbidden
until the downstream reveal step.
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
PROFILE_BUNDLE_MODEL = "creator-blind persistent-part motif profile bundle"
FREEZE_MODEL = "creator-blind persistent-part motif corpus freeze"
LEGACY_FREEZE_MODEL = "creator-blind SPC forensic corpus freeze"
SPC_REPRESENTATION = "spc_forensic_persistent_part_motif"
RHYTHM_ONLY_IDENTITY_CEILING = 0.55
FORBIDDEN_KEYS = {
    "artist", "composer", "candidate", "attribution", "game", "title",
    "track_title", "soundtrack", "gd3_artist", "id666_artist", "external_artist",
}

@dataclass(frozen=True)
class CueSidecar:
    cue_id: str
    sha256: str
    provenance: dict[str, Any]
    profiles: list[dict[str, Any]]
    diagnostics: dict[str, Any]
    representation: str = SPC_REPRESENTATION

def _walk_keys(value: Any) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() in FORBIDDEN_KEYS:
                raise ValueError(f"label-bearing key is forbidden in frozen motif evidence: {key}")
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
    semantics = profile.get("interval_semantics")
    if intervals is not None and (not isinstance(semantics, str) or not semantics):
        raise ValueError("pitched motif profile requires non-empty interval semantics")

def _validate_cue_id(cue_id: str) -> None:
    if not cue_id or not cue_id.startswith("cue-"):
        raise ValueError("cue ids must be opaque ids beginning with 'cue-'")

def load_sidecar(cue_id: str, path: pathlib.Path, *, require_profiles: bool = True) -> CueSidecar:
    _validate_cue_id(cue_id)
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
    if require_profiles and not profiles:
        raise ValueError("cue produced no admissible persistent-part motif profiles")
    for profile in profiles:
        if not isinstance(profile, dict):
            raise ValueError("part profile must be a JSON object")
        _validate_profile(profile)
    required_provenance = (
        "retro_vgm_compiler_commit", "snes_spc_repository", "snes_spc_commit",
        "instrumentation_patch_contract", "device_tick_rate",
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
    return CueSidecar(cue_id, hashlib.sha256(raw).hexdigest(), dict(provenance), list(profiles), diagnostics, SPC_REPRESENTATION)

def load_profile_bundle(cue_id: str, path: pathlib.Path) -> CueSidecar:
    _validate_cue_id(cue_id)
    raw = path.read_bytes()
    value = json.loads(raw.decode("utf-8"))
    if not isinstance(value, dict):
        raise ValueError("motif profile bundle must be a JSON object")
    _walk_keys(value)
    if value.get("model") != PROFILE_BUNDLE_MODEL:
        raise ValueError("not a creator-blind persistent-part motif profile bundle")
    representation = value.get("representation")
    provenance = value.get("provenance")
    profiles = value.get("part_profiles")
    diagnostics = value.get("diagnostics", {})
    if not isinstance(representation, str) or not representation:
        raise ValueError("motif profile bundle requires a representation family")
    if not isinstance(provenance, dict) or not provenance:
        raise ValueError("motif profile bundle requires provenance")
    if not isinstance(profiles, list) or not profiles:
        raise ValueError("motif profile bundle requires at least one part profile")
    if not isinstance(diagnostics, dict):
        raise ValueError("motif profile bundle diagnostics must be an object")
    for profile in profiles:
        if not isinstance(profile, dict):
            raise ValueError("part profile must be a JSON object")
        _validate_profile(profile)
    return CueSidecar(cue_id, hashlib.sha256(raw).hexdigest(), dict(provenance), list(profiles), dict(diagnostics), representation)

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
    rhythm = _bounded_difference(first["normalized_inter_onset_intervals"], second["normalized_inter_onset_intervals"], 1.0)
    evidence_confidence = min(float(first["evidence_confidence"]), float(second["evidence_confidence"]))
    first_intervals = first.get("interval_octaves")
    second_intervals = second.get("interval_octaves")
    first_contour = first.get("pitch_contour")
    second_contour = second.get("pitch_contour")
    first_semantics = first.get("interval_semantics")
    second_semantics = second.get("interval_semantics")
    pitch_comparable = (
        isinstance(first_intervals, list) and isinstance(second_intervals, list)
        and isinstance(first_contour, list) and isinstance(second_contour, list)
        and isinstance(first_semantics, str) and bool(first_semantics)
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

def _maximum_weight_assignment(weights: list[list[float]]) -> list[tuple[int, int]]:
    """Return a maximum-weight one-to-one assignment for a rectangular matrix."""
    if not weights or not weights[0]:
        return []
    row_count = len(weights)
    column_count = len(weights[0])
    if any(len(row) != column_count for row in weights):
        raise ValueError("assignment weight matrix must be rectangular")
    for row in weights:
        for weight in row:
            if not math.isfinite(float(weight)) or float(weight) < 0.0:
                raise ValueError("assignment weights must be finite and nonnegative")

    transposed = row_count > column_count
    if transposed:
        matrix = [[weights[row][column] for row in range(row_count)] for column in range(column_count)]
    else:
        matrix = weights
    n = len(matrix)
    m = len(matrix[0])

    u = [0.0] * (n + 1)
    v = [0.0] * (m + 1)
    p = [0] * (m + 1)
    way = [0] * (m + 1)
    for i in range(1, n + 1):
        p[0] = i
        j0 = 0
        minv = [math.inf] * (m + 1)
        used = [False] * (m + 1)
        while True:
            used[j0] = True
            i0 = p[j0]
            delta = math.inf
            j1 = 0
            for j in range(1, m + 1):
                if used[j]:
                    continue
                cur = -float(matrix[i0 - 1][j - 1]) - u[i0] - v[j]
                if cur < minv[j]:
                    minv[j] = cur
                    way[j] = j0
                if minv[j] < delta:
                    delta = minv[j]
                    j1 = j
            for j in range(m + 1):
                if used[j]:
                    u[p[j]] += delta
                    v[j] -= delta
                else:
                    minv[j] -= delta
            j0 = j1
            if p[j0] == 0:
                break
        while True:
            j1 = way[j0]
            p[j0] = p[j1]
            j0 = j1
            if j0 == 0:
                break

    pairs: list[tuple[int, int]] = []
    for column in range(1, m + 1):
        if p[column] == 0:
            continue
        row = p[column] - 1
        col = column - 1
        pairs.append((col, row) if transposed else (row, col))
    pairs.sort()
    return pairs


def _profile_assignment_key(profile: dict[str, Any]) -> tuple[Any, ...]:
    """Canonical musical key for assignment tie-breaking.

    Only fields consumed by ``compare_profiles`` participate. Native pitch basis,
    representation, provenance, artifact hashes, source-node ids, and profile
    enumeration are deliberately excluded.
    """
    intervals = profile.get("interval_octaves")
    contour = profile.get("pitch_contour")
    semantics = profile.get("interval_semantics")
    return (
        tuple(float(value) for value in profile["normalized_inter_onset_intervals"]),
        intervals is not None,
        tuple(float(value) for value in intervals) if isinstance(intervals, list) else (),
        contour is not None,
        tuple(int(value) for value in contour) if isinstance(contour, list) else (),
        semantics if isinstance(semantics, str) else "",
        float(profile["evidence_confidence"]),
    )


def compare_profile_sets(query: list[dict[str, Any]], control: list[dict[str, Any]]) -> dict[str, Any]:
    if not query or not control:
        return {"query_profile_count":len(query),"control_profile_count":len(control),"matched_pair_count":0,"pitch_comparable_pair_count":0,"matched_coverage":0.0,"similarity":0.0,"matches":[]}

    # Hungarian assignment is globally optimal, but an optimum may not be unique.
    # Canonicalize by scored musical content first so tied optima do not let
    # profile enumeration alter diagnostics such as pitch-comparable pair count.
    query_order = sorted(range(len(query)), key=lambda index: _profile_assignment_key(query[index]))
    control_order = sorted(range(len(control)), key=lambda index: _profile_assignment_key(control[index]))
    canonical_query = [query[index] for index in query_order]
    canonical_control = [control[index] for index in control_order]

    similarities = [
        [compare_profiles(q_profile, c_profile) for c_profile in canonical_control]
        for q_profile in canonical_query
    ]
    weights = [
        [float(item["identity_confidence"]) for item in row]
        for row in similarities
    ]
    assignment = _maximum_weight_assignment(weights)

    matches=[]; score_sum=0.0; pitch_comparable=0
    for canonical_qi, canonical_ci in assignment:
        similarity=similarities[canonical_qi][canonical_ci]
        score_sum += float(similarity["identity_confidence"])
        pitch_comparable += 1 if similarity["pitch_comparable"] else 0
        matches.append({
            "query_index": query_order[canonical_qi],
            "control_index": control_order[canonical_ci],
            **similarity,
        })
    matches.sort(key=lambda item: (item["query_index"], item["control_index"]))
    denominator=max(len(query),len(control))
    return {"query_profile_count":len(query),"control_profile_count":len(control),"matched_pair_count":len(matches),"pitch_comparable_pair_count":pitch_comparable,"matched_coverage":len(matches)/denominator,"similarity":score_sum/denominator,"matches":matches}

def freeze_corpus(cues: list[CueSidecar]) -> dict[str, Any]:
    if len(cues)<2: raise ValueError("frozen corpus requires at least two cues")
    cue_ids=[cue.cue_id for cue in cues]
    if len(set(cue_ids)) != len(cue_ids): raise ValueError("opaque cue ids must be unique")
    provenance_worlds={}
    for cue in cues:
        if not cue.representation: raise ValueError("frozen cue requires a representation family")
        existing=provenance_worlds.get(cue.representation)
        if existing is None: provenance_worlds[cue.representation]=cue.provenance
        elif existing != cue.provenance: raise ValueError("all cues within one representation family must share exact provenance")
    ordered=sorted(cues,key=lambda cue:cue.cue_id)
    matrix={cue.cue_id:{} for cue in ordered}; pairs=[]
    for li,left in enumerate(ordered):
        for ri in range(li,len(ordered)):
            right=ordered[ri]; comparison=compare_profile_sets(left.profiles,right.profiles); score=float(comparison["similarity"])
            matrix[left.cue_id][right.cue_id]=score; matrix[right.cue_id][left.cue_id]=score
            if li != ri: pairs.append({"left":left.cue_id,"right":right.cue_id,**comparison})
    return {
        "model":FREEZE_MODEL,
        "claim_boundary":"Opaque cue ids and creator-blind persistent-part motif geometry only. Native representations and provenance are retained for audit but never scored. Identity labels are joined later.",
        "similarity_contract":"model/part_motif_attribution_bridge.h",
        "provenance_worlds":{key:provenance_worlds[key] for key in sorted(provenance_worlds)},
        "cue_count":len(ordered),
        "cues":[{"cue_id":cue.cue_id,"artifact_sha256":cue.sha256,"representation":cue.representation,"diagnostics":cue.diagnostics,"part_profiles":cue.profiles} for cue in ordered],
        "similarity_matrix":matrix,
        "pairwise":pairs,
    }

def _parse_cue_argument(text: str) -> tuple[str,pathlib.Path]:
    cue_id,separator,path=text.partition("=")
    if not separator or not cue_id or not path: raise argparse.ArgumentTypeError("cue must be formatted cue-NNN=path.json")
    return cue_id,pathlib.Path(path)

def main()->None:
    parser=argparse.ArgumentParser()
    parser.add_argument("--cue",action="append",type=_parse_cue_argument,default=[])
    parser.add_argument("--profile-bundle",action="append",type=_parse_cue_argument,default=[])
    parser.add_argument("--output",type=pathlib.Path,required=True)
    args=parser.parse_args()
    cues=[load_sidecar(cue_id,path) for cue_id,path in args.cue]
    cues.extend(load_profile_bundle(cue_id,path) for cue_id,path in args.profile_bundle)
    if not cues: parser.error("at least one --cue or --profile-bundle is required")
    frozen=freeze_corpus(cues)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(frozen,indent=2,sort_keys=True)+"\n",encoding="utf-8")

if __name__ == "__main__":
    main()
