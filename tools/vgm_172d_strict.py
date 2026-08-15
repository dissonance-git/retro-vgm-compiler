#!/usr/bin/env python3
"""Strict negative validation for the current VGM 1.72d closure surface.

`vgm_corpus_audit.py` owns general structure/timing. This companion layer owns
fields which the upstream VGM spec explicitly reserves and therefore requires
to be zero. Keeping the negative law separate makes it easy to distinguish a
file which is structurally walkable from one which is strictly conformant.
"""

from __future__ import annotations

import importlib.util
import pathlib
from typing import Any


_HERE = pathlib.Path(__file__).resolve().parent
_SPEC = importlib.util.spec_from_file_location("vgm_corpus_audit", _HERE / "vgm_corpus_audit.py")
assert _SPEC is not None and _SPEC.loader is not None
_BASE = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_BASE)


def _header_byte(raw: bytes, data_start: int, offset: int) -> int:
    """Return a header byte, respecting the VGM overlap-as-zero rule."""
    if offset >= len(raw) or (data_start < 0x100 and offset >= data_start):
        return 0
    return raw[offset]


def reserved_header_errors(raw: bytes, version: int, data_start: int) -> list[str]:
    errors: list[str] = []

    # Bytes that remain reserved in the complete currently documented 1.72
    # header surface. Header bytes overlapped by earlier sound data are defined
    # as zero and are therefore ignored by _header_byte().
    for offset in (0x7D, 0x97, 0xD7):
        value = _header_byte(raw, data_start, offset)
        if value:
            errors.append(f"reserved header byte 0x{offset:02X} is 0x{value:02X}, expected 00")

    for offset in range(0xE8, 0x100):
        value = _header_byte(raw, data_start, offset)
        if value:
            errors.append(f"reserved header byte 0x{offset:02X} is 0x{value:02X}, expected 00")

    # Explicitly documented reserved flag bits.
    masks = (
        (0x2B, 0xE0, "SN76489 flags bits 5-7"),
        (0x79, 0xE0, "AY8910 flags bits 5-7"),
        (0x7A, 0xE0, "YM2203/AY flags bits 5-7"),
        (0x7B, 0xE0, "YM2608/AY flags bits 5-7"),
        (0x94, 0xF0, "MSM6258 flags bits 4-7"),
        (0x95, 0xF8, "K054539 flags bits 3-7"),
    )
    for offset, mask, label in masks:
        value = _header_byte(raw, data_start, offset)
        if value & mask:
            errors.append(
                f"{label} are nonzero at 0x{offset:02X}: 0x{value & mask:02X}"
            )

    # Current 1.72d is the current upstream 1.72 beta surface. A future upstream
    # revision is deliberately not silently certified by this validator.
    if version > _BASE.VGM_172_BETA:
        errors.append(
            f"VGM version 0x{version:03X} is newer than the 1.72d closure surface"
        )

    return errors


def strict_audit(path: pathlib.Path) -> dict[str, Any]:
    report = _BASE.audit(path)
    if not path.exists():
        return report
    raw = _BASE.load_vgm(path)
    if raw[:4] != b"Vgm " or len(raw) < 0x38:
        return report
    version = _BASE.u32(raw, 0x08)
    data_start = _BASE.data_offset(raw, version)
    strict_errors = reserved_header_errors(raw, version, data_start)
    report["strict_172d"] = not strict_errors
    report["strict_errors"] = strict_errors
    report["valid"] = bool(report.get("valid", False)) and not strict_errors
    report["errors"] = list(report.get("errors", [])) + strict_errors
    return report


# Transport-width truth table for command families whose sizes are fixed by the
# current upstream specification. 0x67 and 0x68 are variable/special and are
# tested through the main auditor instead of this fixed-width table.
def expected_fixed_operand_count(command: int, version: int = _BASE.VGM_172_BETA) -> int | None:
    if 0x30 <= command <= 0x3F:
        return 1
    if command == 0x40:
        return 2 if version >= _BASE.VGM_172_BETA else None
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
