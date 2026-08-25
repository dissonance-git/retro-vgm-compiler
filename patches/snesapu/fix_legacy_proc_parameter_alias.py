#!/usr/bin/env python3
"""Repair the generated studio-provider setter for SNESAPU's legacy PROC macros.

The pinned macro.inc implements procedure arguments with ordinary `%define`
aliases rather than context-local aliases. Those aliases survive ENDP. The
pre-BRR setter therefore leaves `user` defined as a stack expression; reusing
`user` as the fourth argument of the immediately following studio setter can
cause PROC to expand that argument before defining it and poison the EBP token.

Keep the generated assembly faithful to the historical macro system by giving
the studio user argument a unique identifier. This is an exact, singular,
fail-closed post-patch delta and runs immediately after the studio provider is
materialized.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text = raw.decode("utf-8")
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_bytes(text.replace(old_file, new_file, 1).encode("utf-8"))
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path, help="path to pinned spcplay checkout")
    args = parser.parse_args()
    asm = args.root.resolve() / "snesapu.dll" / "DSP.asm"

    replace_once(
        asm,
        """PROC SetDSPStudioSourceProvider, beginCallback, sampleCallback, user

    Mov     EAX,[beginCallback]
    Mov     [studioSourceBegin],EAX
    Mov     EAX,[sampleCallback]
    Mov     [studioSourceSample],EAX
    Mov     EAX,[user]
    Mov     [studioSourceUser],EAX
""",
        """PROC SetDSPStudioSourceProvider, beginCallback, sampleCallback, studioUserArg

    Mov     EAX,[beginCallback]
    Mov     [studioSourceBegin],EAX
    Mov     EAX,[sampleCallback]
    Mov     [studioSourceSample],EAX
    Mov     EAX,[studioUserArg]
    Mov     [studioSourceUser],EAX
""",
        "legacy PROC studio user parameter alias",
    )

    print("SNESAPU legacy PROC parameter alias repaired")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
