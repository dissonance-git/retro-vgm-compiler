#!/usr/bin/env python3
"""Inspect structural state shared across a collection of SPC snapshots.

This tool intentionally ignores ID666/xid6 metadata. It looks only at the
machine snapshot carried by an SPC file: 64 KiB SPC700 RAM and the S-DSP
register image.

The first use is corpus reasoning: identify stable RAM regions, source-directory
state, and exact BRR objects that persist or move between SRCN slots across
multiple songs from the same game/driver.

This is not a driver parser and does not infer musical roles.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import pathlib
import zipfile
from dataclasses import dataclass
from typing import Iterable

SPC_SIGNATURE = b"SNES-SPC700 Sound File Data"
RAM_OFFSET = 0x100
RAM_SIZE = 0x10000
DSP_OFFSET = 0x10100
DSP_SIZE = 0x80
DSP_DIR = 0x5D


@dataclass(frozen=True)
class SpcSnapshot:
    name: str
    ram: bytes
    dsp: bytes


def load_spc(name: str, data: bytes) -> SpcSnapshot:
    if not data.startswith(SPC_SIGNATURE):
        raise ValueError(f"{name}: not an SPC file")
    if len(data) < DSP_OFFSET + DSP_SIZE:
        raise ValueError(f"{name}: truncated SPC file")
    return SpcSnapshot(
        name=name,
        ram=data[RAM_OFFSET : RAM_OFFSET + RAM_SIZE],
        dsp=data[DSP_OFFSET : DSP_OFFSET + DSP_SIZE],
    )


def iter_inputs(path: pathlib.Path) -> Iterable[SpcSnapshot]:
    if path.is_dir():
        for item in sorted(path.rglob("*.spc")):
            yield load_spc(str(item), item.read_bytes())
        return

    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            for name in sorted(archive.namelist()):
                if name.lower().endswith(".spc"):
                    yield load_spc(name, archive.read(name))
        return

    yield load_spc(str(path), path.read_bytes())


def identical_ram_ranges(snapshots: list[SpcSnapshot], minimum: int) -> list[dict]:
    if not snapshots:
        return []

    base = snapshots[0].ram
    same = [all(s.ram[i] == base[i] for s in snapshots[1:]) for i in range(RAM_SIZE)]
    ranges: list[dict] = []
    start = None
    for index, matches in enumerate(same + [False]):
        if matches and start is None:
            start = index
        elif not matches and start is not None:
            length = index - start
            if length >= minimum:
                ranges.append({"start": start, "end_exclusive": index, "length": length})
            start = None
    ranges.sort(key=lambda row: row["length"], reverse=True)
    return ranges


def decode_brr_object(ram: bytes, start: int) -> bytes | None:
    """Return raw BRR blocks reachable from start through the first END block.

    Directory entries in unused portions of RAM can contain arbitrary-looking
    values. A returned object therefore proves only that this byte sequence is
    reachable from the snapshot directory entry and terminates like BRR. It is
    not by itself proof that the game actually triggers that SRCN during the
    captured song.
    """

    if start <= 0 or start >= RAM_SIZE:
        return None

    position = start
    seen: set[int] = set()
    raw = bytearray()
    for _ in range(4096):
        if position in seen or position + 9 > RAM_SIZE:
            return None
        seen.add(position)
        header = ram[position]
        raw.extend(ram[position : position + 9])
        position += 9
        if header & 0x01:  # BRR END flag
            return bytes(raw)
    return None


def directory_objects(snapshot: SpcSnapshot) -> list[dict]:
    directory_page = snapshot.dsp[DSP_DIR]
    directory_base = directory_page << 8
    result: list[dict] = []

    for srcn in range(256):
        offset = directory_base + srcn * 4
        if offset + 4 > RAM_SIZE:
            break
        start = snapshot.ram[offset] | (snapshot.ram[offset + 1] << 8)
        loop = snapshot.ram[offset + 2] | (snapshot.ram[offset + 3] << 8)
        raw = decode_brr_object(snapshot.ram, start)
        if raw is None:
            continue
        result.append(
            {
                "srcn": srcn,
                "start": start,
                "loop": loop,
                "length": len(raw),
                "sha256": hashlib.sha256(raw).hexdigest(),
            }
        )
    return result


def analyze(snapshots: list[SpcSnapshot], minimum_range: int) -> dict:
    if not snapshots:
        raise ValueError("no SPC files found")

    object_tracks: dict[str, set[str]] = collections.defaultdict(set)
    object_slots: dict[str, collections.Counter[int]] = collections.defaultdict(collections.Counter)
    object_lengths: dict[str, int] = {}
    per_file: list[dict] = []

    for snapshot in snapshots:
        objects = directory_objects(snapshot)
        for obj in objects:
            digest = obj["sha256"]
            object_tracks[digest].add(snapshot.name)
            object_slots[digest][obj["srcn"]] += 1
            object_lengths[digest] = obj["length"]

        voices = []
        for voice in range(8):
            base = voice * 0x10
            voices.append(
                {
                    "voice": voice,
                    "srcn": snapshot.dsp[base + 0x04],
                    "pitch": snapshot.dsp[base + 0x02] | (snapshot.dsp[base + 0x03] << 8),
                    "volume_left_raw": snapshot.dsp[base + 0x00],
                    "volume_right_raw": snapshot.dsp[base + 0x01],
                    "adsr1": snapshot.dsp[base + 0x05],
                    "adsr2": snapshot.dsp[base + 0x06],
                    "gain": snapshot.dsp[base + 0x07],
                }
            )

        per_file.append(
            {
                "name": snapshot.name,
                "directory_page": snapshot.dsp[DSP_DIR],
                "directory_base": snapshot.dsp[DSP_DIR] << 8,
                "voices_at_snapshot": voices,
                "directory_object_count": len(objects),
            }
        )

    shared_objects = []
    for digest, tracks in object_tracks.items():
        shared_objects.append(
            {
                "sha256": digest,
                "length": object_lengths[digest],
                "track_count": len(tracks),
                "srcn_histogram": [
                    {"srcn": slot, "count": count}
                    for slot, count in object_slots[digest].most_common()
                ],
            }
        )
    shared_objects.sort(key=lambda row: (row["track_count"], row["length"]), reverse=True)

    ranges = identical_ram_ranges(snapshots, minimum_range)
    identical_count = sum(row["length"] for row in ranges)
    # The ranges above omit short identical runs, so calculate the exact count separately.
    base = snapshots[0].ram
    exact_identical_count = sum(
        all(snapshot.ram[i] == base[i] for snapshot in snapshots[1:])
        for i in range(RAM_SIZE)
    )

    return {
        "spc_count": len(snapshots),
        "metadata_ignored": True,
        "dsp_directory_pages": dict(
            collections.Counter(snapshot.dsp[DSP_DIR] for snapshot in snapshots)
        ),
        "identical_ram_byte_count": exact_identical_count,
        "identical_ram_fraction": exact_identical_count / RAM_SIZE,
        "identical_ram_ranges_minimum_length": minimum_range,
        "identical_ram_ranges": ranges,
        "shared_directory_brr_objects": shared_objects,
        "files": per_file,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=pathlib.Path, help="SPC file, directory, or ZIP containing SPC files")
    parser.add_argument("--minimum-range", type=int, default=16)
    parser.add_argument("--pretty", action="store_true")
    args = parser.parse_args()

    snapshots = list(iter_inputs(args.input))
    report = analyze(snapshots, args.minimum_range)
    print(json.dumps(report, indent=2 if args.pretty else None, sort_keys=True))


if __name__ == "__main__":
    main()
