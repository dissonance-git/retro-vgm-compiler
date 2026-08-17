#!/usr/bin/env python3
"""Freeze already-approved upstream SPC sources into one `.studiosrc` packet.

This is a final evidence-boundary tool, not a discovery or matching tool. It does
not search sample libraries, fit gains, infer loop points, resample audio, promote
provenance, or decide that an upstream source is historically correct.

The manifest must already carry the exact admission state used by the C++ top
rung plus content hashes and opaque 128-bit identities. The upstream waveform is
supplied as raw little-endian IEEE-754 float32 so packet authoring performs no
sample-format conversion. Start address, BRR extent, END/LOOP state, loop block
and the compressed witness are derived only from the SPC snapshot.

Manifest example:

{
  "schema": "spc-studiosrc-sidecar-manifest-001",
  "spc_file": "music.spc",
  "sources": [
    {
      "source_number": 7,
      "directory_page": "0x4c",
      "upstream_pcm_f32le": "samples/snare.f32le",
      "upstream_pcm_sha256": "...64 hex chars...",
      "game_brr_sha256": "...64 hex chars...",
      "game_brr_identity": {"high": "0x1111", "low": "0x2222"},
      "upstream_identity": {"high": "0x3333", "low": "0x4444"},
      "sample_rate_hz": 48000.0,
      "game_pcm_units_per_source_unit": 32768.0,
      "game_origin": 0.0,
      "upstream_origin": 0.0,
      "upstream_frames_per_game_sample": 1.5,
      "upstream_loop_start": 48.0,
      "admission": {
        "relation": "exact_pre_brr_source",
        "evidence": "exact_upstream_source",
        "basis": "exact_upstream_pcm",
        "preparation_chain_exact": true,
        "identity_validation_passed": true
      }
    }
  ]
}

`directory_page` is optional and defaults to the snapshot's current DSP DIR
register. If supplied, it selects/asserts the directory topology that this
approved source is meant to describe. `upstream_loop_start` is required exactly
when the snapshot BRR END header has LOOP set.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct


MAGIC = 0x43525353  # "SSRC" little-endian
VERSION = 1
HEADER_SIZE = 16
ENTRY_SIZE = 120
FLAG_LOOP = 0x01
NO_LOOP = 0xFFFFFFFF
MAX_SOURCES = 256
SAMPLES_PER_BRR_BLOCK = 16
BYTES_PER_BRR_BLOCK = 9

SPC_SIGNATURE = b"SNES-SPC700 Sound File Data"
SPC_RAM_OFFSET = 0x100
SPC_RAM_SIZE = 0x10000
SPC_DSP_OFFSET = 0x10100
SPC_DSP_SIZE = 0x80
SPC_MIN_SIZE = SPC_DSP_OFFSET + SPC_DSP_SIZE
SPC_DIR_REGISTER = 0x5D
MAX_BRR_SCAN_BLOCKS = SPC_RAM_SIZE // BYTES_PER_BRR_BLOCK

SCHEMA = "spc-studiosrc-sidecar-manifest-001"
EXPECTED_ADMISSION = {
    "relation": "exact_pre_brr_source",
    "evidence": "exact_upstream_source",
    "basis": "exact_upstream_pcm",
    "preparation_chain_exact": True,
    "identity_validation_passed": True,
}


def _parse_uint(value: object, bits: int, label: str) -> int:
    if isinstance(value, str):
        parsed = int(value, 0)
    elif isinstance(value, int) and not isinstance(value, bool):
        parsed = value
    else:
        raise ValueError(f"{label} must be an integer or base-prefixed integer string")
    maximum = (1 << bits) - 1
    if parsed < 0 or parsed > maximum:
        raise ValueError(f"{label} out of range for uint{bits}: {parsed}")
    return parsed


def _parse_finite_positive(value: object, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be a finite positive number")
    parsed = float(value)
    if not math.isfinite(parsed) or parsed <= 0.0:
        raise ValueError(f"{label} must be a finite positive number")
    return parsed


def _parse_finite(value: object, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be a finite number")
    parsed = float(value)
    if not math.isfinite(parsed):
        raise ValueError(f"{label} must be a finite number")
    return parsed


def _parse_identity(value: object, label: str) -> tuple[int, int]:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object with high/low uint64 values")
    high = _parse_uint(value.get("high"), 64, f"{label}.high")
    low = _parse_uint(value.get("low"), 64, f"{label}.low")
    if high == 0 and low == 0:
        raise ValueError(f"{label} must be present/nonzero")
    return high, low


def _parse_sha256(value: object, label: str) -> str:
    if not isinstance(value, str) or len(value) != 64:
        raise ValueError(f"{label} must be exactly 64 hexadecimal characters")
    try:
        int(value, 16)
    except ValueError as exc:
        raise ValueError(f"{label} must be hexadecimal") from exc
    return value.lower()


def _resolve_path(value: object, base: Path, label: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{label} must be a path string")
    path = Path(value)
    return path if path.is_absolute() else base / path


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _near_integer(value: float, label: str) -> int:
    if not math.isfinite(value):
        raise ValueError(f"{label} is not finite")
    rounded = round(value)
    if abs(value - rounded) > 1.0e-9:
        raise ValueError(f"{label} must map to an exact integer upstream frame: {value}")
    return int(rounded)


def read_spc(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < SPC_MIN_SIZE or not data.startswith(SPC_SIGNATURE):
        raise ValueError(f"not a complete SNES-SPC700 snapshot: {path}")
    return data


def spc_ram_byte(spc: bytes, address: int) -> int:
    return spc[SPC_RAM_OFFSET + (address & 0xFFFF)]


def spc_ram_u16(spc: bytes, address: int) -> int:
    return spc_ram_byte(spc, address) | (spc_ram_byte(spc, address + 1) << 8)


def current_directory_page(spc: bytes) -> int:
    return spc[SPC_DSP_OFFSET + SPC_DIR_REGISTER]


def copy_wrapped_ram(spc: bytes, first_address: int, size: int) -> bytes:
    return bytes(spc_ram_byte(spc, first_address + offset) for offset in range(size))


def derive_brr_topology(spc: bytes, directory_page: int, source_number: int) -> dict[str, object]:
    directory_address = ((directory_page << 8) + source_number * 4) & 0xFFFF
    first_address = spc_ram_u16(spc, directory_address)
    loop_address = spc_ram_u16(spc, directory_address + 2)

    for block in range(MAX_BRR_SCAN_BLOCKS):
        address = (first_address + block * BYTES_PER_BRR_BLOCK) & 0xFFFF
        header = spc_ram_byte(spc, address)
        if not (header & 0x01):
            continue

        block_count = block + 1
        loop_present = bool(header & 0x02)
        loop_ordinal = NO_LOOP
        if loop_present:
            delta = (loop_address - first_address) & 0xFFFF
            if delta % BYTES_PER_BRR_BLOCK:
                raise ValueError(
                    f"SRCN {source_number} live loop address 0x{loop_address:04x} "
                    "is not a witnessed BRR block boundary"
                )
            loop_ordinal = delta // BYTES_PER_BRR_BLOCK
            if loop_ordinal >= block_count:
                raise ValueError(
                    f"SRCN {source_number} live loop target is outside first..END witness"
                )

        witness_size = block_count * BYTES_PER_BRR_BLOCK
        witness = copy_wrapped_ram(spc, first_address, witness_size)
        return {
            "directory_page": directory_page,
            "directory_address": directory_address,
            "first_address": first_address,
            "loop_address": loop_address,
            "terminal_header": header,
            "block_count": block_count,
            "loop_present": loop_present,
            "loop_ordinal": loop_ordinal,
            "game_end_sample": block_count * SAMPLES_PER_BRR_BLOCK,
            "game_loop_start": (
                loop_ordinal * SAMPLES_PER_BRR_BLOCK if loop_present else None
            ),
            "witness": witness,
        }

    raise ValueError(
        f"SRCN {source_number} did not reach BRR END within bounded one-RAM-image witness"
    )


def read_f32le(path: Path) -> tuple[bytes, int]:
    data = path.read_bytes()
    if not data or len(data) % 4:
        raise ValueError(f"upstream PCM must be nonempty raw float32 LE: {path}")
    frame_count = len(data) // 4
    for (sample,) in struct.iter_unpack("<f", data):
        if not math.isfinite(sample):
            raise ValueError(f"upstream PCM contains non-finite float32: {path}")
    return data, frame_count


def _validate_admission(value: object, label: str) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{label} must be an object")
    if value != EXPECTED_ADMISSION:
        raise ValueError(
            f"{label} must exactly equal the already-approved top-rung admission state"
        )


def _validate_coordinate_map(
    *,
    frame_count: int,
    game_origin: float,
    upstream_origin: float,
    ratio: float,
    topology: dict[str, object],
    upstream_loop_start: float,
) -> None:
    game_start = 0.0
    game_end = float(topology["game_end_sample"])
    mapped_start = upstream_origin + (game_start - game_origin) * ratio
    mapped_end = upstream_origin + (game_end - game_origin) * ratio
    start_frame = _near_integer(mapped_start, "mapped playback start")
    end_frame = _near_integer(mapped_end, "mapped playback end")
    if start_frame < 0 or end_frame <= start_frame or end_frame > frame_count:
        raise ValueError(
            f"mapped playback frames [{start_frame}, {end_frame}) exceed upstream PCM "
            f"extent [0, {frame_count})"
        )

    if bool(topology["loop_present"]):
        game_loop_start = float(topology["game_loop_start"])
        mapped_loop = upstream_origin + (game_loop_start - game_origin) * ratio
        if abs(mapped_loop - upstream_loop_start) > 1.0e-9:
            raise ValueError(
                "upstream_loop_start disagrees with snapshot-derived game loop under "
                "the exact coordinate map"
            )
        loop_frame = _near_integer(upstream_loop_start, "mapped loop start")
        mapped_loop_end = upstream_loop_start + (game_end - game_loop_start) * ratio
        loop_end_frame = _near_integer(mapped_loop_end, "mapped loop end")
        if loop_frame < start_frame or loop_end_frame != end_frame or loop_end_frame <= loop_frame:
            raise ValueError("mapped upstream loop does not close exactly at mapped playback END")
    elif upstream_loop_start != 0.0:
        raise ValueError("one-shot snapshot source requires upstream_loop_start = 0.0")


def build_sidecar(manifest: dict, manifest_dir: Path) -> bytes:
    if manifest.get("schema") != SCHEMA:
        raise ValueError("unsupported or missing manifest schema")
    spc_path = _resolve_path(manifest.get("spc_file"), manifest_dir, "spc_file")
    spc = read_spc(spc_path)

    sources = manifest.get("sources")
    if not isinstance(sources, list) or not sources or len(sources) > MAX_SOURCES:
        raise ValueError("sources must be a nonempty list with at most 256 entries")

    parsed: list[dict[str, object]] = []
    seen_runtime_keys: set[tuple[int, int]] = set()
    snapshot_dir = current_directory_page(spc)

    for index, item in enumerate(sources):
        prefix = f"sources[{index}]"
        if not isinstance(item, dict):
            raise ValueError(f"{prefix} must be an object")
        _validate_admission(item.get("admission"), f"{prefix}.admission")

        source_number = _parse_uint(item.get("source_number"), 8, f"{prefix}.source_number")
        directory_page = (
            snapshot_dir
            if item.get("directory_page") is None
            else _parse_uint(item.get("directory_page"), 8, f"{prefix}.directory_page")
        )
        topology = derive_brr_topology(spc, directory_page, source_number)
        runtime_key = (source_number, int(topology["first_address"]))
        if runtime_key in seen_runtime_keys:
            raise ValueError(
                f"duplicate runtime source identity SRCN {source_number} + "
                f"0x{runtime_key[1]:04x}"
            )
        seen_runtime_keys.add(runtime_key)

        witness = topology["witness"]
        assert isinstance(witness, bytes)
        expected_brr_hash = _parse_sha256(
            item.get("game_brr_sha256"), f"{prefix}.game_brr_sha256"
        )
        actual_brr_hash = _sha256(witness)
        if actual_brr_hash != expected_brr_hash:
            raise ValueError(
                f"{prefix} BRR witness SHA-256 mismatch: snapshot={actual_brr_hash} "
                f"manifest={expected_brr_hash}"
            )

        pcm_path = _resolve_path(
            item.get("upstream_pcm_f32le"), manifest_dir, f"{prefix}.upstream_pcm_f32le"
        )
        pcm, frame_count = read_f32le(pcm_path)
        expected_pcm_hash = _parse_sha256(
            item.get("upstream_pcm_sha256"), f"{prefix}.upstream_pcm_sha256"
        )
        actual_pcm_hash = _sha256(pcm)
        if actual_pcm_hash != expected_pcm_hash:
            raise ValueError(
                f"{prefix} upstream PCM SHA-256 mismatch: file={actual_pcm_hash} "
                f"manifest={expected_pcm_hash}"
            )

        game_identity = _parse_identity(
            item.get("game_brr_identity"), f"{prefix}.game_brr_identity"
        )
        upstream_identity = _parse_identity(
            item.get("upstream_identity"), f"{prefix}.upstream_identity"
        )
        sample_rate = _parse_finite_positive(
            item.get("sample_rate_hz"), f"{prefix}.sample_rate_hz"
        )
        amplitude = _parse_finite_positive(
            item.get("game_pcm_units_per_source_unit"),
            f"{prefix}.game_pcm_units_per_source_unit",
        )
        game_origin = _parse_finite(item.get("game_origin"), f"{prefix}.game_origin")
        upstream_origin = _parse_finite(
            item.get("upstream_origin"), f"{prefix}.upstream_origin"
        )
        ratio = _parse_finite_positive(
            item.get("upstream_frames_per_game_sample"),
            f"{prefix}.upstream_frames_per_game_sample",
        )

        if bool(topology["loop_present"]):
            if item.get("upstream_loop_start") is None:
                raise ValueError(f"{prefix}.upstream_loop_start required for looping source")
            upstream_loop_start = _parse_finite(
                item.get("upstream_loop_start"), f"{prefix}.upstream_loop_start"
            )
        else:
            upstream_loop_start = _parse_finite(
                item.get("upstream_loop_start", 0.0), f"{prefix}.upstream_loop_start"
            )

        _validate_coordinate_map(
            frame_count=frame_count,
            game_origin=game_origin,
            upstream_origin=upstream_origin,
            ratio=ratio,
            topology=topology,
            upstream_loop_start=upstream_loop_start,
        )

        parsed.append(
            {
                "source_number": source_number,
                "topology": topology,
                "pcm": pcm,
                "frame_count": frame_count,
                "game_identity": game_identity,
                "upstream_identity": upstream_identity,
                "game_origin": game_origin,
                "upstream_origin": upstream_origin,
                "ratio": ratio,
                "upstream_loop_start": upstream_loop_start,
                "sample_rate": sample_rate,
                "amplitude": amplitude,
            }
        )

    table_end = HEADER_SIZE + len(parsed) * ENTRY_SIZE
    cursor = table_end
    for item in parsed:
        witness = item["topology"]["witness"]
        assert isinstance(witness, bytes)
        item["brr_offset"] = cursor
        cursor += len(witness)
        cursor = (cursor + 3) & ~3
        item["pcm_offset"] = cursor
        pcm = item["pcm"]
        assert isinstance(pcm, bytes)
        cursor += len(pcm)
    if cursor > 0xFFFFFFFF:
        raise ValueError("studio-source packet exceeds uint32 framing")

    out = bytearray(cursor)
    struct.pack_into("<IHHHHI", out, 0, MAGIC, VERSION, HEADER_SIZE, len(parsed), 0, cursor)

    for index, item in enumerate(parsed):
        topology = item["topology"]
        witness = topology["witness"]
        pcm = item["pcm"]
        assert isinstance(witness, bytes) and isinstance(pcm, bytes)
        entry = HEADER_SIZE + index * ENTRY_SIZE
        flags = FLAG_LOOP if topology["loop_present"] else 0
        struct.pack_into("<BBH", out, entry, item["source_number"], flags, topology["first_address"])
        struct.pack_into(
            "<IIIII",
            out,
            entry + 4,
            topology["block_count"],
            topology["loop_ordinal"],
            item["frame_count"],
            item["brr_offset"],
            item["pcm_offset"],
        )
        game_high, game_low = item["game_identity"]
        upstream_high, upstream_low = item["upstream_identity"]
        struct.pack_into(
            "<QQQQ",
            out,
            entry + 24,
            game_high,
            game_low,
            upstream_high,
            upstream_low,
        )
        struct.pack_into(
            "<dddddd",
            out,
            entry + 56,
            item["game_origin"],
            item["upstream_origin"],
            item["ratio"],
            item["upstream_loop_start"] if topology["loop_present"] else 0.0,
            item["sample_rate"],
            item["amplitude"],
        )
        struct.pack_into("<QQ", out, entry + 104, 0, 0)
        out[item["brr_offset"] : item["brr_offset"] + len(witness)] = witness
        out[item["pcm_offset"] : item["pcm_offset"] + len(pcm)] = pcm

    return bytes(out)


def _brr_headers_match_playback(brr: bytes, block_count: int, loop_present: bool) -> bool:
    if block_count <= 0 or len(brr) != block_count * BYTES_PER_BRR_BLOCK:
        return False
    for block in range(block_count):
        header = brr[block * BYTES_PER_BRR_BLOCK]
        end = bool(header & 0x01)
        loop = bool(header & 0x02)
        if block + 1 < block_count:
            if end:
                return False
        elif not end or loop != loop_present:
            return False
    return True


def parse_sidecar(data: bytes) -> list[dict[str, object]]:
    """Strict Python mirror of C++ packet/runtime admission for tests/audits."""
    if len(data) < HEADER_SIZE:
        raise ValueError("packet shorter than header")
    magic, version, header_size, count, reserved, declared = struct.unpack_from(
        "<IHHHHI", data, 0
    )
    if magic != MAGIC or version != VERSION or header_size != HEADER_SIZE:
        raise ValueError("invalid studio-source packet identity/version")
    if not count or count > MAX_SOURCES or reserved != 0 or declared != len(data):
        raise ValueError("invalid studio-source packet framing")
    expected_payload = HEADER_SIZE + count * ENTRY_SIZE
    if expected_payload > len(data):
        raise ValueError("entry table exceeds packet")

    result: list[dict[str, object]] = []
    seen: set[tuple[int, int]] = set()
    for index in range(count):
        entry = HEADER_SIZE + index * ENTRY_SIZE
        source_number, flags, first_address = struct.unpack_from("<BBH", data, entry)
        blocks, loop_ordinal, frames, brr_offset, pcm_offset = struct.unpack_from(
            "<IIIII", data, entry + 4
        )
        game_high, game_low, upstream_high, upstream_low = struct.unpack_from(
            "<QQQQ", data, entry + 24
        )
        if (
            flags & ~FLAG_LOOP
            or not blocks
            or blocks > SPC_RAM_SIZE // BYTES_PER_BRR_BLOCK
            or not frames
            or (game_high == 0 and game_low == 0)
            or (upstream_high == 0 and upstream_low == 0)
        ):
            raise ValueError("invalid source entry flags/counts/identities")
        loop_present = bool(flags & FLAG_LOOP)
        if loop_present:
            if loop_ordinal == NO_LOOP or loop_ordinal >= blocks:
                raise ValueError("invalid loop block ordinal")
        elif loop_ordinal != NO_LOOP:
            raise ValueError("one-shot entry carries loop ordinal")
        if struct.unpack_from("<QQ", data, entry + 104) != (0, 0):
            raise ValueError("nonzero reserved entry tail")

        key = (source_number, first_address)
        if key in seen:
            raise ValueError("duplicate runtime source identity")
        seen.add(key)

        brr_size = blocks * BYTES_PER_BRR_BLOCK
        if brr_offset != expected_payload or brr_offset + brr_size > len(data):
            raise ValueError("invalid BRR payload extent")
        brr = data[brr_offset : brr_offset + brr_size]
        if not _brr_headers_match_playback(brr, blocks, loop_present):
            raise ValueError("BRR witness headers disagree with packet playback topology")
        expected_payload += brr_size
        aligned = (expected_payload + 3) & ~3
        if aligned > len(data) or any(data[expected_payload:aligned]):
            raise ValueError("invalid nonzero alignment padding")
        expected_payload = aligned

        pcm_size = frames * 4
        if (
            pcm_size > 0xFFFFFFFF
            or pcm_offset != expected_payload
            or pcm_offset & 3
            or pcm_offset + pcm_size > len(data)
        ):
            raise ValueError("invalid PCM payload extent")
        pcm = data[pcm_offset : pcm_offset + pcm_size]
        for (sample,) in struct.iter_unpack("<f", pcm):
            if not math.isfinite(sample):
                raise ValueError("non-finite packet PCM")
        expected_payload += pcm_size

        values = struct.unpack_from("<dddddd", data, entry + 56)
        if not all(math.isfinite(value) for value in values):
            raise ValueError("non-finite packet coordinate/rate value")
        game_origin, upstream_origin, ratio, upstream_loop, rate, amplitude = values
        if ratio <= 0.0 or rate <= 0.0 or amplitude <= 0.0:
            raise ValueError("invalid positive packet parameter")
        if not loop_present and upstream_loop != 0.0:
            raise ValueError("one-shot packet carries upstream loop coordinate")

        result.append(
            {
                "source_number": source_number,
                "first_brr_block_address": first_address,
                "loop_present": loop_present,
                "brr_block_count": blocks,
                "loop_block_ordinal": loop_ordinal,
                "pcm_frame_count": frames,
                "brr_offset_bytes": brr_offset,
                "pcm_offset_bytes": pcm_offset,
                "game_brr_identity": {"high": game_high, "low": game_low},
                "upstream_identity": {"high": upstream_high, "low": upstream_low},
                "game_origin": game_origin,
                "upstream_origin": upstream_origin,
                "upstream_frames_per_game_sample": ratio,
                "upstream_loop_start": upstream_loop,
                "sample_rate_hz": rate,
                "game_pcm_units_per_source_unit": amplitude,
            }
        )

    if expected_payload != len(data):
        raise ValueError("trailing or unclaimed studio-source payload")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path, nargs="?")
    args = parser.parse_args()

    manifest_path = args.manifest.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    packet = build_sidecar(manifest, manifest_path.parent)
    output = args.output
    if output is None:
        name = manifest_path.name
        if name.endswith(".studiosrc.json"):
            output = manifest_path.with_name(name[:-5])
        else:
            output = Path(str(manifest_path) + ".studiosrc")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packet)
    print(f"wrote {output} ({len(packet)} bytes, {len(parse_sidecar(packet))} sources)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
