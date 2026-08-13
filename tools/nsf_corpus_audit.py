#!/usr/bin/env python3
"""Perform narrow structural admission checks for NSF and NSFe fixtures.

This tool validates only container identity and cheap structural invariants. It
does not execute 6502 code, emulate NES audio, or establish correct playback.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import struct


EXPANSION_AUDIO = (
    (0x01, "VRC6"),
    (0x02, "VRC7"),
    (0x04, "FDS"),
    (0x08, "MMC5"),
    (0x10, "Namco 163"),
    (0x20, "Sunsoft 5B"),
    (0x40, "VT02+"),
)

KNOWN_MANDATORY_NSFE_CHUNKS = {"INFO", "DATA", "NEND", "BANK", "RATE", "NSF2"}


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def expansion_audio(flags: int) -> list[str]:
    return [name for mask, name in EXPANSION_AUDIO if flags & mask]


def audit_nsf(path: pathlib.Path, data: bytes) -> dict[str, object]:
    errors: list[str] = []
    if len(data) < 0x80:
        errors.append("truncated NSF header")
    if data[:5] != b"NESM\x1a":
        errors.append("missing NESM signature")

    if errors:
        return {
            "file": path.name,
            "source_family": "NSF",
            "size_bytes": len(data),
            "valid_container": False,
            "errors": errors,
        }

    version = data[5]
    song_count = data[6]
    starting_song = data[7]
    if version == 0:
        errors.append("zero NSF version")
    if song_count == 0:
        errors.append("zero declared songs")
    if not 1 <= starting_song <= song_count:
        errors.append("starting song is outside the declared song range")

    flags = data[0x7B]
    return {
        "file": path.name,
        "source_family": "NSF",
        "size_bytes": len(data),
        "valid_container": not errors,
        "version": version,
        "song_count": song_count,
        "starting_song_1_based": starting_song,
        "load_address": u16(data, 0x08),
        "init_address": u16(data, 0x0A),
        "play_address": u16(data, 0x0C),
        "expansion_audio": expansion_audio(flags),
        "expansion_flags_raw": flags,
        "errors": errors,
        "validation_scope": "container structure only; playback not validated",
    }


def audit_nsfe(path: pathlib.Path, data: bytes) -> dict[str, object]:
    errors: list[str] = []
    chunks: list[str] = []
    info: bytes | None = None
    data_seen = False
    nend_seen = False

    if data[:4] != b"NSFE":
        errors.append("missing NSFE signature")
    else:
        position = 4
        while position < len(data):
            if position + 8 > len(data):
                errors.append("truncated NSFe chunk header")
                break
            size = struct.unpack_from("<I", data, position)[0]
            chunk_id_bytes = data[position + 4 : position + 8]
            try:
                chunk_id = chunk_id_bytes.decode("ascii")
            except UnicodeDecodeError:
                chunk_id = chunk_id_bytes.hex()
                errors.append("non-ASCII NSFe chunk identifier")
            position += 8
            if position + size > len(data):
                errors.append(f"truncated {chunk_id} chunk payload")
                break
            payload = data[position : position + size]
            position += size
            chunks.append(chunk_id)

            if chunk_id == "INFO":
                if info is not None:
                    errors.append("duplicate INFO chunk")
                info = payload
            elif chunk_id == "DATA":
                if info is None:
                    errors.append("DATA appears before INFO")
                data_seen = True
            elif chunk_id == "NEND":
                nend_seen = True
                break
            elif chunk_id[:1].isupper() and chunk_id not in KNOWN_MANDATORY_NSFE_CHUNKS:
                errors.append(f"unknown mandatory NSFe chunk: {chunk_id}")

        if info is None:
            errors.append("missing INFO chunk")
        elif len(info) < 9:
            errors.append("INFO chunk is shorter than 9 bytes")
        if not data_seen:
            errors.append("missing DATA chunk")
        if not nend_seen:
            errors.append("missing NEND chunk")

    report: dict[str, object] = {
        "file": path.name,
        "source_family": "NSFe",
        "size_bytes": len(data),
        "valid_container": not errors,
        "chunks": chunks,
        "errors": errors,
        "validation_scope": "container structure only; playback not validated",
    }
    if info is not None and len(info) >= 9:
        flags = info[7]
        starting_song = info[9] if len(info) >= 10 else 0
        load_address = u16(info, 0)
        init_address = u16(info, 2)
        play_address = u16(info, 4)
        report.update(
            {
                "song_count": info[8],
                "starting_song_0_based": starting_song,
                "load_address": load_address,
                "init_address": init_address,
                "play_address": play_address,
                "expansion_audio": expansion_audio(flags),
                "expansion_flags_raw": flags,
            }
        )
        if info[8] == 0:
            errors.append("zero declared songs")
        if info[8] and starting_song >= info[8]:
            errors.append("starting song is outside the declared song range")
        if info[6] & 0xFC:
            errors.append("reserved INFO region bits are set")
        if flags & 0x80:
            errors.append("reserved INFO expansion-audio bit is set")
        for label, address in (
            ("load", load_address),
            ("init", init_address),
            ("play", play_address),
        ):
            if address < 0x8000:
                errors.append(f"{label} address is below 0x8000")
        report["valid_container"] = not errors
    return report


def audit(path: pathlib.Path) -> dict[str, object]:
    data = path.read_bytes()
    if path.suffix.lower() == ".nsf":
        return audit_nsf(path, data)
    if path.suffix.lower() == ".nsfe":
        return audit_nsfe(path, data)
    return {
        "file": path.name,
        "source_family": "unknown",
        "size_bytes": len(data),
        "valid_container": False,
        "errors": ["unsupported suffix; expected .nsf or .nsfe"],
    }


def input_paths(inputs: list[str]) -> list[pathlib.Path]:
    result: list[pathlib.Path] = []
    for input_name in inputs:
        path = pathlib.Path(input_name)
        if path.is_dir():
            result.extend(
                sorted(
                    p
                    for p in path.iterdir()
                    if p.is_file() and p.suffix.lower() in {".nsf", ".nsfe"}
                )
            )
        elif path.suffix.lower() in {".nsf", ".nsfe"}:
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
            print(
                f"{report['file']}: {report['source_family']} "
                f"valid={report['valid_container']} "
                f"songs={report.get('song_count', '?')} "
                f"errors={len(report['errors'])}"
            )
    return 1 if not reports or any(not report["valid_container"] for report in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
