#!/usr/bin/env python3
"""Derive HuC6280 physical voice episodes from a creator-blind HES sidecar.

This layer intentionally stops below musical-part identity. HuC6280 channel
numbers are physical execution slots. Period changes are native-relative pitch
observations only when the enabled slot is in waveform mode; noise and direct
DAC remain explicit realization modes instead of being forced into pitch.
"""
from __future__ import annotations

import argparse
import json
import math
import pathlib
import sys
from dataclasses import dataclass
from typing import Any

THIS_DIR = pathlib.Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))
import validate_forensic_sidecar as schema

MODEL = "creator-blind cached HES physical voice evidence"
PITCH_BASIS = "huc6280_period_relative_frequency"
INTERVAL_SEMANTICS = "log2_frequency_ratio_octaves"


@dataclass
class ChannelState:
    period: int = 0
    control: int = 0x40
    noise: int = 0
    active_episode_id: int | None = None


def _columns(value: Any, label: str) -> list[tuple[int, int, int]]:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    count = value.get("count")
    clocks = value.get("clock")
    registers = value.get("register_offset")
    data = value.get("data")
    if not isinstance(count, int) or isinstance(count, bool) or count < 0:
        raise ValueError(f"{label}.count must be nonnegative")
    if not all(isinstance(column, list) and len(column) == count for column in (clocks, registers, data)):
        raise ValueError(f"{label} columns must match count")
    result: list[tuple[int, int, int]] = []
    previous = -1
    for clock, register, byte in zip(clocks, registers, data):
        if not isinstance(clock, int) or isinstance(clock, bool) or clock < previous:
            raise ValueError(f"{label} clocks must be monotonic integers")
        if not isinstance(register, int) or isinstance(register, bool):
            raise ValueError(f"{label} registers must be integers")
        if not isinstance(byte, int) or isinstance(byte, bool):
            raise ValueError(f"{label} data must be integers")
        previous = clock
        result.append((clock, register, byte))
    return result


def _mode(channel: int, state: ChannelState) -> str:
    if not state.control & 0x80:
        return "disabled"
    if channel >= 4 and state.noise & 0x80:
        return "noise"
    if state.control & 0x40:
        return "direct_dac"
    return "wave"


def project(sidecar: dict[str, Any]) -> dict[str, Any]:
    schema.assert_label_blind(sidecar)
    if sidecar.get("model") != schema.EXPECTED_MODEL or sidecar.get("schema_version") != 1:
        raise ValueError("not a supported creator-blind HES forensic sidecar")
    provenance = sidecar.get("provenance")
    capture = sidecar.get("capture")
    if not isinstance(provenance, dict) or not isinstance(capture, dict):
        raise ValueError("HES sidecar requires provenance and capture")
    if provenance.get("libgme_repository") != schema.EXPECTED_LIBGME_REPOSITORY:
        raise ValueError("HES sidecar repository provenance differs from decoder contract")
    if provenance.get("libgme_commit") != schema.EXPECTED_LIBGME_COMMIT:
        raise ValueError("HES sidecar libgme provenance differs from decoder contract")
    if provenance.get("instrumentation_contract") != schema.EXPECTED_INSTRUMENTATION_CONTRACT:
        raise ValueError("HES sidecar instrumentation differs from decoder contract")
    retro_commit = provenance.get("retro_vgm_compiler_commit")
    if not isinstance(retro_commit, str) or not schema.HEX40.fullmatch(retro_commit):
        raise ValueError("HES sidecar requires exact VGM Compiler provenance")
    if capture.get("capture_complete") is not True or capture.get("warning_count") != 0:
        raise ValueError("incomplete HES capture cannot produce physical voice evidence")
    captured_clocks = capture.get("captured_clocks")
    if not isinstance(captured_clocks, int) or isinstance(captured_clocks, bool) or captured_clocks < 0:
        raise ValueError("captured_clocks must be a nonnegative integer")

    writes = _columns(sidecar.get("psg_writes"), "psg_writes")
    states = [ChannelState() for _ in range(6)]
    latch = 0
    episodes: list[dict[str, Any]] = []
    pitch: list[dict[str, Any]] = []
    modes: list[dict[str, Any]] = []
    pitch_at: dict[tuple[int, int], int] = {}
    mode_at: dict[tuple[int, int], int] = {}

    def add_pitch(channel: int, clock: int) -> None:
        state = states[channel]
        if state.active_episode_id is None or _mode(channel, state) != "wave" or state.period <= 0:
            return
        item = {
            "episode_id": state.active_episode_id,
            "channel": channel,
            "clock": clock,
            "period": state.period,
            "log2_pitch_coordinate": -math.log2(float(state.period)),
            "pitch_basis": PITCH_BASIS,
            "interval_semantics": INTERVAL_SEMANTICS,
        }
        key = (state.active_episode_id, clock)
        existing = pitch_at.get(key)
        if existing is None:
            pitch_at[key] = len(pitch)
            pitch.append(item)
        else:
            pitch[existing] = item

    def mode_observation(channel: int, clock: int) -> None:
        state = states[channel]
        if state.active_episode_id is None:
            return
        item = {
            "episode_id": state.active_episode_id,
            "channel": channel,
            "clock": clock,
            "mode": _mode(channel, state),
        }
        key = (state.active_episode_id, clock)
        existing = mode_at.get(key)
        if existing is None:
            mode_at[key] = len(modes)
            modes.append(item)
        else:
            modes[existing] = item

    for clock, register, data in writes:
        if clock > captured_clocks:
            raise ValueError("PSG write escapes bounded HES capture")
        if not 0 <= register <= 9 or not 0 <= data <= 0xFF:
            raise ValueError("PSG write lies outside HuC6280 register/data range")
        if register == 0:
            latch = data & 7
            continue
        if latch >= 6 or register == 1:
            continue
        state = states[latch]
        old_enabled = bool(state.control & 0x80)
        old_mode = _mode(latch, state)

        if register == 2:
            state.period = (state.period & 0xF00) | data
        elif register == 3:
            state.period = (state.period & 0x0FF) | ((data & 0x0F) << 8)
        elif register == 4:
            state.control = data
        elif register == 7 and latch >= 4:
            state.noise = data
        elif register not in (5, 6, 8, 9):
            continue

        new_enabled = bool(state.control & 0x80)
        if not old_enabled and new_enabled:
            episode_id = len(episodes)
            state.active_episode_id = episode_id
            episodes.append({
                "episode_id": episode_id,
                "channel": latch,
                "start_clock": clock,
                "end_clock": None,
                "right_censored": False,
                "end_reason": None,
            })
            mode_observation(latch, clock)
            add_pitch(latch, clock)
        elif old_enabled and not new_enabled:
            if state.active_episode_id is None:
                raise ValueError("enabled HES slot lost its active episode")
            episode = episodes[state.active_episode_id]
            episode["end_clock"] = clock
            episode["end_reason"] = "control_disable"
            state.active_episode_id = None
        elif new_enabled:
            new_mode = _mode(latch, state)
            if new_mode != old_mode:
                mode_observation(latch, clock)
                if new_mode == "wave":
                    add_pitch(latch, clock)
            elif register in (2, 3) and new_mode == "wave":
                add_pitch(latch, clock)

    for state in states:
        if state.active_episode_id is None:
            continue
        episode = episodes[state.active_episode_id]
        episode["end_clock"] = captured_clocks
        episode["end_reason"] = "capture_boundary"
        episode["right_censored"] = True

    return {
        "model": MODEL,
        "claim_boundary": (
            "Bounded HuC6280 enabled-slot episodes, realization-mode transitions, and "
            "native-relative waveform pitch observations only. Physical channel identity "
            "is not persistent musical-part identity."
        ),
        "source_model": schema.EXPECTED_MODEL,
        "provenance": dict(provenance),
        "capture": {
            "track_index": capture.get("track_index"),
            "captured_clocks": captured_clocks,
        },
        "episodes": episodes,
        "mode_observations": modes,
        "pitch_observations": pitch,
        "adpcm_observed_write_count": sidecar.get("adpcm_writes", {}).get("count", 0),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sidecar", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()
    value = json.loads(args.sidecar.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("HES sidecar must be a JSON object")
    result = project(value)
    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
