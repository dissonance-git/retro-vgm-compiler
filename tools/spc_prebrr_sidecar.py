#!/usr/bin/env python3
"""Build one `.spc.prebrr` packet from already-approved prepared PCM.

This tool does no discovery, resampling, gain fitting or provenance promotion.
Its input WAVs must already represent the exact game sample grid *before BRR
encoding*. When an SPC is supplied, SRCN -> directory -> first BRR address and
BRR extent are derived directly from the snapshot and checked against the WAV.

Manifest example:

{
  "schema": "spc-prebrr-sidecar-manifest-001",
  "spc_file": "music.spc",
  "sources": [
    {
      "source_number": 12,
      "prepared_pcm_wav": "samples/snare-prebrr.wav"
    }
  ]
}

`first_brr_block_address` may still be supplied, but with `spc_file` it becomes
an assertion and must equal the snapshot-derived value. Each WAV must be mono,
16-bit integer PCM, and contain exactly sixteen frames per shipped BRR block.
No implicit conversion occurs at this final evidence boundary.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import wave


MAGIC = 0x52425250  # PRBR little-endian
VERSION = 1
HEADER_SIZE = 16
ENTRY_SIZE = 16
SAMPLES_PER_BLOCK = 16
MAX_SOURCES = 256

SPC_SIGNATURE = b"SNES-SPC700 Sound File Data"
SPC_RAM_OFFSET = 0x100
SPC_RAM_SIZE = 0x10000
SPC_DSP_OFFSET = 0x10100
SPC_DSP_SIZE = 0x80
SPC_MIN_SIZE = 0x10180
SPC_DIR_REGISTER = 0x5D
MAX_BRR_SCAN_BLOCKS = SPC_RAM_SIZE // 9


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


def _resolve_path(value: object, base: Path, label: str) -> Path:
    if not isinstance(value, str) or not value:
        raise ValueError(f"{label} must be a path string")
    path = Path(value)
    return path if path.is_absolute() else base / path


def read_prepared_pcm_wav(path: Path) -> bytes:
    with wave.open(str(path), "rb") as handle:
        if handle.getnchannels() != 1:
            raise ValueError(f"prepared PCM must be mono: {path}")
        if handle.getsampwidth() != 2:
            raise ValueError(f"prepared PCM must be 16-bit integer: {path}")
        if handle.getcomptype() != "NONE":
            raise ValueError(f"prepared PCM must be uncompressed WAV: {path}")
        frames = handle.getnframes()
        if frames == 0 or frames % SAMPLES_PER_BLOCK != 0:
            raise ValueError(
                f"prepared PCM frame count must be a nonzero multiple of {SAMPLES_PER_BLOCK}: {path}"
            )
        payload = handle.readframes(frames)
        if len(payload) != frames * 2:
            raise ValueError(f"truncated prepared PCM WAV: {path}")
        return payload


def read_spc(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < SPC_MIN_SIZE or not data.startswith(SPC_SIGNATURE):
        raise ValueError(f"not a complete SNES-SPC700 snapshot: {path}")
    return data


def spc_ram_byte(spc: bytes, address: int) -> int:
    return spc[SPC_RAM_OFFSET + (address & 0xFFFF)]


def spc_source_start_address(spc: bytes, source_number: int) -> int:
    if len(spc) < SPC_MIN_SIZE:
        raise ValueError("truncated SPC")
    directory_page = spc[SPC_DSP_OFFSET + SPC_DIR_REGISTER]
    directory_address = ((directory_page << 8) + source_number * 4) & 0xFFFF
    lo = spc_ram_byte(spc, directory_address)
    hi = spc_ram_byte(spc, directory_address + 1)
    return lo | (hi << 8)


def scan_brr_extent(spc: bytes, first_address: int) -> int:
    address = first_address & 0xFFFF
    for block in range(1, MAX_BRR_SCAN_BLOCKS + 1):
        header = spc_ram_byte(spc, address)
        if header & 0x01:
            return block
        address = (address + 9) & 0xFFFF
    raise ValueError(
        f"BRR sample at 0x{first_address:04x} did not reach END within bounded RAM scan"
    )


def build_sidecar(manifest: dict, manifest_dir: Path) -> bytes:
    if manifest.get("schema") != "spc-prebrr-sidecar-manifest-001":
        raise ValueError("unsupported or missing manifest schema")
    sources = manifest.get("sources")
    if not isinstance(sources, list) or len(sources) > MAX_SOURCES:
        raise ValueError("sources must be a list with at most 256 entries")

    spc: bytes | None = None
    if manifest.get("spc_file") is not None:
        spc = read_spc(_resolve_path(manifest.get("spc_file"), manifest_dir, "spc_file"))

    parsed: list[tuple[int, int, bytes]] = []
    seen: set[int] = set()
    for index, item in enumerate(sources):
        if not isinstance(item, dict):
            raise ValueError(f"sources[{index}] must be an object")
        source_number = _parse_uint(item.get("source_number"), 8, f"sources[{index}].source_number")
        if source_number in seen:
            raise ValueError(f"duplicate source_number: {source_number}")
        seen.add(source_number)

        asserted_address = item.get("first_brr_block_address")
        if spc is not None:
            first_address = spc_source_start_address(spc, source_number)
            if asserted_address is not None:
                asserted = _parse_uint(
                    asserted_address, 16, f"sources[{index}].first_brr_block_address"
                )
                if asserted != first_address:
                    raise ValueError(
                        f"sources[{index}] BRR address 0x{asserted:04x} disagrees with "
                        f"SPC directory 0x{first_address:04x}"
                    )
        else:
            first_address = _parse_uint(
                asserted_address, 16, f"sources[{index}].first_brr_block_address"
            )

        wav_path = _resolve_path(
            item.get("prepared_pcm_wav"),
            manifest_dir,
            f"sources[{index}].prepared_pcm_wav",
        )
        pcm = read_prepared_pcm_wav(wav_path)
        pcm_blocks = len(pcm) // (SAMPLES_PER_BLOCK * 2)
        if spc is not None:
            shipped_blocks = scan_brr_extent(spc, first_address)
            if pcm_blocks != shipped_blocks:
                raise ValueError(
                    f"sources[{index}] prepared PCM has {pcm_blocks} blocks but exact SPC BRR "
                    f"extent has {shipped_blocks}; preparation/trim mapping is not exact"
                )
        parsed.append((source_number, first_address, pcm))

    payload_offset = HEADER_SIZE + len(parsed) * ENTRY_SIZE
    total_size = payload_offset + sum(len(pcm) for _, _, pcm in parsed)
    if total_size > 0xFFFFFFFF:
        raise ValueError("sidecar exceeds uint32 packet size")

    out = bytearray(total_size)
    struct.pack_into("<IHHHHI", out, 0, MAGIC, VERSION, HEADER_SIZE, len(parsed), 0, total_size)

    cursor = payload_offset
    for index, (source_number, first_address, pcm) in enumerate(parsed):
        block_count = len(pcm) // (SAMPLES_PER_BLOCK * 2)
        entry = HEADER_SIZE + index * ENTRY_SIZE
        struct.pack_into(
            "<BBHIII",
            out,
            entry,
            source_number,
            0,
            first_address,
            block_count,
            cursor,
            len(pcm),
        )
        out[cursor : cursor + len(pcm)] = pcm
        cursor += len(pcm)

    return bytes(out)


def parse_sidecar(data: bytes) -> list[dict[str, int]]:
    """Strict parser used by tests/audits; mirrors the C++ transport contract."""
    if len(data) < HEADER_SIZE:
        raise ValueError("sidecar shorter than header")
    magic, version, header_size, count, reserved, declared = struct.unpack_from("<IHHHHI", data, 0)
    if magic != MAGIC or version != VERSION or header_size != HEADER_SIZE:
        raise ValueError("invalid sidecar identity/version")
    if reserved != 0 or declared != len(data):
        raise ValueError("invalid sidecar framing")
    table_end = HEADER_SIZE + count * ENTRY_SIZE
    if table_end > len(data):
        raise ValueError("entry table exceeds packet")

    result: list[dict[str, int]] = []
    seen: set[int] = set()
    for index in range(count):
        entry = HEADER_SIZE + index * ENTRY_SIZE
        source_number, reserved8, first_address, blocks, pcm_offset, pcm_bytes = struct.unpack_from(
            "<BBHIII", data, entry
        )
        if reserved8 != 0 or source_number in seen:
            raise ValueError("duplicate source or nonzero reserved entry byte")
        seen.add(source_number)
        expected = blocks * SAMPLES_PER_BLOCK * 2
        if blocks == 0 or pcm_bytes != expected or pcm_offset < table_end:
            raise ValueError("invalid PCM extent")
        if pcm_offset & 1 or pcm_offset + pcm_bytes > len(data):
            raise ValueError("invalid PCM offset")
        result.append(
            {
                "source_number": source_number,
                "first_brr_block_address": first_address,
                "block_count": blocks,
                "pcm_offset_bytes": pcm_offset,
                "pcm_size_bytes": pcm_bytes,
            }
        )
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
        if name.endswith(".prebrr.json"):
            output = manifest_path.with_name(name[:-5])
        else:
            output = Path(str(manifest_path) + ".prebrr")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packet)
    print(f"wrote {output} ({len(packet)} bytes, {len(parse_sidecar(packet))} sources)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
