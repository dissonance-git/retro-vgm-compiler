#!/usr/bin/env python3
"""Audit VGM/VGZ structure, transport semantics, timing, and loop consistency.

This tool intentionally stays at the VGM format layer. It validates preserved
container/transport facts through the current VGM 1.72 beta surface, called
"1.72d" by this project. It does not infer chip-specific musical meaning.
"""

from __future__ import annotations

import argparse
import collections
import gzip
import json
import pathlib
import struct


VGM_172_BETA = 0x172

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

STREAM_BLOCK_NAMES = {
    0x00: "YM2612 PCM",
    0x01: "RF5C68 PCM",
    0x02: "RF5C164 PCM",
    0x03: "PWM PCM",
    0x04: "MSM6258 ADPCM",
    0x05: "HuC6280 PCM",
    0x06: "SCSP PCM",
    0x07: "NES APU DPCM",
    0x08: "Mikey PCM",
}

ROM_BLOCK_NAMES = {
    0x80: "SegaPCM ROM",
    0x81: "YM2608 DELTA-T ROM",
    0x82: "YM2610 ADPCM ROM",
    0x83: "YM2610 DELTA-T ROM",
    0x84: "YMF278B ROM",
    0x85: "YMF271 ROM",
    0x86: "YMZ280B ROM",
    0x87: "YMF278B RAM image",
    0x88: "Y8950 DELTA-T ROM",
    0x89: "MultiPCM ROM",
    0x8A: "uPD7759 ROM",
    0x8B: "MSM6295 ROM",
    0x8C: "K054539 ROM",
    0x8D: "C140 ROM",
    0x8E: "K053260 ROM",
    0x8F: "QSound ROM",
    0x90: "ES5505/ES5506 ROM",
    0x91: "X1-010 ROM",
    0x92: "C352 ROM",
    0x93: "GA20 ROM",
}

RAM_BLOCK_NAMES = {
    0xC0: "RF5C68 RAM write",
    0xC1: "RF5C164 RAM write",
    0xC2: "NES APU RAM write",
    0xE0: "SCSP RAM write",
    0xE1: "ES5503 RAM write",
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


def version_label(version: int) -> str:
    if version == VGM_172_BETA:
        return "1.72d"
    return f"{(version >> 8) & 0xFF}.{version & 0xFF:02X}"


def version_provenance(version: int) -> str:
    if version == VGM_172_BETA:
        return "upstream VGM 1.72 beta; project alias 1.72d"
    if version <= 0x171:
        return "upstream stable"
    return "newer/unknown upstream version"


def relative_offset(raw: bytes, field_offset: int) -> int | None:
    value = u32(raw, field_offset)
    return None if value == 0 else field_offset + value


def data_offset(raw: bytes, version: int) -> int:
    if version < 0x150:
        return 0x40
    relative = u32(raw, 0x34)
    return 0x40 if relative == 0 else 0x34 + relative


def header_u32(raw: bytes, offset: int, data_start: int) -> int:
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
        result.append({"field": "legacy 0x10", "chip": role, "clock_hz": legacy,
                       "raw": legacy, "dual": False, "flag31": False})

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
            "field": f"0x{offset:02X}", "chip": name, "clock_hz": clock_hz,
            "raw": raw_clock, "dual": dual, "flag31": flag31,
        }
        if flag31 and name in VARIANT31:
            entry["variant_hint"] = VARIANT31[name]
        result.append(entry)
    return result


def parse_counted_entries(raw: bytes, offset: int, entry_size: int, kind: str) -> tuple[list[dict[str, object]], list[str]]:
    if not (0 <= offset < len(raw)):
        return [], [f"{kind} list offset outside file"]
    count = raw[offset]
    start = offset + 1
    end = start + count * entry_size
    if end > len(raw):
        return [], [f"truncated {kind} list"]
    entries: list[dict[str, object]] = []
    for index in range(count):
        pos = start + index * entry_size
        if entry_size == 5:
            entries.append({"chip_id": raw[pos], "clock_hz": u32(raw, pos + 1)})
        else:
            value = u16(raw, pos + 2)
            entries.append({
                "chip_id": raw[pos] & 0x7F,
                "paired": bool(raw[pos] & 0x80),
                "second_chip": bool(raw[pos + 1] & 0x01),
                "flags": raw[pos + 1],
                "raw_volume": value,
                "relative": bool(value & 0x8000),
            })
    return entries, []


def parse_extra_header(raw: bytes, version: int, data_start: int) -> tuple[dict[str, object] | None, list[str]]:
    if version < 0x170 or data_start <= 0xBC:
        return None, []
    extra_abs = relative_offset(raw, 0xBC)
    if extra_abs is None:
        return None, []
    errors: list[str] = []
    if not (0 <= extra_abs + 4 <= len(raw)):
        return {"offset": extra_abs}, ["extra header offset outside file"]
    if extra_abs >= data_start:
        errors.append("extra header does not precede VGM data")
    size = u32(raw, extra_abs)
    if size < 4:
        errors.append("extra header size below 4")
    if extra_abs + size > len(raw):
        errors.append("extra header extends beyond file")

    clock_abs = None
    volume_abs = None
    if size >= 8 and extra_abs + 8 <= len(raw):
        rel = u32(raw, extra_abs + 4)
        clock_abs = None if rel == 0 else extra_abs + 4 + rel
    if size >= 12 and extra_abs + 12 <= len(raw):
        rel = u32(raw, extra_abs + 8)
        volume_abs = None if rel == 0 else extra_abs + 8 + rel

    clocks: list[dict[str, object]] = []
    volumes: list[dict[str, object]] = []
    if clock_abs is not None:
        clocks, more = parse_counted_entries(raw, clock_abs, 5, "extra chip-clock")
        errors.extend(more)
    if volume_abs is not None:
        volumes, more = parse_counted_entries(raw, volume_abs, 4, "extra chip-volume")
        errors.extend(more)

    return {
        "offset": extra_abs, "size": size,
        "chip_clock_offset": clock_abs, "chip_clocks": clocks,
        "chip_volume_offset": volume_abs, "chip_volumes": volumes,
    }, errors


def parse_gd3(raw: bytes, eof_abs: int) -> tuple[dict[str, object] | None, list[str]]:
    gd3_abs = relative_offset(raw, 0x14)
    if gd3_abs is None:
        return None, []
    errors: list[str] = []
    if gd3_abs + 12 > min(len(raw), eof_abs):
        return {"offset": gd3_abs}, ["GD3 header outside file"]
    if raw[gd3_abs:gd3_abs + 4] != b"Gd3 ":
        errors.append("invalid GD3 signature")
    gd3_version = u32(raw, gd3_abs + 4)
    if not (0x100 <= gd3_version < 0x200):
        errors.append("unsupported GD3 version")
    payload_size = u32(raw, gd3_abs + 8)
    payload_start = gd3_abs + 12
    payload_end = payload_start + payload_size
    if payload_end > min(len(raw), eof_abs):
        errors.append("GD3 payload extends beyond EOF")
        payload_end = min(len(raw), eof_abs)
    payload = raw[payload_start:payload_end]
    if payload_size & 1:
        errors.append("GD3 UTF-16 payload size is odd")

    fields: list[str] = []
    extra_null_padding = 0
    if payload:
        try:
            text = payload.decode("utf-16-le")
            fields = text.split("\x00")
            if fields and fields[-1] == "":
                fields.pop()
            if len(fields) > 11 and all(field == "" for field in fields[11:]):
                extra_null_padding = len(fields) - 11
                fields = fields[:11]
            if len(fields) != 11:
                errors.append(f"GD3 field count is {len(fields)}, expected 11")
        except UnicodeDecodeError:
            errors.append("invalid GD3 UTF-16LE payload")
    elif payload_size == 0:
        errors.append("GD3 contains no fields")
    return {
        "offset": gd3_abs, "version_raw": gd3_version,
        "payload_size": payload_size, "field_count": len(fields),
        "extra_null_padding_fields": extra_null_padding if payload else 0,
    }, errors


def data_block_info(block_type: int) -> dict[str, object]:
    if block_type in STREAM_BLOCK_NAMES:
        return {"type": f"{block_type:02X}", "category": "recorded_stream", "name": STREAM_BLOCK_NAMES[block_type], "defined": True}
    if 0x09 <= block_type <= 0x3F:
        return {"type": f"{block_type:02X}", "category": "recorded_stream", "name": None, "defined": False}
    if 0x40 <= block_type <= 0x7E:
        source_type = block_type - 0x40
        return {"type": f"{block_type:02X}", "category": "compressed_stream", "name": STREAM_BLOCK_NAMES.get(source_type), "defined": source_type in STREAM_BLOCK_NAMES}
    if block_type == 0x7F:
        return {"type": "7F", "category": "decompression_table", "name": "Decompression table", "defined": True}
    if block_type in ROM_BLOCK_NAMES:
        return {"type": f"{block_type:02X}", "category": "rom_ram_image", "name": ROM_BLOCK_NAMES[block_type], "defined": True}
    if 0x80 <= block_type <= 0xBF:
        return {"type": f"{block_type:02X}", "category": "rom_ram_image", "name": None, "defined": False}
    if block_type in RAM_BLOCK_NAMES:
        return {"type": f"{block_type:02X}", "category": "ram_write", "name": RAM_BLOCK_NAMES[block_type], "defined": True}
    if 0xC0 <= block_type <= 0xFF:
        return {"type": f"{block_type:02X}", "category": "ram_write", "name": None, "defined": False}
    raise AssertionError("byte out of range")


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


def scan_commands(raw: bytes, version: int, start: int, end: int, loop_abs: int) -> dict[str, object]:
    position = start
    ticks = 0
    command_counts: collections.Counter[str] = collections.Counter()
    data_block_counts: collections.Counter[str] = collections.Counter()
    data_blocks: dict[str, dict[str, object]] = {}
    loop_tick: int | None = None
    end_found = False
    errors: list[str] = []
    warnings: list[str] = []
    boundaries: set[int] = set()

    while position < end:
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
            if position + 2 > end:
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
            if position + 6 > end:
                errors.append("truncated 0x67 data block header")
                break
            if raw[position] != 0x66:
                errors.append(f"0x67 missing 0x66 marker at 0x{position:X}")
                break
            block_type = raw[position + 1]
            raw_size = u32(raw, position + 2)
            size = raw_size & 0x7FFFFFFF
            position += 6
            if position + size > end:
                errors.append("truncated 0x67 data block payload")
                break
            key = f"{block_type:02X}"
            data_block_counts[key] += 1
            info = data_block_info(block_type)
            data_blocks[key] = info
            if not info["defined"] and block_type >= 0x80:
                warnings.append(f"unknown ROM/RAM block 0x{block_type:02X} skipped")
            position += size
            continue
        if command == 0x68:
            if position + 11 > end:
                errors.append("truncated 0x68 PCM RAM write")
                break
            if raw[position] != 0x66:
                errors.append(f"0x68 missing 0x66 marker at 0x{position:X}")
                break
            position += 11
            continue

        operand_count = fixed_operand_count(command, version)
        if operand_count is None:
            errors.append(f"unsupported/unassigned command 0x{command:02X} at 0x{position - 1:X}")
            break
        if command == 0x40 and version < VGM_172_BETA:
            warnings.append("0x40 encountered before VGM 1.72 beta; treating as two-byte reserved-family payload")
        if position + operand_count > end:
            errors.append(f"truncated 0x{command:02X} command")
            break
        position += operand_count

    if loop_abs and loop_abs not in boundaries:
        errors.append("loop offset is not a command boundary")
    return {
        "computed_total_samples": ticks,
        "command_counts": dict(sorted(command_counts.items())),
        "data_block_counts": dict(sorted(data_block_counts.items())),
        "data_blocks": [data_blocks[key] | {"count": data_block_counts[key]} for key in sorted(data_blocks)],
        "loop_tick": loop_tick,
        "end_found": end_found,
        "errors": errors,
        "warnings": warnings,
    }


def audit(path: pathlib.Path) -> dict[str, object]:
    raw = load_vgm(path)
    if raw[:4] != b"Vgm ":
        return {"file": path.name, "valid": False, "errors": ["missing Vgm signature"]}
    if len(raw) < 0x38:
        return {"file": path.name, "valid": False, "errors": ["file too small for VGM header"]}

    version = u32(raw, 0x08)
    start = data_offset(raw, version)
    total_samples = u32(raw, 0x18)
    loop_abs = relative_offset(raw, 0x1C) or 0
    loop_samples = u32(raw, 0x20)
    eof_field = u32(raw, 0x04)
    eof_abs = 0x04 + eof_field if eof_field else len(raw)
    gd3_abs = relative_offset(raw, 0x14)
    stream_end = gd3_abs if gd3_abs is not None and start <= gd3_abs <= eof_abs else min(eof_abs, len(raw))

    header_errors: list[str] = []
    header_warnings: list[str] = []
    if version > VGM_172_BETA:
        header_warnings.append("version is newer than the 1.72d closure surface")
    if eof_field and eof_abs != len(raw):
        header_errors.append(f"EOF offset resolves to 0x{eof_abs:X}, file length is 0x{len(raw):X}")
    if start >= min(eof_abs, len(raw)):
        header_errors.append("data offset outside file")
    if loop_abs and not (start <= loop_abs < stream_end):
        header_errors.append("loop offset outside sound data")
    if loop_abs and loop_samples == 0:
        header_errors.append("loop offset present with zero loop samples")

    extra_header, extra_errors = parse_extra_header(raw, version, start)
    gd3, gd3_errors = parse_gd3(raw, min(eof_abs, len(raw)))
    header_errors.extend(extra_errors)
    header_errors.extend(gd3_errors)
    if gd3 and gd3.get("extra_null_padding_fields"):
        header_warnings.append(
            f"GD3 has {gd3['extra_null_padding_fields']} surplus empty field terminator(s)"
        )

    if start < stream_end:
        scan = scan_commands(raw, version, start, stream_end, loop_abs)
    else:
        scan = {"computed_total_samples": 0, "command_counts": {}, "data_block_counts": {},
                "data_blocks": [], "loop_tick": None, "end_found": False,
                "errors": [], "warnings": []}

    computed_total = int(scan["computed_total_samples"])
    loop_tick = scan["loop_tick"]
    computed_loop = None if loop_tick is None else computed_total - int(loop_tick)
    errors = header_errors + list(scan["errors"])
    warnings = header_warnings + list(scan["warnings"])

    return {
        "file": path.name,
        "valid": not errors,
        "version": version_label(version),
        "version_raw": version,
        "version_provenance": version_provenance(version),
        "data_offset": start,
        "eof_offset": eof_abs,
        "eof_matches_file": eof_abs == len(raw),
        "gd3": gd3,
        "extra_header": extra_header,
        "declared_chips": declared_chips(raw, version, start),
        "header_total_samples": total_samples,
        "computed_total_samples": computed_total,
        "total_samples_match": total_samples == computed_total,
        "loop_offset": loop_abs or None,
        "header_loop_samples": loop_samples,
        "computed_loop_samples": computed_loop,
        "loop_samples_match": (not loop_abs or (computed_loop is not None and loop_samples == computed_loop)),
        "end_found": scan["end_found"],
        "command_counts": scan["command_counts"],
        "data_block_counts": scan["data_block_counts"],
        "data_blocks": scan["data_blocks"],
        "warnings": warnings,
        "errors": errors,
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
                f"{report['file']}: VGM {report.get('version', '?')} chips=[{chips}] "
                f"total={report.get('computed_total_samples')} match={report.get('total_samples_match')} "
                f"loop={report.get('loop_samples_match')} blocks={len(report.get('data_blocks', []))} "
                f"warnings={len(report.get('warnings', []))} errors={len(report.get('errors', []))}"
            )
    return 1 if any(not report["valid"] for report in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
