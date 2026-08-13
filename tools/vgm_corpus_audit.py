#!/usr/bin/env python3
"""Audit VGM/VGZ structure, chip declarations, timing, and loop consistency.

This tool intentionally stays at the VGM format layer. It uses header fields and
command-stream timing defined by the VGM specification without inferring chip-
specific musical meaning. Higher chip adapters can consume the validated files
later.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import json
import pathlib
import struct


CHIP_FIELDS = (
    ("SN76489", 0x0C, 0x100),
    ("YM2413", 0x10, 0x100),
    ("YM2612", 0x2C, 0x110),
    ("YM2151", 0x30, 0x110),
    ("SegaPCM", 0x38, 0x151),
    ("RF5C68", 0x40, 0x151),
    ("YM2203", 0x44, 0x151),
    ("YM2608", 0x48, 0x151),
    ("YM2610/B", 0x4C, 0x151),
    ("YM3812", 0x50, 0x151),
    ("YM3526", 0x54, 0x151),
    ("Y8950", 0x58, 0x151),
    ("YMF262", 0x5C, 0x151),
    ("YMF278B", 0x60, 0x151),
    ("YMF271", 0x64, 0x151),
    ("YMZ280B", 0x68, 0x151),
    ("RF5C164", 0x6C, 0x151),
    ("PWM", 0x70, 0x151),
    ("AY8910", 0x74, 0x151),
    ("GameBoy DMG", 0x80, 0x161),
    ("NES APU", 0x84, 0x161),
    ("MultiPCM", 0x88, 0x161),
    ("uPD7759", 0x8C, 0x161),
    ("MSM6258", 0x90, 0x161),
    ("MSM6295", 0x98, 0x161),
    ("K051649", 0x9C, 0x161),
    ("K054539", 0xA0, 0x161),
    ("HuC6280", 0xA4, 0x161),
    ("C140", 0xA8, 0x161),
    ("K053260", 0xAC, 0x161),
    ("Pokey", 0xB0, 0x161),
    ("QSound", 0xB4, 0x161),
    ("SCSP", 0xB8, 0x171),
    ("WonderSwan", 0xC0, 0x171),
    ("VSU", 0xC4, 0x171),
    ("SAA1099", 0xC8, 0x171),
    ("ES5503", 0xCC, 0x171),
    ("ES5505/6", 0xD0, 0x171),
    ("X1-010", 0xD8, 0x171),
    ("C352", 0xDC, 0x171),
    ("GA20", 0xE0, 0x171),
    ("Mikey", 0xE4, 0x172),
)

VARIANT31 = {
    "YM2612": "YM3438",
    "YM2151": "YM2164",
    "YM2610/B": "YM2610B",
    "K051649": "K052539",
    "ES5505/6": "ES5506",
}


def u16(data: bytes, offset: int) -> int:
    if offset + 2 > len(data):
        return 0
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    if offset + 4 > len(data):
        return 0
    return struct.unpack_from("<I", data, offset)[0]


def load_vgm(path: pathlib.Path) -> bytes:
    data = path.read_bytes()
    return gzip.decompress(data) if data[:2] == b"\x1f\x8b" else data


def data_offset(raw: bytes, version: int) -> int:
    if version < 0x150:
        return 0x40
    relative = u32(raw, 0x34)
    return 0x40 if relative == 0 else 0x34 + relative


def header_u32(raw: bytes, offset: int, data_start: int) -> int:
    # VGM specifies that header bytes overlapped by sound data below 0x100 are
    # treated as zero rather than parsed as header fields.
    if data_start < 0x100 and offset >= data_start:
        return 0
    return u32(raw, offset)


def declared_chips(raw: bytes, version: int, data_start: int) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []

    legacy = header_u32(raw, 0x10, data_start)
    if version <= 0x101 and legacy:
        if legacy > 5_000_000:
            role = "YM2612"
        elif legacy < 5_000_000:
            role = "YM2151"
        else:
            role = "ambiguous YM2151/YM2612 @5MHz"
        result.append(
            {
                "field": "legacy 0x10",
                "chip": role,
                "clock_hz": legacy,
                "raw": legacy,
                "dual": False,
                "flag31": False,
            }
        )

    for name, offset, minimum_version in CHIP_FIELDS:
        if version < minimum_version:
            continue
        if version <= 0x101 and name == "YM2413":
            continue

        raw_clock = header_u32(raw, offset, data_start)
        if raw_clock == 0:
            continue

        if version >= 0x151:
            clock_hz = raw_clock & 0x3FFFFFFF
            dual = bool(raw_clock & 0x40000000)
            flag31 = bool(raw_clock & 0x80000000)
        else:
            clock_hz = raw_clock
            dual = False
            flag31 = False

        entry: dict[str, object] = {
            "field": f"0x{offset:02X}",
            "chip": name,
            "clock_hz": clock_hz,
            "raw": raw_clock,
            "dual": dual,
            "flag31": flag31,
        }
        if flag31 and name in VARIANT31:
            entry["variant_hint"] = VARIANT31[name]
        result.append(entry)

    return result


def fixed_operand_count(command: int, version: int) -> int | None:
    if 0x30 <= command <= 0x3F:
        return 1
    if command == 0x40:
        return 2
    if 0x41 <= command <= 0x4E:
        return 1 if version <= 0x160 else 2
    if command in (0x4F, 0x50):
        return 1
    if 0x51 <= command <= 0x5F:
        return 2
    if command == 0x61:
        return 2
    if command in (0x62, 0x63, 0x66):
        return 0
    if 0x70 <= command <= 0x8F:
        return 0
    if command in (0x90, 0x91):
        return 4
    if command == 0x92:
        return 5
    if command == 0x93:
        return 10
    if command == 0x94:
        return 1
    if command == 0x95:
        return 4
    if 0xA0 <= command <= 0xBF:
        return 2
    if 0xC0 <= command <= 0xDF:
        return 3
    if 0xE0 <= command <= 0xFF:
        return 4
    return None


def scan_commands(raw: bytes, version: int, start: int, loop_abs: int) -> dict[str, object]:
    position = start
    ticks = 0
    command_counts: collections.Counter[str] = collections.Counter()
    loop_tick: int | None = None
    end_found = False
    errors: list[str] = []
    boundaries: set[int] = set()

    while position < len(raw):
        boundaries.add(position)
        if loop_abs and position == loop_abs:
            loop_tick = ticks

        command = raw[position]
        command_counts[f"{command:02X}"] += 1
        position += 1

        if command == 0x66:
            end_found = True
            break

        if command == 0x61:
            if position + 2 > len(raw):
                errors.append("truncated 0x61 wait")
                break
            ticks += u16(raw, position)
            position += 2
            continue
        if command == 0x62:
            ticks += 735
            continue
        if command == 0x63:
            ticks += 882
            continue
        if 0x70 <= command <= 0x7F:
            ticks += (command & 0x0F) + 1
            continue
        if 0x80 <= command <= 0x8F:
            ticks += command & 0x0F
            continue

        if command == 0x67:
            if position + 6 > len(raw):
                errors.append("truncated 0x67 data block header")
                break
            if raw[position] != 0x66:
                errors.append(f"0x67 missing 0x66 marker at 0x{position:X}")
                break
            size = u32(raw, position + 2)
            position += 6
            if position + size > len(raw):
                errors.append("truncated 0x67 data block payload")
                break
            position += size
            continue

        if command == 0x68:
            if position + 11 > len(raw):
                errors.append("truncated 0x68 PCM RAM write")
                break
            if raw[position] != 0x66:
                errors.append(f"0x68 missing 0x66 marker at 0x{position:X}")
                break
            position += 11
            continue

        operand_count = fixed_operand_count(command, version)
        if operand_count is None:
            errors.append(
                f"unsupported/unassigned command 0x{command:02X} at 0x{position - 1:X}"
            )
            break
        if position + operand_count > len(raw):
            errors.append(f"truncated 0x{command:02X} command")
            break
        position += operand_count

    if loop_abs and loop_abs not in boundaries:
        errors.append("loop offset is not a command boundary")

    return {
        "computed_total_samples": ticks,
        "command_counts": dict(sorted(command_counts.items())),
        "loop_tick": loop_tick,
        "end_found": end_found,
        "errors": errors,
    }


def audit(path: pathlib.Path) -> dict[str, object]:
    raw = load_vgm(path)
    if raw[:4] != b"Vgm ":
        return {"file": path.name, "valid": False, "errors": ["missing Vgm signature"]}

    version = u32(raw, 0x08)
    start = data_offset(raw, version)
    total_samples = u32(raw, 0x18)
    loop_relative = u32(raw, 0x1C)
    loop_samples = u32(raw, 0x20)
    loop_abs = 0 if loop_relative == 0 else 0x1C + loop_relative

    header_errors: list[str] = []
    if start >= len(raw):
        header_errors.append("data offset outside file")
    if loop_abs and not (start <= loop_abs < len(raw)):
        header_errors.append("loop offset outside sound data")
    if loop_abs and loop_samples == 0:
        header_errors.append("loop offset present with zero loop samples")

    if start < len(raw):
        scan = scan_commands(raw, version, start, loop_abs)
    else:
        scan = {
            "computed_total_samples": 0,
            "command_counts": {},
            "loop_tick": None,
            "end_found": False,
            "errors": [],
        }

    computed_total = int(scan["computed_total_samples"])
    loop_tick = scan["loop_tick"]
    computed_loop = None if loop_tick is None else computed_total - int(loop_tick)

    return {
        "file": path.name,
        "valid": not (header_errors or scan["errors"]),
        "version": f"{(version >> 8) & 0xFF}.{version & 0xFF:02X}",
        "version_raw": version,
        "data_offset": start,
        "declared_chips": declared_chips(raw, version, start),
        "header_total_samples": total_samples,
        "computed_total_samples": computed_total,
        "total_samples_match": total_samples == computed_total,
        "loop_offset": loop_abs or None,
        "header_loop_samples": loop_samples,
        "computed_loop_samples": computed_loop,
        "loop_samples_match": (
            not loop_abs or (computed_loop is not None and loop_samples == computed_loop)
        ),
        "end_found": scan["end_found"],
        "command_counts": scan["command_counts"],
        "errors": header_errors + list(scan["errors"]),
    }


def input_paths(inputs: list[str]) -> list[pathlib.Path]:
    result: list[pathlib.Path] = []
    for input_name in inputs:
        path = pathlib.Path(input_name)
        if path.is_dir():
            result.extend(sorted([*path.glob("*.vgm"), *path.glob("*.vgz")]))
        elif path.suffix.lower() in {".vgm", ".vgz"}:
            result.append(path)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    reports = [audit(path) for path in input_paths(args.inputs)]
    if args.json:
        print(json.dumps(reports, indent=2))
    else:
        for report in reports:
            chips = ", ".join(
                f"{chip['chip']}@{chip['clock_hz']}"
                for chip in report.get("declared_chips", [])
            ) or "none"
            print(
                f"{report['file']}: VGM {report.get('version', '?')} "
                f"chips=[{chips}] total={report.get('computed_total_samples')} "
                f"match={report.get('total_samples_match')} "
                f"loop={report.get('loop_samples_match')} "
                f"errors={len(report.get('errors', []))}"
            )

    return 1 if any(not report["valid"] for report in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
