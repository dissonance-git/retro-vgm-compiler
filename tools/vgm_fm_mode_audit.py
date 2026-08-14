#!/usr/bin/env python3
"""Audit FM topology and static-patch reuse in VGM/VGZ corpus files.

This tool stays below musical interpretation. It reconstructs only source-specific
FM control state needed to test whether channel-level register assumptions survive
real logs.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import pathlib
from dataclasses import dataclass, field

from vgm_corpus_audit import data_offset, fixed_operand_count, load_vgm, u16, u32


PRIMARY_YAMAHA = {
    0x51: ("YM2413", 0),
    0x52: ("YM2612", 0),
    0x53: ("YM2612", 1),
    0x54: ("YM2151", 0),
    0x55: ("YM2203", 0),
    0x56: ("YM2608", 0),
    0x57: ("YM2608", 1),
    0x58: ("YM2610", 0),
    0x59: ("YM2610", 1),
    0x5A: ("YM3812", 0),
    0x5B: ("YM3526", 0),
    0x5C: ("Y8950", 0),
    0x5D: ("YMZ280B", 0),
    0x5E: ("YMF262", 0),
    0x5F: ("YMF262", 1),
}
OPN_TARGETS = {"YM2612", "YM2203", "YM2608", "YM2610"}


@dataclass(frozen=True)
class RegisterWrite:
    tick: int
    file_offset: int
    target: str
    instance: int
    port: int
    address: int
    data: int
    command: int


@dataclass
class OPNState:
    mode_bits: int = 0
    high_latch: int = 0
    channel_frequency: dict[int, tuple[int, int]] = field(default_factory=dict)
    patch_registers: dict[int, dict[tuple[int, int], int]] = field(
        default_factory=lambda: collections.defaultdict(dict)
    )
    algorithm_feedback: dict[int, int] = field(default_factory=dict)
    patch_pitch_uses: dict[str, set[tuple[int, int]]] = field(
        default_factory=lambda: collections.defaultdict(set)
    )
    complete_patch_uses: collections.Counter[str] = field(default_factory=collections.Counter)


def decode_yamaha_write(command: int, payload: bytes, tick: int, file_offset: int):
    if len(payload) != 2:
        return None
    primary = command
    instance = 0
    if 0xA1 <= command <= 0xAF:
        primary = command - 0x50
        instance = 1
    semantics = PRIMARY_YAMAHA.get(primary)
    if semantics is None:
        return None
    target, port = semantics
    return RegisterWrite(
        tick=tick,
        file_offset=file_offset,
        target=target,
        instance=instance,
        port=port,
        address=payload[0],
        data=payload[1],
        command=command,
    )


def iter_register_writes(raw: bytes):
    if raw[:4] != b"Vgm ":
        raise ValueError("missing Vgm signature")
    version = u32(raw, 0x08)
    position = data_offset(raw, version)
    tick = 0

    while position < len(raw):
        command_offset = position
        command = raw[position]
        position += 1

        if command == 0x66:
            return

        if command == 0x61:
            if position + 2 > len(raw):
                raise ValueError("truncated 0x61 wait")
            tick += u16(raw, position)
            position += 2
            continue
        if command == 0x62:
            tick += 735
            continue
        if command == 0x63:
            tick += 882
            continue
        if 0x70 <= command <= 0x7F:
            tick += (command & 0x0F) + 1
            continue
        if 0x80 <= command <= 0x8F:
            tick += command & 0x0F
            continue

        if command == 0x67:
            if position + 6 > len(raw) or raw[position] != 0x66:
                raise ValueError("malformed 0x67 data block")
            size = u32(raw, position + 2)
            position += 6
            if position + size > len(raw):
                raise ValueError("truncated 0x67 data block")
            position += size
            continue

        if command == 0x68:
            if position + 11 > len(raw) or raw[position] != 0x66:
                raise ValueError("malformed 0x68 PCM RAM write")
            position += 11
            continue

        operand_count = fixed_operand_count(command, version)
        if operand_count is None:
            raise ValueError(
                f"unsupported/unassigned command 0x{command:02X} at 0x{command_offset:X}"
            )
        if position + operand_count > len(raw):
            raise ValueError(f"truncated 0x{command:02X} command")

        payload = raw[position:position + operand_count]
        position += operand_count
        write = decode_yamaha_write(command, payload, tick, command_offset)
        if write is not None:
            yield write


def opn_channel_from_port_register(port: int, reg: int):
    local = reg & 0x03
    if local >= 3 or port >= 2:
        return None
    return port * 3 + local


def opn_key_channel(data: int):
    local = data & 0x03
    if local == 3:
        return None
    return local + (3 if data & 0x04 else 0)


def opn_operator_register(reg: int) -> bool:
    return 0x30 <= reg <= 0x9F and (reg & 0x03) != 0x03


def normalized_opn_patch_key(reg: int):
    # Operator parameter group (0x30..0x90) and Yamaha physical slot number.
    return (reg & 0xF0, (reg >> 2) & 0x03)


def patch_fingerprint(state: OPNState, channel: int):
    regs = state.patch_registers.get(channel, {})
    algorithm = state.algorithm_feedback.get(channel)
    complete = len(regs) == 28 and algorithm is not None
    if not regs and algorithm is None:
        return None, False

    payload = {
        "operators": sorted((group, slot, value) for (group, slot), value in regs.items()),
        "algorithm_feedback": algorithm,
    }
    digest = hashlib.sha256(
        json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    ).hexdigest()
    return digest, complete


def audit_bytes(raw: bytes, name: str = "<memory>"):
    report = {
        "file": name,
        "opn": {
            "mode_write_count": 0,
            "mode_values": [],
            "effect_mode_seen": False,
            "csm_exact_seen": False,
            "three_slot_bit_seen": False,
            "special_frequency_write_count": 0,
            "special_frequency_writes_while_effect_enabled": 0,
            "key_on_count": 0,
            "complete_patch_key_on_count": 0,
            "complete_patch_reused_at_distinct_pitches": 0,
            "distinct_complete_patch_count": 0,
        },
        "opl3": {
            "new_mode_write_count": 0,
            "new_mode_seen": False,
            "four_op_mask_write_count": 0,
            "programmed_four_op_mask_values": [],
            "active_four_op_seen": False,
            "active_four_op_mask_values": [],
            "rhythm_mode_seen": False,
        },
        "errors": [],
    }

    opn_states: dict[tuple[str, int], OPNState] = {}
    opl3_state: dict[int, dict[str, int | bool]] = collections.defaultdict(
        lambda: {"new_mode": False, "pair_mask": 0}
    )
    opn_mode_values: set[int] = set()
    opl3_programmed_masks: set[int] = set()
    opl3_active_masks: set[int] = set()

    try:
        writes = iter_register_writes(raw)
        for write in writes:
            if write.target in OPN_TARGETS:
                key = (write.target, write.instance)
                state = opn_states.setdefault(key, OPNState())

                if write.port == 0 and write.address == 0x27:
                    state.mode_bits = (write.data >> 6) & 0x03
                    report["opn"]["mode_write_count"] += 1
                    opn_mode_values.add(state.mode_bits)
                    report["opn"]["effect_mode_seen"] |= state.mode_bits != 0
                    report["opn"]["csm_exact_seen"] |= state.mode_bits == 0x02
                    report["opn"]["three_slot_bit_seen"] |= bool(state.mode_bits & 0x01)

                if write.port == 0 and (
                    0xA8 <= write.address <= 0xAA or 0xAC <= write.address <= 0xAE
                ):
                    report["opn"]["special_frequency_write_count"] += 1
                    if state.mode_bits != 0:
                        report["opn"]["special_frequency_writes_while_effect_enabled"] += 1

                if (
                    0xA4 <= write.address <= 0xA6
                    or 0xAC <= write.address <= 0xAE
                ):
                    state.high_latch = write.data & 0x3F
                elif 0xA0 <= write.address <= 0xA2:
                    channel = opn_channel_from_port_register(write.port, write.address)
                    if channel is not None:
                        fnum = write.data | ((state.high_latch & 0x07) << 8)
                        block = (state.high_latch >> 3) & 0x07
                        state.channel_frequency[channel] = (block, fnum)

                if opn_operator_register(write.address):
                    channel = opn_channel_from_port_register(write.port, write.address)
                    if channel is not None:
                        state.patch_registers[channel][
                            normalized_opn_patch_key(write.address)
                        ] = write.data
                elif 0xB0 <= write.address <= 0xB2:
                    channel = opn_channel_from_port_register(write.port, write.address)
                    if channel is not None:
                        state.algorithm_feedback[channel] = write.data & 0x3F

                if write.port == 0 and write.address == 0x28:
                    channel = opn_key_channel(write.data)
                    operator_mask = (write.data >> 4) & 0x0F
                    if channel is not None and operator_mask:
                        report["opn"]["key_on_count"] += 1
                        fingerprint, complete = patch_fingerprint(state, channel)
                        frequency = state.channel_frequency.get(channel)
                        if fingerprint is not None and frequency is not None:
                            state.patch_pitch_uses[fingerprint].add(frequency)
                            if complete:
                                state.complete_patch_uses[fingerprint] += 1
                                report["opn"]["complete_patch_key_on_count"] += 1

            elif write.target == "YMF262":
                state = opl3_state[write.instance]
                if write.port == 1 and write.address == 0x04:
                    state["pair_mask"] = write.data & 0x3F
                    report["opl3"]["four_op_mask_write_count"] += 1
                    opl3_programmed_masks.add(int(state["pair_mask"]))
                elif write.port == 1 and write.address == 0x05:
                    state["new_mode"] = bool(write.data & 0x01)
                    report["opl3"]["new_mode_write_count"] += 1
                    report["opl3"]["new_mode_seen"] |= bool(state["new_mode"])
                elif write.port == 0 and write.address == 0xBD:
                    report["opl3"]["rhythm_mode_seen"] |= bool(write.data & 0x20)

                active_mask = int(state["pair_mask"]) if state["new_mode"] else 0
                if active_mask:
                    report["opl3"]["active_four_op_seen"] = True
                    opl3_active_masks.add(active_mask)

    except ValueError as exc:
        report["errors"].append(str(exc))

    complete_patch_pitch_sets: dict[str, set[tuple[int, int]]] = collections.defaultdict(set)
    complete_patch_names: set[str] = set()
    for state in opn_states.values():
        for fingerprint in state.complete_patch_uses:
            complete_patch_names.add(fingerprint)
            complete_patch_pitch_sets[fingerprint].update(state.patch_pitch_uses[fingerprint])

    report["opn"]["mode_values"] = sorted(opn_mode_values)
    report["opn"]["distinct_complete_patch_count"] = len(complete_patch_names)
    report["opn"]["complete_patch_reused_at_distinct_pitches"] = sum(
        1 for pitches in complete_patch_pitch_sets.values() if len(pitches) > 1
    )
    report["opl3"]["programmed_four_op_mask_values"] = sorted(opl3_programmed_masks)
    report["opl3"]["active_four_op_mask_values"] = sorted(opl3_active_masks)
    return report


def audit(path: pathlib.Path):
    return audit_bytes(load_vgm(path), path.name)


def input_paths(inputs: list[str]) -> list[pathlib.Path]:
    result: list[pathlib.Path] = []
    for input_name in inputs:
        path = pathlib.Path(input_name)
        if path.is_dir():
            result.extend(sorted([*path.glob("*.vgm"), *path.glob("*.vgz")]))
        elif path.suffix.lower() in {".vgm", ".vgz"}:
            result.append(path)
    return result


def summarize(reports):
    return {
        "files": len(reports),
        "files_with_errors": sum(bool(r["errors"]) for r in reports),
        "opn_effect_mode_files": sum(r["opn"]["effect_mode_seen"] for r in reports),
        "opn_csm_exact_files": sum(r["opn"]["csm_exact_seen"] for r in reports),
        "opn_special_frequency_files": sum(
            r["opn"]["special_frequency_write_count"] > 0 for r in reports
        ),
        "opn_patch_reuse_files": sum(
            r["opn"]["complete_patch_reused_at_distinct_pitches"] > 0 for r in reports
        ),
        "opl3_new_mode_files": sum(r["opl3"]["new_mode_seen"] for r in reports),
        "opl3_active_four_op_files": sum(r["opl3"]["active_four_op_seen"] for r in reports),
        "opl3_rhythm_mode_files": sum(r["opl3"]["rhythm_mode_seen"] for r in reports),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    reports = [audit(path) for path in input_paths(args.inputs)]
    aggregate = summarize(reports)

    if args.json:
        print(json.dumps({"summary": aggregate, "files": reports}, indent=2))
    elif not args.quiet:
        print(json.dumps(aggregate, indent=2))

    return 1 if aggregate["files_with_errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
