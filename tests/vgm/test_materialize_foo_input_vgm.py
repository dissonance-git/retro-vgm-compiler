#!/usr/bin/env python3
"""Regression for the canonical foo_input_vgm materialization path."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile


REPO = Path(__file__).resolve().parents[2]
MATERIALIZER = REPO / "tools" / "materialize_foo_input_vgm.py"


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="foo_input_vgm_materialize_") as tmp:
        sdk_root = Path(tmp) / "components" / "vgm"
        completed = subprocess.run(
            [sys.executable, str(MATERIALIZER), "--sdk-root", str(sdk_root)],
            cwd=str(REPO),
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            sys.stderr.write(completed.stdout)
            sys.stderr.write(completed.stderr)
            return completed.returncode

        component = sdk_root / "foo_input_vgm"
        source = component / "src"
        enhancement = sdk_root / "enhancement"
        required = (
            component / "foo_input_vgm.vcxproj",
            component / "Directory.Build.targets",
            source / "input_vgm.cpp",
            source / "input_vgm.h",
            source / "input_vgm_shadow.cpp",
            source / "source_aware_vgm_player.h",
            source / "nuked_opn2_source_capture.h",
            enhancement / "selected_source_transport.h",
            enhancement / "genesis_source_episode_7_1.h",
            enhancement / "genesis_source_spread_7_1.h",
        )
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise AssertionError(f"materializer omitted required VGM files: {missing}")

        header = (source / "input_vgm.h").read_text(encoding="utf-8-sig")
        shadow = (source / "input_vgm_shadow.cpp").read_text(encoding="utf-8-sig")
        targets = (component / "Directory.Build.targets").read_text(encoding="utf-8-sig")

        for marker in (
            "selected_source_queue",
            "selected_source_block_storage",
            "genesis_source_spread_7_1",
            "genesis_source_episode_transport",
            "m_supported_chip_surround_eligible",
        ):
            assert marker in header, f"materialized input_vgm.h missing {marker}"

        for marker in (
            "capture_genesis_reference_sources",
            "render_genesis_surround_output",
            "m_genesis_surround_episodes.observe",
            "m_genesis_surround_episodes.prepare_delivered_block",
            "project_genesis_source_spread_7_1",
            "cfg_vgm_enhanced_enabled",
        ):
            assert marker in shadow, f"materialized input_vgm_shadow.cpp missing {marker}"

        for retired in (
            "render_genesis_spatial_output",
            "genesis_realtime_musical_omniphony_pipeline",
            "process_selected_sources_timed",
        ):
            assert retired not in shadow, f"retired VGM runtime is active: {retired}"

        assert '#include "my_cfg_external.h"' in shadow
        assert "class SourceAwareVGMPlayer;" in header
        assert "abort_callback &p_abort) override;" not in header

        core_options = (source / "my_view_core_options.cpp").read_text(
            encoding="utf-8-sig"
        )
        assert "emu/cores/gb.h" in core_options
        assert "emu/cores/gbintf.h" not in core_options
        assert "OPT_VST_WRAM_WRT_WHILE_ON" in core_options
        assert "OPT_VSU_WRAM_WRT_WHILE_ON" not in core_options

        rendered_end = shadow.index("const std::uint64_t rendered_end =")
        studio_branch = shadow.index("if (studio_block)", rendered_end)
        deferred_psg_use = shadow.index(
            "advance_studio_deferred_psg_to(rendered_end)", studio_branch
        )
        assert rendered_end < studio_branch < deferred_psg_use

        call_marker = "render_genesis_surround_output(\n\t\t\tp_chunk,"
        call = shadow.index(call_marker)
        protected_stereo = shadow.rfind(
            "result = input_base::decode_run(p_chunk, p_abort);", 0, call
        )
        assert protected_stereo >= 0, "VGM Surround call must follow protected 0.31 decode"
        assert protected_stereo < call

        helper_start = shadow.index("bool input_vgm::render_genesis_surround_output(")
        helper_end = shadow.index(
            "bool input_vgm::capture_genesis_reference_sources(", helper_start
        )
        helper = shadow[helper_start:helper_end]
        replacement = helper.index("chunk.set_data_floatingpoint_ex(")
        before_replacement = helper[:replacement]
        assert "return false;" in before_replacement
        assert "!cfg_surround_sound || !m_supported_chip_surround_eligible" in before_replacement
        assert "|| !sources_ready || !episodes_ready" in before_replacement
        assert "project_genesis_source_spread_7_1" in before_replacement
        assert "source_topology_supported()" in shadow
        assert helper.rfind("return true;") > replacement

        seek = shadow.index("void input_vgm::decode_seek(")
        assert "m_genesis_surround_episodes.begin_replay();" in shadow[seek:]
        assert "m_genesis_surround_episodes.end_replay();" in shadow[seek:]
        assert "reset_genesis_surround_audio_delivery(genesis_seek_sample);" in shadow[seek:]

        assert "input_vgm_shadow.cpp" in targets
        assert "<PostBuildEventUseInBuild>false</PostBuildEventUseInBuild>" in targets
        assert "<PreprocessorDefinitions>NOMINMAX;%(PreprocessorDefinitions)</PreprocessorDefinitions>" in targets
        assert "<AdditionalDependencies>UxTheme.lib;%(AdditionalDependencies)</AdditionalDependencies>" in targets

        output = completed.stdout + completed.stderr
        assert "materialized foo_input_vgm:" in output
        assert "materialized VGM enhancement sources:" in output

    print("foo_input_vgm materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
