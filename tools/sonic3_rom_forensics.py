#!/usr/bin/env python3
"""Derived-only ROM provenance audit for the Sonic 3 research testbed.

The tool never redistributes ROM bytes. It reports hashes, offsets, printable
string classifications, padding topology, optional needle matches, and exact
cross-image regions/block matches. Its output is historical/provenance evidence
and is intentionally separate from musical blind-attribution features.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tempfile
from typing import Any, Iterable


PRINTABLE_LOW = 0x20
PRINTABLE_HIGH = 0x7E
DEFAULT_MIN_STRING = 5
DEFAULT_MIN_PADDING = 32
DEFAULT_MIN_EQUAL_RUN = 64
DEFAULT_BLOCK_SIZE = 4096
DEFAULT_MAX_ITEMS = 2000

_PATH_EXTENSIONS = (
    ".asm",
    ".bin",
    ".c",
    ".cpp",
    ".dat",
    ".h",
    ".inc",
    ".mid",
    ".midi",
    ".obj",
    ".pcm",
    ".raw",
    ".s3k",
    ".wav",
)
_TOOL_TOKENS = (
    "smps",
    "compiler",
    "assembler",
    "build",
    "convert",
    "export",
    "driver",
)


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _classify_string(text: str) -> str:
    lower = text.lower()
    if (
        "\\" in text
        or "/" in text
        or re.search(r"[A-Za-z]:[\\/]", text)
        or lower.endswith(_PATH_EXTENSIONS)
    ):
        return "path_or_filename_like"
    if any(token in lower for token in _TOOL_TOKENS):
        return "tool_or_build_like"
    if re.fullmatch(r"[A-Za-z][A-Za-z ._'\-]{5,}", text):
        words = [word for word in re.split(r"\s+", text.strip()) if word]
        if 2 <= len(words) <= 5 and all(word[:1].isupper() for word in words):
            return "person_or_title_like"
    return "printable"


def printable_strings(
    data: bytes,
    min_length: int = DEFAULT_MIN_STRING,
    max_items: int = DEFAULT_MAX_ITEMS,
) -> list[dict[str, Any]]:
    if min_length < 1:
        raise ValueError("min_length must be >= 1")
    if max_items < 0:
        raise ValueError("max_items must be >= 0")

    result: list[dict[str, Any]] = []
    start: int | None = None

    def emit(end: int) -> None:
        nonlocal start
        if start is None:
            return
        if end - start >= min_length and len(result) < max_items:
            text = data[start:end].decode("ascii", errors="strict")
            result.append(
                {
                    "offset": start,
                    "offset_hex": f"0x{start:X}",
                    "length": end - start,
                    "text": text,
                    "classification": _classify_string(text),
                }
            )
        start = None

    for index, value in enumerate(data):
        if PRINTABLE_LOW <= value <= PRINTABLE_HIGH:
            if start is None:
                start = index
        else:
            emit(index)
            if len(result) >= max_items:
                break
    if len(result) < max_items:
        emit(len(data))
    return result


def padding_runs(
    data: bytes,
    min_length: int = DEFAULT_MIN_PADDING,
    values: tuple[int, ...] = (0x00, 0xFF),
    max_items: int = DEFAULT_MAX_ITEMS,
) -> list[dict[str, Any]]:
    if min_length < 1:
        raise ValueError("min_length must be >= 1")
    allowed = set(values)
    result: list[dict[str, Any]] = []
    index = 0
    while index < len(data) and len(result) < max_items:
        value = data[index]
        if value not in allowed:
            index += 1
            continue
        end = index + 1
        while end < len(data) and data[end] == value:
            end += 1
        if end - index >= min_length:
            result.append(
                {
                    "start": index,
                    "start_hex": f"0x{index:X}",
                    "end": end,
                    "end_hex": f"0x{end:X}",
                    "length": end - index,
                    "byte": f"0x{value:02X}",
                }
            )
        index = end
    return result


def find_needles(
    data: bytes,
    needles: Iterable[str],
    max_items: int = DEFAULT_MAX_ITEMS,
) -> list[dict[str, Any]]:
    lowered = data.lower()
    result: list[dict[str, Any]] = []
    for needle in needles:
        cleaned = needle.strip()
        if not cleaned:
            continue
        encoded = cleaned.encode("ascii", errors="ignore")
        if not encoded:
            continue
        encoded_lower = encoded.lower()
        start = 0
        while len(result) < max_items:
            offset = lowered.find(encoded_lower, start)
            if offset < 0:
                break
            result.append(
                {
                    "needle": cleaned,
                    "offset": offset,
                    "offset_hex": f"0x{offset:X}",
                    "length": len(encoded),
                    "case_insensitive_ascii": True,
                }
            )
            start = offset + 1
    return result


def equal_offset_runs(
    left: bytes,
    right: bytes,
    min_length: int = DEFAULT_MIN_EQUAL_RUN,
    max_items: int = DEFAULT_MAX_ITEMS,
) -> list[dict[str, Any]]:
    if min_length < 1:
        raise ValueError("min_length must be >= 1")
    limit = min(len(left), len(right))
    result: list[dict[str, Any]] = []
    index = 0
    while index < limit and len(result) < max_items:
        if left[index] != right[index]:
            index += 1
            continue
        end = index + 1
        while end < limit and left[end] == right[end]:
            end += 1
        if end - index >= min_length:
            result.append(
                {
                    "start": index,
                    "start_hex": f"0x{index:X}",
                    "end": end,
                    "end_hex": f"0x{end:X}",
                    "length": end - index,
                    "sha256": _sha256(left[index:end]),
                }
            )
        index = end
    return result


def _block_records(data: bytes, block_size: int) -> list[tuple[int, bytes]]:
    if block_size < 1:
        raise ValueError("block_size must be >= 1")
    return [
        (offset, data[offset : offset + block_size])
        for offset in range(0, len(data) - block_size + 1, block_size)
    ]


def cross_offset_block_matches(
    left: bytes,
    right: bytes,
    block_size: int = DEFAULT_BLOCK_SIZE,
    max_items: int = DEFAULT_MAX_ITEMS,
) -> list[dict[str, Any]]:
    """Return unique exact full-block matches, including relocated blocks.

    Blocks repeated within either image are excluded because repeated fill,
    tables, or common code can otherwise swamp the result. The aim is to expose
    high-value relocated regions for later source-specific interpretation.
    """

    left_records = _block_records(left, block_size)
    right_records = _block_records(right, block_size)

    left_map: dict[str, list[int]] = {}
    right_map: dict[str, list[int]] = {}
    for offset, block in left_records:
        left_map.setdefault(_sha256(block), []).append(offset)
    for offset, block in right_records:
        right_map.setdefault(_sha256(block), []).append(offset)

    result: list[dict[str, Any]] = []
    for digest in sorted(set(left_map) & set(right_map)):
        left_offsets = left_map[digest]
        right_offsets = right_map[digest]
        if len(left_offsets) != 1 or len(right_offsets) != 1:
            continue
        left_offset = left_offsets[0]
        right_offset = right_offsets[0]
        if left_offset == right_offset:
            continue
        result.append(
            {
                "left_offset": left_offset,
                "left_offset_hex": f"0x{left_offset:X}",
                "right_offset": right_offset,
                "right_offset_hex": f"0x{right_offset:X}",
                "length": block_size,
                "sha256": digest,
            }
        )
        if len(result) >= max_items:
            break
    return result


def _entropy(data: bytes) -> float:
    if not data:
        return 0.0
    counts = [0] * 256
    for value in data:
        counts[value] += 1
    total = len(data)
    entropy = 0.0
    for count in counts:
        if count:
            probability = count / total
            entropy -= probability * math.log2(probability)
    return entropy


def entropy_windows(data: bytes, window_size: int = 4096) -> dict[str, Any]:
    if window_size < 1:
        raise ValueError("window_size must be >= 1")
    values = [
        _entropy(data[offset : offset + window_size])
        for offset in range(0, len(data), window_size)
    ]
    if not values:
        return {"window_size": window_size, "count": 0}
    return {
        "window_size": window_size,
        "count": len(values),
        "min": min(values),
        "max": max(values),
        "mean": sum(values) / len(values),
    }


def scan_bytes(
    data: bytes,
    *,
    source_name: str,
    needles: Iterable[str] = (),
    min_string: int = DEFAULT_MIN_STRING,
    min_padding: int = DEFAULT_MIN_PADDING,
    max_items: int = DEFAULT_MAX_ITEMS,
) -> dict[str, Any]:
    return {
        "source": source_name,
        "size": len(data),
        "sha256": _sha256(data),
        "printable_strings": printable_strings(data, min_string, max_items),
        "needle_matches": find_needles(data, needles, max_items),
        "padding_runs": padding_runs(data, min_padding, max_items=max_items),
        "entropy_summary": entropy_windows(data),
    }


def compare_bytes(
    left: bytes,
    right: bytes,
    *,
    min_equal_run: int = DEFAULT_MIN_EQUAL_RUN,
    block_size: int = DEFAULT_BLOCK_SIZE,
    max_items: int = DEFAULT_MAX_ITEMS,
) -> dict[str, Any]:
    shared_prefix = 0
    prefix_limit = min(len(left), len(right))
    while shared_prefix < prefix_limit and left[shared_prefix] == right[shared_prefix]:
        shared_prefix += 1

    shared_suffix = 0
    while (
        shared_suffix < prefix_limit - shared_prefix
        and left[len(left) - 1 - shared_suffix] == right[len(right) - 1 - shared_suffix]
    ):
        shared_suffix += 1

    return {
        "left_sha256": _sha256(left),
        "right_sha256": _sha256(right),
        "same_size": len(left) == len(right),
        "shared_prefix_bytes": shared_prefix,
        "shared_suffix_bytes": shared_suffix,
        "equal_offset_runs": equal_offset_runs(
            left, right, min_length=min_equal_run, max_items=max_items
        ),
        "unique_relocated_exact_blocks": cross_offset_block_matches(
            left, right, block_size=block_size, max_items=max_items
        ),
        "claim_boundary": (
            "Exact binary continuity/relocation is provenance evidence. It does not "
            "identify composition, arrangement, programming, or authorship without "
            "source-specific interpretation."
        ),
    }


def _read_needles(values: list[str], needle_file: Path | None) -> list[str]:
    result = list(values)
    if needle_file is not None:
        result.extend(needle_file.read_text(encoding="utf-8").splitlines())
    deduped: list[str] = []
    seen: set[str] = set()
    for value in result:
        cleaned = value.strip()
        key = cleaned.casefold()
        if not cleaned or key in seen:
            continue
        seen.add(key)
        deduped.append(cleaned)
    return deduped


def self_test() -> None:
    left = (
        b"HEADER\x00"
        b"C:\\sound\\NAGAO\\AIZ1.MID\x00"
        b"SMPS exporter 1.0\x00"
        + b"\xFF" * 40
        + b"UNIQUE-BLOCK-AAAA"
        + b"\x10\x11\x12"
        + b"TAIL" * 20
    )
    right = (
        b"HEADER\x00"
        b"different\x00"
        + b"\xFF" * 40
        + b"UNIQUE-BLOCK-AAAA"
        + b"\x99\x98"
        + b"TAIL" * 20
    )

    strings = printable_strings(left, min_length=5)
    assert any(item["classification"] == "path_or_filename_like" for item in strings)
    assert any(item["classification"] == "tool_or_build_like" for item in strings)

    pads = padding_runs(left, min_length=32)
    assert any(item["byte"] == "0xFF" and item["length"] >= 40 for item in pads)

    matches = find_needles(left, ["nagao", "missing"])
    assert len(matches) == 1
    assert matches[0]["needle"] == "nagao"

    comparison = compare_bytes(left, right, min_equal_run=8, block_size=8)
    assert comparison["shared_prefix_bytes"] == len(b"HEADER\x00")
    assert comparison["equal_offset_runs"]

    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "fixture.bin"
        path.write_bytes(left)
        scanned = scan_bytes(left, source_name=str(path), needles=["AIZ1"])
        assert scanned["needle_matches"]
        assert scanned["sha256"] == _sha256(left)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path, nargs="?")
    parser.add_argument("--compare", type=Path)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--needle", action="append", default=[])
    parser.add_argument("--needle-file", type=Path)
    parser.add_argument("--min-string", type=int, default=DEFAULT_MIN_STRING)
    parser.add_argument("--min-padding", type=int, default=DEFAULT_MIN_PADDING)
    parser.add_argument("--min-equal-run", type=int, default=DEFAULT_MIN_EQUAL_RUN)
    parser.add_argument("--block-size", type=int, default=DEFAULT_BLOCK_SIZE)
    parser.add_argument("--max-items", type=int, default=DEFAULT_MAX_ITEMS)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        self_test()
        print("sonic3_rom_forensics self-test passed")
        return 0

    if args.rom is None:
        parser.error("rom is required unless --self-test is used")

    if args.max_items < 0:
        parser.error("--max-items must be >= 0")

    needles = _read_needles(args.needle, args.needle_file)
    left = args.rom.read_bytes()
    result: dict[str, Any] = {
        "tool": "sonic3-rom-derived-provenance-audit",
        "mode": "forensic-only",
        "musical_blind_firewall": (
            "Do not feed these strings, offsets, filenames, IDs, or binary-layout "
            "features into the musical blind-attribution benchmark. Freeze musical "
            "outputs before using this evidence for historical validation."
        ),
        "scan": scan_bytes(
            left,
            source_name=str(args.rom),
            needles=needles,
            min_string=args.min_string,
            min_padding=args.min_padding,
            max_items=args.max_items,
        ),
    }

    if args.compare is not None:
        right = args.compare.read_bytes()
        result["comparison_source"] = str(args.compare)
        result["comparison"] = compare_bytes(
            left,
            right,
            min_equal_run=args.min_equal_run,
            block_size=args.block_size,
            max_items=args.max_items,
        )

    text = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
