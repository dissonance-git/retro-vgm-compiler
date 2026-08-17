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
        parent_source = (parent / "spcplayer_controller.cpp").read_text(encoding="utf-8-sig")
        parent_ui = (parent / "resource.rc").read_text(encoding="utf-8-sig")

        assert "SPCP_HEADER_VERSION    3" in child_header
        assert "SPCP_HEADER_STUDIO_SIZE_OFFSET" in child_header
        assert "__stdcall retro_prebrr_callback" in child_source
        assert "SetDSPStudioSourceProvider" in child_source
        assert "m_studio_source_size" in parent_source
        assert '"enhanced"' in parent_ui
        assert "vgmspc" not in completed.stdout.lower()

    print("foo_snesapu materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
