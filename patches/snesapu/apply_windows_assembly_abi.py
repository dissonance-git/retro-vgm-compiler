#!/usr/bin/env python3
"""Select the historical Win32/stdcall ABI for private SNESAPU assembly builds."""

from __future__ import annotations

import argparse
from pathlib import Path


ANCHOR = "CPU     386\nBITS    32\n"
REPLACEMENT = """CPU     386
BITS    32

; The historical Windows SNESAPU build pairs MSVC /Gz with stdcall-decorated
; NASM procedures. Keep that ABI selection inside the reconstructed source so
; every private builder links the same symbols instead of depending on ambient
; assembler command-line defines.
%ifndef WIN32
%define WIN32
%endif
%ifndef STDCALL
%define STDCALL
%endif
"""


def patch_one(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(ANCHOR)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one CPU/BITS ABI anchor, found {count}")
    path.write_text(text.replace(ANCHOR, REPLACEMENT, 1), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("spcplay_root", type=Path)
    root = parser.parse_args().spcplay_root.resolve()
    source = root / "snesapu.dll"
    for name in ("APU.asm", "DSP.asm", "SPC700.asm"):
        patch_one(source / name)
    print("SNESAPU Win32/stdcall assembly ABI selected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
