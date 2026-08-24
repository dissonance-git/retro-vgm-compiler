#!/usr/bin/env python3
"""Add a low-frequency pre-BRR restoration provider to pinned SPCPlay/SNESAPU.

Run after apply_source_capture.py / upgrade_source_capture_v2.py. The provider is
called once per BRR block, not once per output sample. When it returns non-zero,
its sixteen int16 samples replace BRR decompression output on the exact game
sample grid. All later SNESAPU machinery remains authoritative: interpolation,
pitch/PMON, envelope, VxVOL, echo send/FIR/feedback, MVOL, and playback timing.

This is therefore the ideal seam for a *proven* original pre-BRR sample. It
removes BRR quantization without rebuilding the rest of the S-DSP outside the
reference renderer. A zero return value falls through to the exact historical
BRR decoder.
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
    dll = args.root.resolve() / "snesapu.dll"

    replace_once(
        dll / "DSP.inc",
        """PUBLIC SetDSPVol, vol:dword
""",
        """PUBLIC SetDSPVol, vol:dword
PUBLIC SetDSPPreBrrProvider, callback:dword, user:dword
""",
        "DSP pre-BRR provider public API",
    )

    replace_once(
        dll / "DSP.h",
        """void SetDSPSourceCapture(u32 enable);
float* GetDSPSourceData(u32 *pFrames);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        """void SetDSPSourceCapture(u32 enable);
float* GetDSPSourceData(u32 *pFrames);


//**************************************************************************************************
// Pre-BRR Source Restoration
//
// The callback runs only when a BRR block would otherwise be decompressed. It
// receives SRCN, the 16-bit BRR block address, and a writable sixteen-sample
// int16 buffer. Return non-zero only when the buffer contains the complete,
// evidence-approved pre-BRR game-grid block. Return zero for exact BRR fallback.
// u32 is deliberate: the assembly caller tests the full EAX return register.

typedef u32 (__stdcall *DSPPreBrrProvider)(void *user, u32 srcn, u32 brrAddr, s16 *out16);
void SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        "DSP pre-BRR C declarations",
    )

    replace_once(
        dll / "SNESAPU.h",
        """    void        (__stdcall *SetDSPEFBCT)(s32 leak);
""",
        """    void        (__stdcall *SetDSPEFBCT)(s32 leak);
    void        (__stdcall *SetDSPPreBrrProvider)(DSPPreBrrProvider callback, void *user);
""",
        "SNESAPU function-table pre-BRR provider",
    )

    replace_once(
        dll / "SNESAPU.h",
        """import  void        __stdcall SetDSPEFBCT(s32 leak);
""",
        """import  void        __stdcall SetDSPEFBCT(s32 leak);
import  void        __stdcall SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);
""",
        "SNESAPU imported pre-BRR provider",
    )

    replace_once(
        dll / "SNESAPU.def",
        """  SetDSPEFBCT
""",
        """  SetDSPEFBCT
  SetDSPPreBrrProvider
""",
        "SNESAPU pre-BRR provider export",
    )

    asm = dll / "DSP.asm"
    replace_once(
        asm,
        """    pTrace      resd    1                                                       ;-> Debugging vector
""",
        """    pTrace      resd    1                                                       ;-> Debugging vector
    preBrrProvider resd 1                                                       ;optional stdcall pre-BRR block provider
    preBrrUser     resd 1                                                       ;opaque provider context
""",
        "DSP pre-BRR provider state",
    )

    replace_once(
        asm,
        """;===================================================================================================
;Set Song Length
""",
        """;===================================================================================================
;Set Pre-BRR Source Provider
;
;callback(user, SRCN, 16-bit BRR block address, int16 output[16]) -> nonzero
;The callback runs in the spcplayer child process, never across process IPC.

PROC SetDSPPreBrrProvider, callback, user

    Mov     EAX,[callback]
    Mov     [preBrrProvider],EAX
    Mov     EAX,[user]
    Mov     [preBrrUser],EAX

ENDP


;===================================================================================================
;Set Song Length
""",
        "DSP pre-BRR provider setter",
    )

    replace_once(
        asm,
        """    ;Decompress first block ------------------
    Mov     AL,[ESI]
    Push    EBX
    Mov     [EBX+bHdr],AL                                                       ;Save block header
    MovSX   EDX,word [EBX+sP1]
    MovSX   EBX,word [EBX+sP2]
    Call    [pDecomp]
    Mov     EAX,EBX
    Pop     EBX
    Mov     [EBX+sP1],DX
    Mov     [EBX+sP2],AX

    ;Initialize interpolation ----------------
""",
        """    ;Fill first block: proven pre-BRR PCM or exact BRR decode ------
    Mov     AL,[ESI]
    Mov     [EBX+bHdr],AL                                                       ;Header/END/LOOP semantics always come from SPC RAM

    Mov     EAX,[preBrrProvider]
    Test    EAX,EAX
    JZ      short .DecodeBRR
        Push    ECX                                                             ;StartSrc promises ECX is preserved
        MovZX   EDX,SI                                                          ;16-bit BRR block address
        MovZX   ECX,byte [EBX+mSrc]                                             ;SRCN
        Push    EDI                                                             ;out16
        Push    EDX                                                             ;block address
        Push    ECX                                                             ;source number
        Push    dword [preBrrUser]
        Call    EAX                                                             ;stdcall callback cleans four arguments
        Pop     ECX
        Test    EAX,EAX
        JZ      short .DecodeBRR
            Mov     AX,[EDI+30]                                                 ;sample 15 = decoder prev1
            Mov     [EBX+sP1],AX
            Mov     AX,[EDI+28]                                                 ;sample 14 = decoder prev2
            Mov     [EBX+sP2],AX
            Jmp     short .BlockReady

    .DecodeBRR:
        Push    EBX
        MovSX   EDX,word [EBX+sP1]
        MovSX   EBX,word [EBX+sP2]
        Call    [pDecomp]
        Mov     EAX,EBX
        Pop     EBX
        Mov     [EBX+sP1],DX
        Mov     [EBX+sP2],AX

    .BlockReady:
    ;Initialize interpolation ----------------
""",
        "StartSrc pre-BRR replacement seam",
    )

    replace_once(
        asm,
        """        ;Decompress next block ----------------
        %%NotEndB:
            Mov     ESI,[EBX+bCur]                                              ;ESI -> Current sample block
            Push    EDI,EBX
            Mov     AL,[ESI]                                                    ;Get block header
            LEA     EDI,[EBX+sBuf]                                              ;EDI -> location to store samples
            Mov     [EBX+bHdr],AL                                               ;Save header byte
            MovSX   EDX,word [EBX+sP1]                                          ;Load previous two samples
            MovSX   EBX,word [EBX+sP2]
            Call    [pDecomp]                                                   ;Call user selected decompression routine

            Mov     EAX,EBX
            Pop     EBX,EDI
            Mov     [EBX+sP1],DX                                                ;Save last two samples in 16-bit form
            Mov     [EBX+sP2],AX

            Mov     AL,[EBX+bHdr]
""",
        """        ;Fill next block: proven pre-BRR PCM or exact BRR decode ----
        %%NotEndB:
            Mov     ESI,[EBX+bCur]                                              ;ESI -> Current sample block
            Push    EDI
            Mov     AL,[ESI]                                                    ;Get block header
            LEA     EDI,[EBX+sBuf]                                              ;EDI -> location to store samples
            Mov     [EBX+bHdr],AL                                               ;Header remains historical truth

            Mov     EAX,[preBrrProvider]
            Test    EAX,EAX
            JZ      short %%DecodeBRR
                Push    ECX                                                     ;UpdateSrc must preserve CH voice mask
                MovZX   EDX,SI
                MovZX   ECX,byte [EBX+mSrc]
                Push    EDI
                Push    EDX
                Push    ECX
                Push    dword [preBrrUser]
                Call    EAX
                Pop     ECX
                Test    EAX,EAX
                JZ      short %%DecodeBRR
                    Mov     AX,[EDI+30]
                    Mov     [EBX+sP1],AX
                    Mov     AX,[EDI+28]
                    Mov     [EBX+sP2],AX
                    Pop     EDI
                    Jmp     short %%BlockReady

            %%DecodeBRR:
                MovSX   EDX,word [EBX+sP1]
                Push    EBX
                MovSX   EBX,word [EBX+sP2]
                Call    [pDecomp]
                Mov     EAX,EBX
                Pop     EBX
                Mov     [EBX+sP1],DX
                Mov     [EBX+sP2],AX
                Pop     EDI

            %%BlockReady:
            Mov     AL,[EBX+bHdr]
""",
        "UpdateSrc pre-BRR replacement seam",
    )

    print("patched SNESAPU pre-BRR restoration provider")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
