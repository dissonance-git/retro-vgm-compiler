#!/usr/bin/env python3
"""Regression for the vgmspc-free foo_snesapu materialization path."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile


REPO = Path(__file__).resolve().parents[2]
MATERIALIZER = REPO / "tools" / "materialize_foo_snesapu.py"


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="foo_snesapu_materialize_") as tmp:
        root = Path(tmp) / "foo_snesapu"
        completed = subprocess.run(
            [sys.executable, str(MATERIALIZER), str(root)],
            cwd=str(REPO),
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode != 0:
            sys.stderr.write(completed.stdout)
            sys.stderr.write(completed.stderr)
            return completed.returncode

        parent = root / "foobar2000" / "foo_snesapu"
        child = root / "spcplayer"
        required = (
            parent / "input_snesapu.cpp",
            parent / "spcplayer_controller.cpp",
            child / "main.cpp",
            child / "spcplayer.h",
            child / "retro_vgm" / "snesapu_prebrr_packet.h",
            child / "retro_vgm" / "snesapu_studio_source_packet_runtime.h",
        )
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise AssertionError(f"materializer omitted required files: {missing}")

        child_header = (child / "spcplayer.h").read_text(encoding="utf-8")
        child_source = (child / "main.cpp").read_text(encoding="utf-8")
        controller_source = (parent / "spcplayer_controller.cpp").read_text(
            encoding="utf-8-sig"
        )
        input_source = (parent / "input_snesapu.cpp").read_text(
            encoding="utf-8-sig"
        )
        parent_ui = (parent / "resource.rc").read_text(encoding="utf-8-sig")

        assert "SPCP_HEADER_VERSION    3" in child_header
        assert "SPCP_HEADER_STUDIO_SIZE_OFFSET" in child_header
        assert "__stdcall retro_prebrr_callback" in child_source
        assert "SetDSPPreBrrProvider" in child_source
        assert "SetDSPStudioSourceProvider" in child_source
        assert "prepare_spc_studio_sample_reconstruction" in child_source

        assert "m_studio_source_size" in controller_source
        assert "SetStudioSourcePacket" in controller_source
        assert "std::string componentPath = core_api::get_my_full_path();" in controller_source
        assert 'static const std::string fileScheme = "file://";' in controller_source
        assert 'find_last_of("\\\\/")' in controller_source
        assert 'szCmdLine += "spcplayer.exe\\\"";' in controller_source

        # Final host owns only the source-native 7.1 Surround path.
        for marker in (
            "ResetSurroundRuntime",
            "RenderSurroundBlock",
            "EmuAPU_with_sources",
            "audio_chunk::channel_config_7point1",
        ):
            assert marker in input_source, f"SPC materialized runtime missing {marker}"
        for retired in (
            "ResetSpatialRuntime",
            "RenderSpatialBlock",
            "omniphony",
            "m_Enhancer.reset()",
        ):
            assert retired not in input_source, f"retired SPC runtime is active: {retired}"

        # Protected stereo is committed before the source-native 7.1 replacement.
        surround_call = input_source.index(
            "RenderSurroundBlock(p_chunk, wanted_sample);"
        )
        protected_stereo = input_source.rfind(
            "p_chunk.set_data_fixedpoint(", 0, surround_call
        )
        assert protected_stereo >= 0, "SPC Surround call no longer follows protected stereo"
        assert protected_stereo < surround_call

        helper_start = input_source.index("bool input_snesapu::RenderSurroundBlock(")
        helper_end = input_source.index(
            "void input_snesapu::decode_initialize(", helper_start
        )
        helper = input_source[helper_start:helper_end]
        replacement = helper.index("chunk.set_data_floatingpoint_ex(")
        before_replacement = helper[:replacement]
        assert "return false;" in before_replacement
        assert "!m_SourceBlock.valid()" in before_replacement
        assert "move_stereo_to_surround_field" in before_replacement
        assert helper.rfind("return true;") > replacement

        assert '"Surround"' in parent_ui
        assert '"enhanced (later)"' in parent_ui
        assert "WS_DISABLED" in parent_ui

        assert "vgmspc" not in completed.stdout.lower()
        assert "vgmspc" not in completed.stderr.lower()

    print("foo_snesapu materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
