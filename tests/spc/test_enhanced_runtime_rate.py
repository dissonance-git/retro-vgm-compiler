import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class EnhancedRuntimeRateTest(unittest.TestCase):
    def test_private_x64_playback_is_48khz_and_enhanced_is_hard_disabled(self):
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
            rate = patched.index("m_CnfSampleRate = 48000;")
            disable = patched.index("cfg_enhanced_enabled = 0;")
            sinc_gate = patched.index("if (cfg_enhanced_enabled)")
            self.assertLess(rate, disable)
            self.assertLess(disable, sinc_gate)
            self.assertIn("m_CnfInterpolation = INT_SINC;", patched)
            self.assertNotIn("m_CnfSampleRate = 96000;", patched)

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
