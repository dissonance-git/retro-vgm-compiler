from __future__ import annotations

import ast
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCHES = ROOT / "patches" / "snesapu"


class SpcSurroundBedPatchContractTest(unittest.TestCase):
    def read(self, path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_patch_scripts_remain_valid_python(self) -> None:
        for name in (
            "apply_surround_ui_bridge.py",
            "apply_spatial_omniphony_private_runtime.py",
            "apply_surround_omniphony_private_bridge.py",
            "apply_private_component.py",
        ):
            ast.parse(self.read(PATCHES / name), filename=name)

    def test_historical_surround_bit_is_the_ui_owner(self) -> None:
        ui = self.read(PATCHES / "apply_surround_ui_bridge.py")
        self.assertIn('CONTROL         "Surround",IDC_DSP_SURROUND', ui)
        self.assertIn("DSP_SURND", ui)
        self.assertIn("MAX_DSP_OPT 13", ui)

    def test_old_snesapu_surround_algorithm_is_masked(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_private_bridge.py")
        self.assertIn("m_CnfOptions & ~DSP_SURND", bridge)
        self.assertIn("snesapu_runtime_options", bridge)
        self.assertIn("disable legacy SNESAPU surround processing", bridge)

    def test_bridge_executes_against_expected_generated_shape(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "foobar2000" / "foo_snesapu"
            source_dir.mkdir(parents=True)
            source = source_dir / "input_snesapu.cpp"
            source.write_text(
                "\tm_Apu.SetAPUOpt(m_CnfMixing, m_CnfChannels, m_CnfBPS, m_CnfSampleRate, m_CnfInterpolation, m_CnfOptions);\n"
                "\tm_Sem71Enabled = cfg_sem71_enabled;\n"
                "\tm_Apu.SetSourceEnabled(m_Sem71Enabled);\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    sys.executable,
                    str(PATCHES / "apply_surround_omniphony_private_bridge.py"),
                    str(root),
                ],
                cwd=root,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)

            patched = source.read_text(encoding="utf-8")
            self.assertIn("m_CnfOptions & ~DSP_SURND", patched)
            self.assertIn("(m_CnfOptions & DSP_SURND) != 0", patched)
            self.assertNotIn("cfg_sem71_enabled", patched)

    def test_surround_ui_bridge_owns_enhanced_lockout(self) -> None:
        ui = self.read(PATCHES / "apply_surround_ui_bridge.py")
        self.assertIn('"enhanced (later)"', ui)
        self.assertIn("WS_DISABLED", ui)
        self.assertIn("cfg_enhanced_enabled = 0;", ui)
        self.assertIn("BST_UNCHECKED", ui)
        self.assertNotIn(
            "cfg_enhanced_enabled = SendDlgItemMessage(IDC_ENHANCED_ENABLED",
            ui.split('"remove duplicate SNES spatial apply"')[0].split(
                '"remove SNES duplicate spatial reset"'
            )[-1],
        )

    def test_runtime_is_exact_dry_wet_to_standard_7_1(self) -> None:
        runtime = self.read(PATCHES / "apply_spatial_omniphony_private_runtime.py")
        self.assertIn("surround_bed_7_1.h", runtime)
        self.assertIn("begin_from_interleaved_stereo", runtime)
        self.assertIn("source.echo_left()", runtime)
        self.assertIn("source.echo_right()", runtime)
        self.assertIn("move_stereo_to_surround_field", runtime)
        self.assertIn("audio_chunk::channel_config_7point1", runtime)
        self.assertNotIn("omniphony_source_spatial_full_sphere", runtime)
        self.assertNotIn("realtime_musical_omniphony_pipeline", runtime)
        self.assertNotIn("SemanticStereoEnhancer", runtime.split("helpers =")[1])

    def test_active_stack_has_no_decoder_side_omniphony_runtime(self) -> None:
        master = self.read(PATCHES / "apply_private_component.py")
        self.assertIn('run(here / "apply_spatial_omniphony_private_runtime.py", root)', master)
        self.assertIn('run(here / "apply_surround_omniphony_private_bridge.py", root)', master)
        self.assertNotIn('run(here / "apply_spatial_omniphony_private_rate_lifecycle.py", root)', master)
        self.assertNotIn('run(here / "apply_foobar_source_session.py", root)', master)

    def test_same_saved_surround_bit_gates_source_capture(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_private_bridge.py")
        self.assertIn("(m_CnfOptions & DSP_SURND) != 0", bridge)
        self.assertIn("m_Apu.SetSourceEnabled(m_Sem71Enabled);", bridge)


if __name__ == "__main__":
    unittest.main()
