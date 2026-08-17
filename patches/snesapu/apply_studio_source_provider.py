#!/usr/bin/env python3
"""Add the verified-upstream per-sample source seam to pinned SPCPlay/SNESAPU.

Run after apply_prebrr_provider.py. The earlier provider still owns exact
prepared game-grid / pre-BRR replacement. This later and more selective seam
runs at MixSample where SNESAPU would call pInter. When one concrete SRCN + first
BRR address was admitted at key-on, a child-local callback may provide the
waveform value that pInter would otherwise leave on the x87 stack.

Everything after that point remains SNESAPU truth: NON/noise replacement,
envelope, mOut/PMON feedback, VxVOL, EON, echo/FIR/feedback, MVOL and timing.
The callback never crosses process IPC. A zero callback result falls through to
exact historical pInter for that sample and disables substitution until the next
key-on so a failed provider cannot repeatedly perturb the hot loop.
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
        """PUBLIC SetDSPPreBrrProvider, callback:dword, user:dword
""",
        """PUBLIC SetDSPPreBrrProvider, callback:dword, user:dword
PUBLIC SetDSPStudioSourceProvider, beginCallback:dword, sampleCallback:dword, user:dword
""",
        "DSP studio source public API",
    )

    replace_once(
        dll / "DSP.h",
        """typedef u32 (__stdcall *DSPPreBrrProvider)(void *user, u32 srcn, u32 brrAddr, s16 *out16);
void SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        """typedef u32 (__stdcall *DSPPreBrrProvider)(void *user, u32 srcn, u32 brrAddr, s16 *out16);
void SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);


//**************************************************************************************************
// Verified Upstream / Studio Source Replacement
//
// Begin runs once at key-on. The source number is the effective source used by
// StartSrc after Script700 NoteChange, and firstBrrAddr is the concrete 16-bit
// source-directory address. Return non-zero only when that exact runtime source
// object has an evidence-approved upstream waveform and exact playback map.
//
// Sample runs at MixSample before historical pInter. mRateQ16_16 is the exact
// current rate after PMON. Write one source sample in SNESAPU's decoded int16
// amplitude units to outSample and return non-zero. Returning zero uses pInter
// for the current sample and disables studio substitution until the next key-on.

typedef u32 (__stdcall *DSPStudioSourceBeginProvider)(
    void *user, u32 voice, u32 srcn, u32 firstBrrAddr, u32 interpolation);
typedef u32 (__stdcall *DSPStudioSourceSampleProvider)(
    void *user, u32 voice, u32 mRateQ16_16, u32 interpolation, float *outSample);
void SetDSPStudioSourceProvider(
    DSPStudioSourceBeginProvider beginCallback,
    DSPStudioSourceSampleProvider sampleCallback,
    void *user);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        "DSP studio source C declarations",
    )

    replace_once(
        dll / "SNESAPU.h",
        """    void        (__stdcall *SetDSPPreBrrProvider)(DSPPreBrrProvider callback, void *user);
""",
        """    void        (__stdcall *SetDSPPreBrrProvider)(DSPPreBrrProvider callback, void *user);
    void        (__stdcall *SetDSPStudioSourceProvider)(DSPStudioSourceBeginProvider beginCallback, DSPStudioSourceSampleProvider sampleCallback, void *user);
""",
        "SNESAPU function-table studio source provider",
    )

    replace_once(
        dll / "SNESAPU.h",
        """import  void        __stdcall SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);
""",
        """import  void        __stdcall SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);
import  void        __stdcall SetDSPStudioSourceProvider(DSPStudioSourceBeginProvider beginCallback, DSPStudioSourceSampleProvider sampleCallback, void *user);
""",
        "SNESAPU imported studio source provider",
    )

    replace_once(
        dll / "SNESAPU.def",
        """  SetDSPPreBrrProvider
""",
        """  SetDSPPreBrrProvider
  SetDSPStudioSourceProvider
""",
        "SNESAPU studio source provider export",
    )

    asm = dll / "DSP.asm"
    replace_once(
        asm,
        """    preBrrProvider resd 1                                                       ;optional stdcall pre-BRR block provider
    preBrrUser     resd 1                                                       ;opaque provider context
""",
        """    preBrrProvider resd 1                                                       ;optional stdcall pre-BRR block provider
    preBrrUser     resd 1                                                       ;opaque provider context
    studioSourceBegin  resd 1                                                   ;optional stdcall key-on binder
    studioSourceSample resd 1                                                   ;optional stdcall MixSample replacement
    studioSourceUser   resd 1                                                   ;opaque child-local provider context
    studioSourceVoices resd 1                                                   ;low eight bits: admitted voice mask
""",
        "DSP studio source provider state",
    )

    replace_once(
        asm,
        """PROC SetDSPPreBrrProvider, callback, user

    Mov     EAX,[callback]
    Mov     [preBrrProvider],EAX
    Mov     EAX,[user]
    Mov     [preBrrUser],EAX

ENDP


;===================================================================================================
;Set Song Length
""",
        """PROC SetDSPPreBrrProvider, callback, user

    Mov     EAX,[callback]
    Mov     [preBrrProvider],EAX
    Mov     EAX,[user]
    Mov     [preBrrUser],EAX

ENDP


;===================================================================================================
;Set Verified Upstream / Studio Source Provider
;
;Both callbacks live inside spcplayer. Set/clear only at setup boundaries, never
;from another process while the DSP hot loop is running.

PROC SetDSPStudioSourceProvider, beginCallback, sampleCallback, user

    Mov     EAX,[beginCallback]
    Mov     [studioSourceBegin],EAX
    Mov     EAX,[sampleCallback]
    Mov     [studioSourceSample],EAX
    Mov     EAX,[user]
    Mov     [studioSourceUser],EAX
    XOr     EAX,EAX
    Mov     [studioSourceVoices],EAX

ENDP


;===================================================================================================
;Set Song Length
""",
        "DSP studio source provider setter",
    )

    replace_once(
        asm,
        """    Mov     [EBX+bCur],ESI                                                      ;Save physical pointers to wave data
    Mov     [EBX+sIdx],EDI

    ;Fill first block: proven pre-BRR PCM or exact BRR decode ------
""",
        """    Mov     [EBX+bCur],ESI                                                      ;Save physical pointers to wave data
    Mov     [EBX+sIdx],EDI

    ;Bind one exact runtime source object to the optional studio path.
    ;CH is the caller's one-hot voice mask; StartSrc's only caller preserves it.
    MovZX   EDX,CH
    Not     EDX
    And     [studioSourceVoices],EDX                                           ;new key-on always invalidates stale binding first
    Mov     EBP,[studioSourceBegin]
    Test    EBP,EBP
    JZ      short .NoStudioSource
    Mov     EAX,[studioSourceSample]
    Test    EAX,EAX
    JZ      short .NoStudioSource
        Push    ECX                                                             ;stdcall may destroy EAX/ECX/EDX; preserve CH
        MovZX   EDX,CH
        BSF     EDX,EDX                                                         ;one-hot voice mask -> 0..7
        MovZX   EAX,byte [dspInter]
        Push    EAX                                                             ;interpolation
        MovZX   EAX,SI
        Push    EAX                                                             ;concrete first BRR address
        MovZX   EAX,byte [EBX+mSrc]
        MovZX   EAX,byte [scr700chg+EAX]                                        ;effective StartSrc source identity
        Push    EAX                                                             ;SRCN after Script700 NoteChange
        Push    EDX                                                             ;voice index
        Push    dword [studioSourceUser]
        Call    EBP                                                             ;stdcall callback cleans five arguments
        Pop     ECX
        Test    EAX,EAX
        JZ      short .NoStudioSource
            MovZX   EDX,CH
            Or      [studioSourceVoices],EDX

    .NoStudioSource:
    ;Fill first block: proven pre-BRR PCM or exact BRR decode ------
""",
        "StartSrc studio source binding seam",
    )

    replace_once(
        asm,
        """%macro MixSample 0
    ;Get sample ========================
    Mov     ESI,[EBX+sIdx]
    MovZX   EAX,word [EBX+mDec]
    Call    [pInter]                                                            ;                                   |smp

""",
        """%macro MixSample 0
    ;Get sample ========================
    ;A verified studio source replaces only pInter's waveform value. The exact
    ;current PMON-adjusted mRate is consumed by the child-local provider after
    ;rendering this phase, mirroring the UpdateSrc call below MixSample.
    Test    byte [studioSourceVoices],CH
    JZ      %%HistoricalInterpolation
    Mov     EDX,[studioSourceSample]
    Test    EDX,EDX
    JZ      %%DisableStudioSource
        Sub     ESP,4                                                          ;float callback output slot
        Push    ECX                                                             ;preserve one-hot voice mask in CH
        LEA     EAX,[ESP+4]
        Push    EAX                                                             ;outSample
        MovZX   EAX,byte [dspInter]
        Push    EAX                                                             ;interpolation timing contract
        Push    dword [EBX+mRate]                                               ;exact current rate after PMON
        MovZX   EAX,CH
        BSF     EAX,EAX
        Push    EAX                                                             ;voice index 0..7
        Push    dword [studioSourceUser]
        Call    EDX                                                             ;stdcall callback cleans five arguments
        Pop     ECX
        Test    EAX,EAX
        JZ      %%StudioSourceFailed
            FLd     dword [ESP]                                                 ;same x87 shape pInter would return
            Add     ESP,4
            Jmp     %%SampleReady

    %%StudioSourceFailed:
        Add     ESP,4
    %%DisableStudioSource:
        MovZX   EDX,CH
        Not     EDX
        And     [studioSourceVoices],EDX                                        ;fail closed for remainder of this key-on

    %%HistoricalInterpolation:
        Mov     ESI,[EBX+sIdx]
        MovZX   EAX,word [EBX+mDec]
        Call    [pInter]                                                        ;                                   |smp

    %%SampleReady:

""",
        "MixSample studio waveform replacement seam",
    )

    print("patched SNESAPU verified-upstream studio source provider")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
