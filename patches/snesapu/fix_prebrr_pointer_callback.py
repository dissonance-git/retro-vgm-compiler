#!/usr/bin/env python3
"""Upgrade the generated spcplayer child callback to the calibrated 5-argument ABI.

Run immediately after apply_prebrr_transport.py. This is kept as a separate
post-patch guard so older staged foo_snesapu trees that already received the v2
transport can be repaired without reapplying or fuzzily rewriting that larger
patch.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    try:
        text = raw.decode("utf-8-sig")
        encoding = "utf-8"
        bom = raw.startswith(b"\xef\xbb\xbf")
    except UnicodeDecodeError:
        text = raw.decode("cp932")
        encoding = "cp932"
        bom = False
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    args = parser.parse_args()
    child_cpp = args.foo_snesapu_root.resolve() / "spcplayer" / "main.cpp"

    replace_once(
        child_cpp,
        """using RetroPreBrrCallback = u32 (__stdcall *)(void*, u32, u32, s16*);
using RetroSetPreBrrProvider = void (__stdcall *)(RetroPreBrrCallback, void*);

static u32 __stdcall retro_prebrr_callback(void* user, u32 srcn, u32 brr_addr, s16* out16)
{
    return RetroPreBrrRuntime::callback(user, srcn, brr_addr, out16) ? 1u : 0u;
}
""",
        """using RetroPreBrrCallback = u32 (__stdcall *)(void*, u32, u32, u32, s16*);
using RetroSetPreBrrProvider = void (__stdcall *)(RetroPreBrrCallback, void*);

static u32 __stdcall retro_prebrr_callback(
    void* user,
    u32 srcn,
    u32 brr_pointer,
    u32 source_start,
    s16* out16)
{
    return RetroPreBrrRuntime::callback(
        user, srcn, brr_pointer, source_start, out16) ? 1u : 0u;
}
""",
        "calibrated pre-BRR child callback ABI",
    )

    print("spcplayer pre-BRR pointer callback ABI repaired")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
