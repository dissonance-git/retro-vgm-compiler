#!/usr/bin/env python3
"""Apply the project-owned causal-source tap to the pinned SPCPlay SNESAPU tree.

This is consolidated from the historical vgmspc implementation. It deliberately
uses exact guarded replacements instead of fuzzy patching. If the pinned upstream
source changes underneath the integration, the script fails rather than silently
relocating an audio-hot-loop edit.
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
    root = args.root.resolve()
    dll = root / "snesapu.dll"

    replace_once(
        dll / "DSP.inc",
        """PUBLIC SetDSPVol, vol:dword
PUBLIC SetDSPLength, song:dword, fade:dword                     ;see SetAPULength in APU.inc
""",
        """PUBLIC SetDSPVol, vol:dword
PUBLIC SetDSPLength, song:dword, fade:dword                     ;see SetAPULength in APU.inc

;===================================================================================================
;Causal Source Capture
;
;Observational tap for source-aware renderers. The returned buffer contains one
;completed RunDSP block in sample-major float32 order:
;  dry voice 0..7, shared echo left, shared echo right.
;Capture is disabled by default.

PUBLIC SetDSPSourceCapture, enable:dword
PUBLIC GetDSPSourceData, pFrames:ptr
""",
        "DSP public source-capture API",
    )

    replace_once(
        dll / "DSP.h",
        """void SetDSPVol(u32 vol);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        """void SetDSPVol(u32 vol);


//**************************************************************************************************
// Causal Source Capture
//
// The buffer returned by GetDSPSourceData is owned by SNESAPU, contains ten
// sample-major float lanes for the most recently completed RunDSP block, and is
// overwritten by the next block.

void SetDSPSourceCapture(u32 enable);
float* GetDSPSourceData(u32 *pFrames);


//**************************************************************************************************
// Set Voice Stereo Separation
""",
        "DSP C source-capture declarations",
    )

    replace_once(
        dll / "SNESAPU.h",
        """    void        (__stdcall *SetDSPAmp)(u32 level);
    DSPDebug*   (__stdcall *SetDSPDbg)(DSPDebug *pTrace);
    void        (__stdcall *SetDSPEFBCT)(s32 leak);
    void        (__stdcall *SetDSPOpt)(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
""",
        """    void        (__stdcall *SetDSPAmp)(u32 level);
    DSPDebug*   (__stdcall *SetDSPDbg)(DSPDebug *pTrace);
    void        (__stdcall *SetDSPEFBCT)(s32 leak);
    void        (__stdcall *SetDSPSourceCapture)(u32 enable);
    float*      (__stdcall *GetDSPSourceData)(u32 *pFrames);
    void        (__stdcall *SetDSPOpt)(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
""",
        "SNESAPU function-table source-capture declarations",
    )

    replace_once(
        dll / "SNESAPU.h",
        """import  void        __stdcall SetDSPAmp(u32 level);
import  DSPDebug*   __stdcall SetDSPDbg(DSPDebug *pTrace);
import  void        __stdcall SetDSPEFBCT(s32 leak);
import  void        __stdcall SetDSPOpt(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
""",
        """import  void        __stdcall SetDSPAmp(u32 level);
import  DSPDebug*   __stdcall SetDSPDbg(DSPDebug *pTrace);
import  void        __stdcall SetDSPEFBCT(s32 leak);
import  void        __stdcall SetDSPSourceCapture(u32 enable);
import  float*      __stdcall GetDSPSourceData(u32 *pFrames);
import  void        __stdcall SetDSPOpt(u32 mix, u32 chn, u32 bits, u32 rate, u32 inter, u32 opts);
""",
        "SNESAPU imported source-capture declarations",
    )

    replace_once(
        dll / "SNESAPU.def",
        """  SetDSPAmp
  SetDSPDbg
  SetDSPEFBCT
  SetDSPPitch
""",
        """  SetDSPAmp
  SetDSPDbg
  SetDSPEFBCT
  SetDSPSourceCapture
  GetDSPSourceData
  SetDSPPitch
""",
        "SNESAPU source-capture exports",
    )

    asm = dll / "DSP.asm"
    replace_once(
        asm,
        """    MIX_SIZE    EQU 1024                                                        ;Size of mixing buffer in samples
    FIRBUF      EQU 2*2*64                                                      ;Stereo * Ring loop * 256kHz / 32kHz
""",
        """    MIX_SIZE    EQU 1024                                                        ;Size of mixing buffer in samples
    SRC_LANES   EQU 10                                                          ;8 dry voices + stereo shared echo
    SRC_STRIDE  EQU SRC_LANES*4                                                 ;Bytes per captured source frame
    FIRBUF      EQU 2*2*64                                                      ;Stereo * Ring loop * 256kHz / 32kHz
""",
        "DSP source-capture constants",
    )

    replace_once(
        asm,
        """    mixBuf      resd    MIX_SIZE*4                                              ;Temporary mixing buffer (linear buffer)
    echoBuf     resd    ECHOBUF                                                 ;External echo memory, 240ms @ 192kHz (ring buffer)
""",
        """    mixBuf      resd    MIX_SIZE*4                                              ;Temporary mixing buffer (linear buffer)
    srcBuf      resd    MIX_SIZE*SRC_LANES                                      ;Causal source capture, sample-major float32
    srcCapture  resd    1                                                       ;0 = disabled, nonzero = capture
    srcFrames   resd    1                                                       ;Frames valid in srcBuf
    srcDryPtr   resd    1                                                       ;Current dry-source frame
    srcEchoPtr  resd    1                                                       ;Current echo-source frame
    echoBuf     resd    ECHOBUF                                                 ;External echo memory, 240ms @ 192kHz (ring buffer)
""",
        "DSP source-capture storage",
    )

    replace_once(
        asm,
        """    ;Call   ResetVol                                                            ;Don't call ResetVol to smooth fade-out

ENDP


;===================================================================================================
;Set Song Length
""",
        """    ;Call   ResetVol                                                            ;Don't call ResetVol to smooth fade-out

ENDP


;===================================================================================================
;Causal Source Capture

PROC SetDSPSourceCapture, enable

    Mov     EAX,[enable]
    Test    EAX,EAX
    SetNZ   AL
    MovZX   EAX,AL
    Mov     [srcCapture],EAX
    Test    EAX,EAX
    JNZ     short .Done
        Mov     dword [srcFrames],0
    .Done:

ENDP


PROC GetDSPSourceData, pFrames

    Mov     EDX,[pFrames]
    Test    EDX,EDX
    JZ      short .NoFrames
        Mov     EAX,[srcFrames]
        Mov     [EDX],EAX
    .NoFrames:
    Mov     EAX,srcBuf

ENDP


;===================================================================================================
;Set Song Length
""",
        "DSP source-capture procedures",
    )

    replace_once(
        asm,
        """    MovZX   EAX,byte [EBX+mSrc]                                                 ;EAX = Source
    Mov     AH,[scr700dsp+EAX]                                                  ;AH = DSPFlag[EAX]
    Test    AH,S700_MUTE                                                        ;AH and S700_MUTE = S700_MUTE?
    JNZ     .VoiceOff                                                           ;   Yes
%endmacro
""",
        """    MovZX   EAX,byte [EBX+mSrc]                                                 ;EAX = Source
    Mov     AH,[scr700dsp+EAX]                                                  ;AH = DSPFlag[EAX]
    Test    AH,S700_MUTE                                                        ;AH and S700_MUTE = S700_MUTE?
    JNZ     .VoiceOff                                                           ;   Yes

    ;The FPU top is the exact post-interpolation/noise + post-envelope sample
    ;which mOut observes, before VxVOLL/VxVOLR. Store it without popping or
    ;altering the normal mixer state. Muted/inactive lanes remain zero because
    ;the capture block is cleared before mixing starts.
    Test    dword [srcCapture],-1
    JZ      short %%NoSourceCapture
        Push    EAX,EDX
        Mov     EAX,EBX
        Sub     EAX,mix
        ShR     EAX,5                                                           ;(voice*80h)/20h = voice*4 byte offset
        Mov     EDX,[srcDryPtr]
        FSt     dword [EDX+EAX]
        Pop     EDX,EAX
    %%NoSourceCapture:
%endmacro
""",
        "DSP dry-voice tap",
    )

    replace_once(
        asm,
        """    Test    dword [dspOpts],DSP_NOFIR                                           ;Is FIR filter disabled?
    JNZ     %%NoFilter                                                          ;   Yes
        FIRFilter
    %%NoFilter:

    FLd     ST1                                                                 ;                                   |FBR FBL FBR
""",
        """    Test    dword [dspOpts],DSP_NOFIR                                           ;Is FIR filter disabled?
    JNZ     %%NoFilter                                                          ;   Yes
        FIRFilter
    %%NoFilter:

    ;FIRFilter leaves ST0=FBL and ST1=FBR. Preserve the shared return before
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

    FLd     ST1                                                                 ;                                   |FBR FBL FBR
""",
        "DSP shared-echo tap",
    )

    replace_once(
        asm,
        """PROC RunDSP

    Push    EBP,EBX,EAX,EDX                                                     ;Last register must be EAX,EDX
    FInit

    Test    byte [disFlag],80h                                                  ;Is DSP reset or volume safe mode? (disFlag = [7])
""",
        """PROC RunDSP

    Push    EBP,EBX,EAX,EDX                                                     ;Last register must be EAX,EDX
    FInit

    ;One RunDSP invocation is one capture block. Clear the requested range up
    ;front so skipped/muted voices and disabled echo remain aligned zero lanes.
    Mov     dword [srcFrames],0
    Test    dword [srcCapture],-1
    JZ      short .SourceInitDone
        Mov     [srcFrames],EDX
        Mov     dword [srcDryPtr],srcBuf
        Mov     dword [srcEchoPtr],srcBuf
        Push    EAX,ECX,EDI
        Mov     EDI,srcBuf
        Mov     ECX,EDX
        IMul    ECX,SRC_LANES
        XOr     EAX,EAX
        Rep     StoSD
        Pop     EDI,ECX,EAX
    .SourceInitDone:

    Test    byte [disFlag],80h                                                  ;Is DSP reset or volume safe mode? (disFlag = [7])
""",
        "DSP source-block initialization",
    )

    replace_once(
        asm,
        """        Mov     [adsrCnt],CH                                                    ;Clear number of times to update envelope
        Add     EDI,16
""",
        """        Mov     [adsrCnt],CH                                                    ;Clear number of times to update envelope
        Test    dword [srcCapture],-1
        JZ      short .NoDrySourceAdvance
            Add     dword [srcDryPtr],SRC_STRIDE
        .NoDrySourceAdvance:
        Add     EDI,16
""",
        "DSP dry-source frame advance",
    )

    replace_once(
        asm,
        """        ApplyLevel
        Add     ESI,16
""",
        """        ApplyLevel
        Test    dword [srcCapture],-1
        JZ      short .NoEchoSourceAdvance
            Add     dword [srcEchoPtr],SRC_STRIDE
        .NoEchoSourceAdvance:
        Add     ESI,16
""",
        "DSP echo-source frame advance",
    )

    print("SNESAPU causal source capture patch applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
