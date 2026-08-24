#!/usr/bin/env python3
"""Regression for the vgmspc-free foo_input_vgm materialization path."""

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
            enhancement / "genesis_selected_source_queue.h",
            enhancement / "genesis_selected_source_block.h",
            enhancement / "genesis_spatial_route_transport.h",
            enhancement / "genesis_realtime_musical_omniphony_pipeline.h",
        )
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise AssertionError(f"materializer omitted required VGM files: {missing}")

        header = (source / "input_vgm.h").read_text(encoding="utf-8-sig")
        shadow = (source / "input_vgm_shadow.cpp").read_text(encoding="utf-8-sig")
        targets = (component / "Directory.Build.targets").read_text(encoding="utf-8-sig")

        # Final host composition, not merely project-owned source-file presence.
        for marker in (
            "genesis_selected_source_queue",
            "genesis_selected_source_block",
            "genesis_spatial_route_transport",
            "genesis_realtime_musical_omniphony_pipeline",
        ):
            assert marker in header, f"materialized input_vgm.h missing {marker}"

        # The current audible seam consumes the finalized delivered source bank
        # directly into timed Omniphony processing. genesis_selected_source_block
        # remains a shared type/header contract but is no longer instantiated as
        # a named local in input_vgm_shadow.cpp.
        for marker in (
            "capture_genesis_reference_sources",
            "render_genesis_spatial_output",
            "cfg_vgm_enhanced_enabled",
            "m_genesis_delivered_sources.consume",
            "process_selected_sources_timed",
        ):
            assert marker in shadow, f"materialized input_vgm_shadow.cpp missing {marker}"

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

        # Fail closed at the final audible seam. input_base::decode_run is the
        # protected foo_input_vgm 0.31 renderer and must complete before Spatial
        # is attempted. The Spatial helper itself does not touch the chunk until
        # every source, route and Omniphony condition succeeds.
        call_marker = "render_genesis_spatial_output(\n\t\t\tp_chunk,"
        call = shadow.index(call_marker)
        protected_stereo = shadow.rfind(
            "result = input_base::decode_run(p_chunk, p_abort);", 0, call
        )
        assert protected_stereo >= 0, "VGM Spatial call no longer follows protected 0.31 decode"
        assert protected_stereo < call

        helper_start = shadow.index("bool input_vgm::render_genesis_spatial_output(")
        helper_end = shadow.index(
            "bool input_vgm::capture_genesis_reference_sources(", helper_start
        )
        helper = shadow[helper_start:helper_end]
        replacement = helper.index("chunk.set_data_floatingpoint_ex(")
        assert "return false;" in helper[:replacement]
        assert "!rendered.source_block_valid || !rendered.omniphony.rendered" in helper[:replacement]
        assert helper.rfind("return true;") > replacement

        # The project overlay must compile the modern runtime translation unit
        # without mutating the historical vcxproj text in the builder.
        assert "input_vgm_shadow.cpp" in targets
        assert "<PreprocessorDefinitions>NOMINMAX;%(PreprocessorDefinitions)</PreprocessorDefinitions>" in targets

        # Materialization is permitted to state that the retired repository was
        # not consulted; it must never request or clone it as an input.
        combined = (completed.stdout + completed.stderr).lower()
        assert "clone" not in combined or "vgmspc" not in combined
        assert "vgmspc was not consulted" in combined

    print("foo_input_vgm materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
