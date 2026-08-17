import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class StudioSourceProviderPatchTest(unittest.TestCase):
    def test_patches_only_the_pinned_prebrr_shape(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "snesapu" / "apply_studio_source_provider.py"

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dll = root / "snesapu.dll"
            dll.mkdir()

            (dll / "DSP.inc").write_text(
                "PUBLIC SetDSPPreBrrProvider, callback:dword, user:dword\n",
                encoding="utf-8",
            )
            (dll / "DSP.h").write_text(
                "typedef u32 (__stdcall *DSPPreBrrProvider)(void *user, u32 srcn, u32 brrAddr, s16 *out16);\n"
                "void SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);\n\n\n"
                "//**************************************************************************************************\n"
                "// Set Voice Stereo Separation\n",
                encoding="utf-8",
            )
            (dll / "SNESAPU.h").write_text(
                "    void        (__stdcall *SetDSPPreBrrProvider)(DSPPreBrrProvider callback, void *user);\n"
                "import  void        __stdcall SetDSPPreBrrProvider(DSPPreBrrProvider callback, void *user);\n",
                encoding="utf-8",
            )
            (dll / "SNESAPU.def").write_text(
                "  SetDSPPreBrrProvider\n",
                encoding="utf-8",
            )
            (dll / "DSP.asm").write_text(
                "    preBrrProvider resd 1                                                       ;optional stdcall pre-BRR block provider\n"
                "    preBrrUser     resd 1                                                       ;opaque provider context\n"
                "\n"
                "PROC SetDSPPreBrrProvider, callback, user\n\n"
                "    Mov     EAX,[callback]\n"
                "    Mov     [preBrrProvider],EAX\n"
                "    Mov     EAX,[user]\n"
                "    Mov     [preBrrUser],EAX\n\n"
                "ENDP\n\n\n"
                ";===================================================================================================\n"
                ";Set Song Length\n"
                "\n"
                "    Mov     ESI,[pAPURAM]\n"
                "    ShL     EAX,2\n"
                "    Add     AH,[dsp+dir]                                                        ;EAX -> Source directory\n"
                "    Mov     SI,[EAX+ESI]                                                        ;ESI -> First block of waveform\n"
                "    LEA     EDI,[EBX+sBuf]                                                      ;EDI -> Uncompressed sample buffer\n"
                "    Mov     [EBX+bCur],ESI                                                      ;Save physical pointers to wave data\n"
                "    Mov     [EBX+sIdx],EDI\n\n"
                "    ;Fill first block: proven pre-BRR PCM or exact BRR decode ------\n"
                "\n"
                "%macro MixSample 0\n"
                "    ;Get sample ========================\n"
                "    Mov     ESI,[EBX+sIdx]\n"
                "    MovZX   EAX,word [EBX+mDec]\n"
                "    Call    [pInter]                                                            ;                                   |smp\n\n",
                encoding="utf-8",
            )

            first = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)

            asm = (dll / "DSP.asm").read_text(encoding="utf-8")
            self.assertIn("studioSourceBegin", asm)
            self.assertIn("studioSourceSample", asm)
            self.assertIn("studioSourceVoices", asm)
            self.assertIn("MovZX   EBP,word [EAX+ESI+2]", asm)
            self.assertIn("BSF     EDX,EDX", asm)
            self.assertIn("MovZX   EAX,byte [scr700chg+EAX]", asm)
            self.assertIn("MovZX   EAX,byte [dsp+dir]", asm)
            self.assertIn("Push    dword [EBX+mRate]", asm)
            self.assertIn("FLd     dword [ESP]", asm)
            self.assertIn("Call    [pInter]", asm)

            # NON/noise replaces the interpolated waveform in the pinned mixer.
            # It must reach the historical pInter branch before the expensive
            # verified-source callback is considered, rather than spending the
            # 64-tap studio FIR on a value that will immediately be discarded.
            noise_guard = asm.index("Test    [dspNoise],CH")
            studio_guard = asm.index("Test    byte [studioSourceVoices],CH")
            callback_rate = asm.index("Push    dword [EBX+mRate]")
            historical = asm.index("%%HistoricalInterpolation:")
            self.assertLess(noise_guard, studio_guard)
            self.assertLess(studio_guard, callback_rate)
            self.assertLess(callback_rate, historical)
            self.assertIn("JNZ     %%HistoricalInterpolation", asm)

            # The pinned END+LOOP path can refresh live DSP SRCN, reapply
            # Script700 NoteChange, and read a new loop pointer without changing
            # DIR. The studio callback must reconstruct exactly those two live
            # locators before it renders the current verified-source sample.
            mix_start = asm.index("%macro MixSample 0")
            live_src = asm.index("MovZX   EDX,byte [EBX+mSrc]", mix_start)
            live_notechange = asm.index(
                "MovZX   EDX,byte [scr700chg+EDX]", live_src
            )
            live_ram = asm.index("Mov     ESI,[pAPURAM]", live_notechange)
            live_loop = asm.index("MovZX   ESI,word [EAX+ESI+2]", live_ram)
            push_loop = asm.index("Push    ESI", live_loop)
            push_source = asm.index("Push    EDX", push_loop)
            callback_rate = asm.index("Push    dword [EBX+mRate]", push_source)
            callback_call = asm.index("Call    EDX", callback_rate)
            self.assertLess(live_src, live_notechange)
            self.assertLess(live_notechange, live_loop)
            self.assertLess(live_loop, push_loop)
            self.assertLess(push_loop, push_source)
            self.assertLess(push_source, callback_rate)
            self.assertLess(callback_rate, callback_call)
            self.assertIn("stdcall callback cleans eight arguments", asm)

            dsp_h = (dll / "DSP.h").read_text(encoding="utf-8")
            self.assertIn("DSPStudioSourceBeginProvider", dsp_h)
            self.assertIn("u32 loopBrrAddr", dsp_h)
            self.assertIn("u32 directoryPage", dsp_h)
            self.assertIn("DSPStudioSourceSampleProvider", dsp_h)
            self.assertIn("u32 effectiveSrcn", dsp_h)
            self.assertIn("u32 liveLoopBrrAddr", dsp_h)
            self.assertIn("float *outSample", dsp_h)

            # Guarded patching is intentionally non-idempotent. A second run no
            # longer sees the exact pinned predecessor and must fail rather than
            # stacking a second hot-loop hook.
            second = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)


if __name__ == "__main__":
    unittest.main()
