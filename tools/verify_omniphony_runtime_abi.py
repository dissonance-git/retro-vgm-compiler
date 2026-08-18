#!/usr/bin/env python3
"""Load the built Omniphony source DLL and prove its runtime contract.

The package verifier can prove that the named PE exports exist, but the host also
requires the version functions to execute and report a compatible contract. This
Windows-only build gate loads the exact DLL that will be packaged, resolves all
symbols used by omniphony_dynamic_backend_loader, and calls the zero-argument
version functions before either foobar component is packaged.
"""

from __future__ import annotations

import argparse
import ctypes
from pathlib import Path
import struct
from typing import Any


EXPECTED_ABI_MAJOR = 0
MINIMUM_ABI_MINOR = 4
REQUIRED_SYMBOLS = (
    "omniphony_source_abi_major",
    "omniphony_source_abi_minor",
    "omniphony_source_create",
    "omniphony_source_destroy",
    "omniphony_source_reset",
    "omniphony_source_set_mix_budget",
    "omniphony_source_process_events_f32",
)


def verify_api(api: Any) -> tuple[int, int]:
    missing = [name for name in REQUIRED_SYMBOLS if not hasattr(api, name)]
    if missing:
        raise AssertionError(f"Omniphony runtime missing required symbols: {missing}")

    major_fn = api.omniphony_source_abi_major
    minor_fn = api.omniphony_source_abi_minor
    major_fn.argtypes = []
    major_fn.restype = ctypes.c_uint32
    minor_fn.argtypes = []
    minor_fn.restype = ctypes.c_uint32

    major = int(major_fn())
    minor = int(minor_fn())
    if major != EXPECTED_ABI_MAJOR or minor < MINIMUM_ABI_MINOR:
        raise AssertionError(
            "Omniphony runtime contract mismatch: "
            f"required {EXPECTED_ABI_MAJOR}.{MINIMUM_ABI_MINOR}+ within major, "
            f"got {major}.{minor}"
        )
    return major, minor


def load_and_verify(path: Path) -> tuple[int, int]:
    if struct.calcsize("P") != 8:
        raise RuntimeError(
            "Omniphony runtime validation must run under 64-bit Python so the x64 DLL can load"
        )
    win_dll = getattr(ctypes, "WinDLL", None)
    if win_dll is None:
        raise RuntimeError("Omniphony runtime validation requires Windows")
    if not path.is_file():
        raise RuntimeError(f"Omniphony source DLL missing: {path}")

    try:
        api = win_dll(str(path))
    except OSError as exc:
        raise RuntimeError(f"could not load Omniphony source DLL {path}: {exc}") from exc
    return verify_api(api)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dll", type=Path)
    args = parser.parse_args()

    major, minor = load_and_verify(args.dll.resolve())
    print(f"Omniphony source runtime contract verified: {major}.{minor}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
