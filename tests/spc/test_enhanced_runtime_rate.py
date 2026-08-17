import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class EnhancedRuntimeRateTest(unittest.TestCase):
    def test_enhanced_forces_48khz_without_rewriting_reference_setting(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "snesapu" / "apply_enhanced_runtime.py"

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            input_cpp = root / "input_snesapu.cpp"
            input_cpp.write_text(
                "\tm_CnfSampleRate = cfg_samplerate;\n"
                "\tm_CnfInterpolation = cfg_interpolation;\n"
                "\tm_CnfStereo = cfg_stereo;\n"
                "\tm_CnfOptions = cfg_dsp_option;\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stderr)

            patched = input_cpp.read_text(encoding="utf-8")
            self.assertIn("m_CnfSampleRate = cfg_samplerate;", patched)
            self.assertIn("if (cfg_enhanced_enabled)", patched)
            self.assertIn("m_CnfSampleRate = 48000;", patched)
            self.assertIn("m_CnfInterpolation = INT_SINC;", patched)
            self.assertIn("m_CnfOptions &= ~DSP_ECHOFIR;", patched)
            self.assertNotIn("m_CnfSampleRate = 96000;", patched)

            # Guarded patching remains non-idempotent: applying it twice must
            # fail instead of stacking another Enhanced policy block.
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
