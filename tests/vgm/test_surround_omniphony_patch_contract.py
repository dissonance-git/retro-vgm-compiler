from __future__ import annotations

import ast
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCHES = ROOT / "patches" / "foo_input_vgm"


class VgmSurroundBedPatchContractTest(unittest.TestCase):
    def read(self, path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_patch_scripts_remain_valid_python(self) -> None:
        for name in (
            "apply_enhanced_ui.py",
            "apply_spatial_omniphony_runtime.py",
            "apply_surround_omniphony_bridge.py",
            "apply_enhanced_component.py",
        ):
            ast.parse(self.read(PATCHES / name), filename=name)

    def test_existing_surround_preference_owns_7_1_presentation(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_bridge.py")
        self.assertIn("cfg_surround_sound", bridge)
        self.assertIn("pa_cfg.chnInvert = 0x00;", bridge)
        self.assertIn("!cfg_surround_sound || !sources_ready", bridge)

    def test_bridge_executes_against_expected_generated_shape(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir)
            input_base = source / "input_base.cpp"
            shadow = source / "input_vgm_shadow.cpp"
            input_base.write_text(
                "\tpa_cfg.chnInvert = cfg_surround_sound ? 0x02 : 0x00;\n",
                encoding="utf-8",
            )
            shadow.write_text(
                "\tif (!cfg_vgm_sem71_enabled || !sources_ready || frame_count == 0\n"
                "\t\t|| frame_count > 8192u || chunk.get_channels() != 2\n"
                "\t\t|| chunk.get_srate() != m_sample_rate)\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [sys.executable, str(PATCHES / "apply_surround_omniphony_bridge.py"), str(source)],
                cwd=source,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)

            patched_base = input_base.read_text(encoding="utf-8")
            patched_shadow = shadow.read_text(encoding="utf-8")
            self.assertIn("pa_cfg.chnInvert = 0x00;", patched_base)
            self.assertNotIn("cfg_surround_sound ? 0x02 : 0x00", patched_base)
            self.assertIn("!cfg_surround_sound || !sources_ready", patched_shadow)
            self.assertNotIn("cfg_vgm_sem71_enabled", patched_shadow)

    def test_runtime_redistributes_only_exact_delivered_families(self) -> None:
        runtime = self.read(PATCHES / "apply_spatial_omniphony_runtime.py")
        self.assertIn("surround_bed_7_1.h", runtime)
        self.assertIn("move_stereo_to_sides", runtime)
        self.assertIn("move_stereo_to_backs", runtime)
        self.assertIn("sn76489_tone0", runtime)
        self.assertIn("sn76489_noise", runtime)
        self.assertIn("ym2612_dac", runtime)
        self.assertIn("audio_chunk::channel_config_7point1", runtime)
        self.assertIn("m_studio_deferred_capture_bypass", runtime)
        self.assertIn("Surround is a host-delivery operation", runtime)
        self.assertLess(
            runtime.index("m_studio_deferred_capture_bypass"),
            runtime.index("const std::uint64_t genesis_block_start"),
        )
        self.assertNotIn("omniphony_source_spatial_full_sphere", runtime)
        self.assertNotIn("genesis_spatial_route_transport", runtime)
        self.assertNotIn("realtime_musical_omniphony_pipeline", runtime)

    def test_active_stack_has_no_decoder_side_omniphony_or_route_governor(self) -> None:
        master = self.read(PATCHES / "apply_enhanced_component.py")
        self.assertIn('run(here / "apply_spatial_selected_source_transport.py", source)', master)
        self.assertIn('run(here / "apply_spatial_omniphony_runtime.py", source)', master)
        self.assertIn('run(here / "apply_surround_omniphony_bridge.py", source)', master)
        self.assertNotIn('run(here / "apply_spatial_route_order_bridge.py"', master)
        self.assertNotIn('run(here / "apply_spatial_omniphony_rate_lifecycle.py"', master)
        self.assertNotIn('run(here / "apply_foobar_source_session.py"', master)

    def test_ui_label_is_exactly_surround(self) -> None:
        ui = self.read(PATCHES / "apply_enhanced_ui.py")
        self.assertIn('CONTROL         "Surround",IDC_SURROUND_SOUND', ui)
        self.assertNotIn('"""Surround""" sound",IDC_SURROUND_SOUND', ui)
        self.assertNotIn('CONTROL         "Spatial"', ui)


if __name__ == "__main__":
    unittest.main()
