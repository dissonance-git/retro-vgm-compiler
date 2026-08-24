#!/usr/bin/env python3
"""Upgrade the guarded SNESAPU source tap to SRCE v2.

Run after apply_source_capture.py. The v2 delta preserves the raw eight voice
signals, sample-exact voice-local L/R mixer coefficients for those voices, and
the post-EVOL shared wet L/R contribution. The L/R control planes include the
same Script700 per-source volume factor used by MixVoice, but intentionally stop
before the later global MVOL/fade stage. Every replacement is exact and singular.
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
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    asm = args.root.resolve() / "snesapu.dll" / "DSP.asm"

    replace_once(
        asm,
        """    SRC_LANES   EQU 10                                                          ;8 dry voices + stereo shared echo
    SRC_STRIDE  EQU SRC_LANES*4                                                 ;Bytes per captured source frame
""",
        """    SRC_PLANES  EQU 26                                                          ;8 dry + 16 effective gains + stereo wet
    SRC_STRIDE  EQU SRC_PLANES*4                                                ;Bytes per captured source frame
""",
        "source plane constants",
    )

    replace_once(
        asm,
        """    srcBuf      resd    MIX_SIZE*SRC_LANES                                      ;Causal source capture, sample-major float32
""",
        """    srcBuf      resd    MIX_SIZE*SRC_PLANES                                     ;Causal source/control capture, sample-major float32
""",
        "source scratch plane count",
    )

    replace_once(
        asm,
        """        IMul    ECX,SRC_LANES
""",
        """        IMul    ECX,SRC_PLANES
""",
        "source scratch clear plane count",
    )

    replace_once(
        asm,
        """    %%NoChVol:
%endif

%if VMETERV
""",
        """    %%NoChVol:
%endif

    ;Capture the exact voice-local per-sample channel coefficients used below.
    ;mChnL/R already contain the current smooth channel-volume trajectory. When
    ;Script700 source volume is active, apply the same signed 16.16 multiplier
    ;that MixVoice applies after mChn. Global MVOL/fade remains a later stage.
    Test    dword [srcCapture],-1
    JZ      short %%NoRouteCapture
        Push    EAX,ECX,EDX
        Mov     EDX,EBX
        Sub     EDX,mix
        ShR     EDX,5                                                           ;voice index * sizeof(float)
        Test    AH,S700_VOLUME
        JNZ     %%ScriptRouteCapture
            Mov     EAX,[srcDryPtr]
            Mov     ECX,[EBX+mChnL]
            Mov     [EAX+32+EDX],ECX                                            ;planes 8..15 = effective L
            Mov     ECX,[EBX+mChnR]
            Mov     [EAX+64+EDX],ECX                                            ;planes 16..23 = effective R
            Jmp     short %%RouteCaptureDone
        %%ScriptRouteCapture:
            MovZX   ECX,AL                                                      ;Script700 source index
            Mov     EAX,[srcDryPtr]
            FLd     dword [EBX+mChnL]
            FIMul   dword [scr700vol+ECX*4]
            FMul    dword [fpShR16]
            FStP    dword [EAX+32+EDX]
            FLd     dword [EBX+mChnR]
            FIMul   dword [scr700vol+ECX*4]
            FMul    dword [fpShR16]
            FStP    dword [EAX+64+EDX]
        %%RouteCaptureDone:
        Pop     EDX,ECX,EAX
    %%NoRouteCapture:

%if VMETERV
""",
        "sample-exact effective route capture",
    )

    replace_once(
        asm,
        """    ;FIRFilter leaves ST0=FBL and ST1=FBR. Preserve the shared return before
    ;EVOLL/EVOLR and feedback. Restore the stack ordering before continuing.
    Test    dword [srcCapture],-1
    JZ      short %%NoSourceCapture
        Push    EAX
        Mov     EAX,[srcEchoPtr]
        FSt     dword [EAX+32]                                                  ;lane 8 = filtered echo left
        FXCh    ST1
        FSt     dword [EAX+36]                                                  ;lane 9 = filtered echo right
        FXCh    ST1
        Pop     EAX
    %%NoSourceCapture:

""",
        "",
        "remove v1 pre-EVOL wet tap",
    )

    replace_once(
        asm,
        """    %%NoEchoL:
    FAdd    dword [ESI]                                                         ;                                   |FBR FBL FBR EchoL+ML
""",
        """    %%NoEchoL:
    Test    dword [srcCapture],-1
    JZ      short %%NoWetLeftCapture
        Push    EAX
        Mov     EAX,[srcEchoPtr]
        FSt     dword [EAX+96]                                                  ;plane 24 = final shared wet L contribution
        Pop     EAX
    %%NoWetLeftCapture:
    FAdd    dword [ESI]                                                         ;                                   |FBR FBL FBR EchoL+ML
""",
        "post-EVOL wet-left capture",
    )

    replace_once(
        asm,
        """    %%NoEchoR:
    FAdd    dword [4+ESI]                                                       ;                                   |FBR FBL FBR+MR
""",
        """    %%NoEchoR:
    Test    dword [srcCapture],-1
    JZ      short %%NoWetRightCapture
        Push    EAX
        Mov     EAX,[srcEchoPtr]
        FSt     dword [EAX+100]                                                 ;plane 25 = final shared wet R contribution
        Pop     EAX
    %%NoWetRightCapture:
    FAdd    dword [4+ESI]                                                       ;                                   |FBR FBL FBR+MR
""",
        "post-EVOL wet-right capture",
    )

    print("SNESAPU SRCE v2 trajectory capture applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
