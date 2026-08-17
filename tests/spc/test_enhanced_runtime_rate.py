import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class EnhancedRuntimeRateTest(unittest.TestCase):
    def test_private_x64_playback_is_always_48khz_but_sinc_is_enhanced_only(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "snesapu" / "apply_enhanced_runtime.py"

        predecessor = (
            "\tm_CnfSampleRate\t\t         = cfg_samplerate;\n"
            "\tm_CnfBPS\t\t\t         = cfg_bitspersample;\n"
            "\tm_CnfChannels\t\t         = cfg_channels;\n"
            "\tm_CnfMixing\t\t\t         = 3;\n"
            "\tm_CnfInterpolation\t         = cfg_interpolation;\n"
            "\tm_CnfOptions\t\t         = cfg_dsp_option;\n"
        )

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            input_cpp = root / "input_snesapu.cpp"
            input_cpp.write_text(predecessor, encoding="utf-8")

            completed = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)

            patched = input_cpp.read_text(encoding="utf-8")
            # The stored preference remains visible in the audited predecessor,
            # but the x64 private runtime overrides its host clock unconditionally.
            self.assertIn("m_CnfSampleRate", patched)
            self.assertIn("m_CnfSampleRate = 48000;", patched)
            rate = patched.index("m_CnfSampleRate = 48000;")
            enhanced = patched.index("if (cfg_enhanced_enabled)")
            sinc = patched.index("m_CnfInterpolation = INT_SINC;")
            self.assertLess(rate, enhanced)
            self.assertLess(enhanced, sinc)
            self.assertNotIn("m_CnfSampleRate = 96000;", patched)

            # Guarded patching remains non-idempotent: applying it twice must
            # fail instead of stacking another private rate/source policy block.
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
