import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class PrivateVgm48KhzOutputPatchTest(unittest.TestCase):
    def test_private_vgm_host_rate_is_48khz_without_source_or_spatial_policy(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "foo_input_vgm" / "apply_private_48khz_output.py"

        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            base = source / "input_base.cpp"
            base.write_text(
                "\tpa_cfg.masterVol /= 100;\n"
                "\tm_main_player.SetConfiguration(pa_cfg);\n"
                "\tm_sample_rate = cfg_sample_rate;\n"
                "\tm_bps = cfg_bps;\n",
                encoding="utf-8",
            )

            first = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)

            patched = base.read_text(encoding="utf-8")
            self.assertIn("m_sample_rate = 48000;", patched)
            self.assertNotIn("m_sample_rate = cfg_sample_rate;", patched)
            self.assertNotIn("cfg_vgm_enhanced_enabled", patched)
            self.assertNotIn("cfg_vgm_sem71_enabled", patched)
            self.assertNotIn("96000", patched)

            second = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)

    def test_private_rate_patch_precedes_source_and_spatial_layers(self):
        repo = Path(__file__).resolve().parents[2]
        chain = (
            repo / "patches" / "foo_input_vgm" / "apply_enhanced_component.py"
        ).read_text(encoding="utf-8")
        rate = chain.index('run(here / "apply_private_48khz_output.py", source)')
        source = chain.index('run(here / "apply_source_aware_player.py", source)')
        spatial = chain.index(
            'run(here / "apply_spatial_selected_source_transport.py", source)'
        )
        self.assertLess(rate, source)
        self.assertLess(source, spatial)


if __name__ == "__main__":
    unittest.main()
