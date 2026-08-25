#!/usr/bin/env python3
"""Expose source-provider callback types through the public SNESAPU header.

SNESAPU.h defines SNESAPU_DLL before including DSP.h, which intentionally hides
DSP.h's private declaration block. The private source patches originally placed
provider callback typedefs inside that hidden block, while also adding provider
members/imports to SNESAPU.h. Client builds therefore saw the members without the
types. Mirror only the callback type declarations into the public umbrella
header; the function declarations remain where the existing patches put them.
"""

from __future__ import annotations

import argparse
from pathlib import Path


ANCHOR = """//**************************************************************************************************
// Function pointers to SNESAPU

typedef struct {
"""

REPLACEMENT = """//**************************************************************************************************
// Private source-provider callback ABI exposed to SNESAPU clients

typedef u32 (__stdcall *DSPPreBrrProvider)(void *user, u32 srcn, u32 brrAddr, s16 *out16);
typedef u32 (__stdcall *DSPStudioSourceBeginProvider)(
    void *user, u32 voice, u32 srcn, u32 firstBrrAddr, u32 loopBrrAddr,
    u32 directoryPage, u32 interpolation);
typedef u32 (__stdcall *DSPStudioSourceSampleProvider)(
    void *user, u32 voice, u32 mRateQ16_16, u32 effectiveSrcn,
    u32 liveLoopBrrAddr, u32 directoryPage, u32 interpolation, float *outSample);


//**************************************************************************************************
// Function pointers to SNESAPU

typedef struct {
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("spcplay_root", type=Path)
    root = parser.parse_args().spcplay_root.resolve()
    path = root / "snesapu.dll" / "SNESAPU.h"

    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text = raw.decode("utf-8")
    old = ANCHOR.replace("\n", newline)
    new = REPLACEMENT.replace("\n", newline)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"public provider typedef anchor: expected exactly one match in {path}, found {count}"
        )
    path.write_bytes(text.replace(old, new, 1).encode("utf-8"))
    print("SNESAPU public source-provider callback types exposed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
