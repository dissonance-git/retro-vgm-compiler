#!/usr/bin/env python3
"""Validate one creator-blind HES forensic sidecar for reusable caching."""
from __future__ import annotations

import json
import math
import pathlib
import re
from typing import Any

EXPECTED_MODEL = "creator-blind HES forensic register sidecar"
EXPECTED_SCHEMA_VERSION = 1
EXPECTED_LIBGME_REPOSITORY = "https://github.com/libgme/game-music-emu"
EXPECTED_LIBGME_COMMIT = "fe8da4b6d3876d7542c2fb69d94487e19836d678"
EXPECTED_INSTRUMENTATION_CONTRACT = (
    "retro-vgm-compiler:libgme-hes-psg-native-adpcm-hook-v1-ordering-v1"
)
HEX40 = re.compile(r"^[0-9a-f]{40}$")
FORBIDDEN_KEYS = {
    "artist", "composer", "candidate", "attribution", "game", "title",
    "track_title", "soundtrack", "source_path", "source_file",
    "playlist_path", "playlist_file", "m3u_path", "m3u_file",
}


def assert_label_blind(value: Any) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() in FORBIDDEN_KEYS:
                raise ValueError(f"label/source-bearing key is forbidden: {key}")
            assert_label_blind(child)
    elif isinstance(value, list):
        for child in value:
            assert_label_blind(child)


def _integer(mapping: dict[str, Any], key: str) -> int:
    value = mapping.get(key)
    if not isinstance(value, int) or isinstance(value, bool):
        raise ValueError(f"{key} must be an integer")
    return value


def _number(mapping: dict[str, Any], key: str) -> float:
    value = mapping.get(key)
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        raise ValueError(f"{key} must be numeric")
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f"{key} must be finite")
    return result


def _write_columns(value: Any, *, label: str, max_register: int, captured_clocks: int) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    count = _integer(value, "count")
    if count < 0:
        raise ValueError(f"{label}.count must be nonnegative")
    columns: dict[str, list[Any]] = {}
    for key in ("clock", "register_offset", "data"):
        column = value.get(key)
        if not isinstance(column, list) or len(column) != count:
            raise ValueError(f"{label}.{key} length must equal count")
        columns[key] = column

    previous = -1
    for clock, register, data in zip(
        columns["clock"], columns["register_offset"], columns["data"]
    ):
        if not isinstance(clock, int) or isinstance(clock, bool):
            raise ValueError(f"{label}.clock must contain integers")
        if clock < previous or not 0 <= clock <= captured_clocks:
            raise ValueError(f"{label}.clock must be monotonic inside capture")
        previous = clock
        if (
            not isinstance(register, int) or isinstance(register, bool)
            or not 0 <= register <= max_register
        ):
            raise ValueError(f"{label}.register_offset outside device range")
        if not isinstance(data, int) or isinstance(data, bool) or not 0 <= data <= 0xFF:
            raise ValueError(f"{label}.data must contain bytes")


def validate(
    value: dict[str, Any], *, source_size: int, playlist_size: int,
    playlist_loaded: bool, track_index: int, seconds: int,
) -> None:
    assert_label_blind(value)
    if value.get("model") != EXPECTED_MODEL or value.get("schema_version") != 1:
        raise ValueError("unsupported HES forensic sidecar")
    provenance = value.get("provenance")
    capture = value.get("capture")
    if not isinstance(provenance, dict) or not isinstance(capture, dict):
        raise ValueError("sidecar requires provenance and capture objects")
    if provenance.get("libgme_repository") != EXPECTED_LIBGME_REPOSITORY:
        raise ValueError("unexpected libgme repository")
    if provenance.get("libgme_commit") != EXPECTED_LIBGME_COMMIT:
        raise ValueError("unexpected libgme commit")
    if provenance.get("instrumentation_contract") != EXPECTED_INSTRUMENTATION_CONTRACT:
        raise ValueError("unexpected HES instrumentation contract")
    retro_commit = provenance.get("retro_vgm_compiler_commit")
    if not isinstance(retro_commit, str) or not HEX40.fullmatch(retro_commit):
        raise ValueError("exact VGM Compiler commit provenance is required")
    clock_rate = _integer(provenance, "clock_rate_hz")
    if clock_rate <= 0:
        raise ValueError("clock_rate_hz must be positive")

    if _integer(capture, "track_index") != track_index:
        raise ValueError("cached track index differs from request")
    if capture.get("playlist_loaded") is not playlist_loaded:
        raise ValueError("cached playlist mode differs from request")
    if _integer(capture, "source_size_bytes") != source_size:
        raise ValueError("cached source size differs from source")
    if _integer(capture, "playlist_size_bytes") != playlist_size:
        raise ValueError("cached playlist size differs from playlist")
    if _number(capture, "requested_seconds") != float(seconds):
        raise ValueError("cached duration differs from request")
    if _integer(capture, "warning_count") != 0 or capture.get("capture_complete") is not True:
        raise ValueError("warning-bearing/incomplete HES capture is not cache-admissible")
    captured_clocks = _integer(capture, "captured_clocks")
    expected_clocks = int(math.floor(float(seconds) * clock_rate + 0.5))
    if captured_clocks != expected_clocks:
        raise ValueError("captured clock count disagrees with duration and clock rate")

    _write_columns(value.get("psg_writes"), label="psg_writes", max_register=0x09,
                   captured_clocks=captured_clocks)
    _write_columns(value.get("adpcm_writes"), label="adpcm_writes", max_register=0x3FF,
                   captured_clocks=captured_clocks)


def load_and_validate(path: pathlib.Path, **kwargs: Any) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("HES forensic sidecar must be a JSON object")
    validate(value, **kwargs)
    return value
