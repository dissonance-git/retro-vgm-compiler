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

        # Final child protocol/ABI, not merely patch-script presence.
        assert "SPCP_HEADER_VERSION    3" in child_header
        assert "SPCP_HEADER_STUDIO_SIZE_OFFSET" in child_header
        assert "__stdcall retro_prebrr_callback" in child_source
        assert "SetDSPPreBrrProvider" in child_source
        assert "SetDSPStudioSourceProvider" in child_source
        assert "prepare_spc_studio_sample_reconstruction" in child_source

        # Final parent source transport and native sibling process geometry.
        assert "m_studio_source_size" in controller_source
        assert "SetStudioSourcePacket" in controller_source
        assert "std::string componentPath = core_api::get_my_full_path();" in controller_source
        assert 'static const std::string fileScheme = "file://";' in controller_source
        assert 'find_last_of("\\\\/")' in controller_source
        assert 'szCmdLine += "spcplayer.exe\\\"";' in controller_source

        # Final composed host must contain current Spatial state and no reference
        # to the removed SemanticStereoEnhancer member after seek cleanup.
        assert "ResetSpatialRuntime" in input_source
        assert "RenderSpatialBlock" in input_source
        assert "m_Enhancer.reset()" not in input_source

        # UI/source-quality naming remains descriptive rather than a tier name.
        assert '"enhanced"' in parent_ui

        # The materializer is allowed to explain that a migration source was not
        # consulted, but it must never print or require the retired repository.
        assert "vgmspc" not in completed.stdout.lower()
        assert "vgmspc" not in completed.stderr.lower()

    print("foo_snesapu materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
