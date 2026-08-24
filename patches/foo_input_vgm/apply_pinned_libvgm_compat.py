#!/usr/bin/env python3
"""Align preserved foo_input_vgm 0.31 with the pinned pre-SameBoy libvgm API.

foo_input_vgm 0.31 names the Game Boy public core header ``gbintf.h``.  The
private build intentionally pins libvgm at 64e1de284e9a4305c54dd162ee8c33539a9bc0d1,
where the same public DMG declaration and option bits still live in ``gb.h``.
Patch only that include name instead of advancing libvgm and invalidating the
source-aware patch/verification baseline.
"""

from __future__ import annotations

import argparse
from pathlib import Path


OLD_HEADER = "emu/cores/gbintf.h"
PINNED_HEADER = "emu/cores/gb.h"
CURRENT_VSU_OPTION = "OPT_VSU_WRAM_WRT_WHILE_ON"
PINNED_VSU_OPTION = "OPT_VST_WRAM_WRT_WHILE_ON"


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def patch_header(path: Path) -> None:
    raw = path.read_bytes()
    text, encoding, bom = decode_source(raw)
    count = text.count(OLD_HEADER)
    if count != 1:
        raise RuntimeError(
            f"pinned libvgm Game Boy header compatibility: expected exactly one "
            f"{OLD_HEADER!r} in {path}, found {count}"
        )
    encoded = text.replace(OLD_HEADER, PINNED_HEADER, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched pinned libvgm Game Boy header compatibility: {path}")


def patch_token(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    text, encoding, bom = decode_source(raw)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"{label}: expected exactly one {old!r} in {path}, found {count}"
        )
    encoded = text.replace(old, new, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    root = parser.parse_args().source_dir.resolve()
    patch_header(root / "my_cfg_var.h")
    patch_header(root / "my_view_core_options.cpp")
    patch_token(
        root / "my_view_core_options.cpp",
        CURRENT_VSU_OPTION,
        PINNED_VSU_OPTION,
        "pinned libvgm Virtual Boy option spelling",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
