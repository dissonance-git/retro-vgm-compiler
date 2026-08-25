#!/usr/bin/env python3
"""Give injected SNESAPU PROC arguments collision-proof names.

The pinned macro.inc implements procedure arguments with ordinary `%define`
aliases, and ENDP does not remove them. Those aliases therefore remain active
for later source text. Only the project-injected procedures are changed here:
each formal receives a globally unique Omniphony-prefixed identifier while the
historical SNESAPU procedures remain untouched. Every replacement is exact,
singular, and fail-closed.
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
        "PROC SetDSPSourceCapture, enable\n\n    Mov     EAX,[enable]\n",
        "PROC SetDSPSourceCapture, omnSourceCaptureEnableArg\n\n    Mov     EAX,[omnSourceCaptureEnableArg]\n",
        "legacy PROC source-capture argument",
    )
    replace_once(
        asm,
        "PROC GetDSPSourceData, pFrames\n\n    Mov     EDX,[pFrames]\n",
        "PROC GetDSPSourceData, omnSourceDataFramesArg\n\n    Mov     EDX,[omnSourceDataFramesArg]\n",
        "legacy PROC source-data argument",
    )
    replace_once(
        asm,
        "PROC SetDSPPreBrrProvider, callback, user\n\n    Mov     EAX,[callback]\n    Mov     [preBrrProvider],EAX\n    Mov     EAX,[user]\n",
        "PROC SetDSPPreBrrProvider, omnPreBrrCallbackArg, omnPreBrrUserArg\n\n    Mov     EAX,[omnPreBrrCallbackArg]\n    Mov     [preBrrProvider],EAX\n    Mov     EAX,[omnPreBrrUserArg]\n",
        "legacy PROC pre-BRR arguments",
    )
    replace_once(
        asm,
        "PROC SetDSPStudioSourceProvider, beginCallback, sampleCallback, user\n\n    Mov     EAX,[beginCallback]\n    Mov     [studioSourceBegin],EAX\n    Mov     EAX,[sampleCallback]\n    Mov     [studioSourceSample],EAX\n    Mov     EAX,[user]\n",
        "PROC SetDSPStudioSourceProvider, omnStudioBeginCallbackArg, omnStudioSampleCallbackArg, omnStudioUserArg\n\n    Mov     EAX,[omnStudioBeginCallbackArg]\n    Mov     [studioSourceBegin],EAX\n    Mov     EAX,[omnStudioSampleCallbackArg]\n    Mov     [studioSourceSample],EAX\n    Mov     EAX,[omnStudioUserArg]\n",
        "legacy PROC studio arguments",
    )

    print("SNESAPU injected legacy PROC parameter names isolated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
