from __future__ import annotations

import ast
import importlib.util
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

    def test_episode_observer_composes_after_pcm_command_boundary(self) -> None:
        runtime = self.read(PATCHES / "apply_spatial_omniphony_runtime.py")
        self.assertIn(
            '"""\\t(void)self->advance_pcm_streams_to(absolute_sample);',
            runtime,
        )
        self.assertIn("m_genesis_surround_episodes.observe", runtime)
        advance = runtime.index(
            '"""\\t(void)self->advance_pcm_streams_to(absolute_sample);'
        )
        observe = runtime.index("m_genesis_surround_episodes.observe", advance)
        capture = runtime.index(
            "\\tif (self->m_source_capture_active)",
            observe,
        )
        self.assertLess(advance, observe)
        self.assertLess(observe, capture)

    def test_runtime_uses_role_free_constant_power_source_spread(self) -> None:
        runtime = self.read(PATCHES / "apply_spatial_omniphony_runtime.py")
        helper = self.read(
            ROOT
            / "components"
            / "vgm"
            / "enhancement"
            / "genesis_source_spread_7_1.h"
        )
        self.assertIn("surround_bed_7_1.h", runtime)
        self.assertIn("genesis_source_spread_7_1.h", runtime)
        self.assertIn("project_genesis_source_spread_7_1", runtime)
        episode = self.read(
            ROOT
            / "components"
            / "vgm"
            / "enhancement"
            / "genesis_source_episode_7_1.h"
        )
        self.assertIn("genesis_source_spread_front_gain", helper)
        self.assertIn("genesis_source_spread_gains_for_depth", helper)
        self.assertIn("genesis_source_episode_transport", episode)
        self.assertIn("choose_depth_slot", episode)
        self.assertIn("begin_replay", episode)
        self.assertIn("end_replay", episode)
        self.assertIn("redistribute_stereo_frame_to_depth", helper)
        self.assertNotIn("move_stereo_to_sides", runtime)
        self.assertNotIn("move_stereo_to_backs", runtime)
        self.assertNotIn("ym2612_fm1", runtime)
        self.assertNotIn("ym2612_dac", runtime)
        self.assertNotIn("sn76489_tone0", runtime)
        self.assertIn("m_genesis_surround_episodes.observe", runtime)
        self.assertIn("prepare_delivered_block", runtime)
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

    def test_seek_insertion_survives_prior_lifecycle_edits(self) -> None:
        spec = importlib.util.spec_from_file_location(
            "vgm_surround_runtime_patch",
            PATCHES / "apply_spatial_omniphony_runtime.py",
        )
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)

        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "input_vgm_shadow.cpp"
            source.write_text(
                "void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)\n"
                "{\n"
                "\tif (m_vgm_player != nullptr)\n"
                "\t{\n"
                "\t\tadvance_shadow_to(123);\n"
                "\t}\n"
                "\treset_pcm_streams();\n"
                "}\n\n"
                "void input_vgm::next_function() {}\n",
                encoding="utf-8",
            )
            module.insert_before_function_close(
                source,
                "void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)\n",
                "\tm_genesis_surround_episodes.end_replay();\n"
                "\treset_genesis_surround_audio_delivery(456);\n",
                "test seek insertion",
            )
            patched = source.read_text(encoding="utf-8")
            seek_close = patched.index("}\n\nvoid input_vgm::next_function")
            rebase = patched.index("reset_genesis_surround_audio_delivery(456);")
            self.assertLess(rebase, seek_close)
            self.assertGreater(rebase, patched.index("reset_pcm_streams();"))

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
