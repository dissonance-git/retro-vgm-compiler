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

        for marker in (
            "capture_genesis_reference_sources",
            "render_genesis_spatial_output",
            "cfg_vgm_enhanced_enabled",
            "genesis_selected_source_block",
        ):
            assert marker in shadow, f"materialized input_vgm_shadow.cpp missing {marker}"

        # The project overlay must compile the modern runtime translation unit
        # without mutating the historical vcxproj text in the builder.
        assert "input_vgm_shadow.cpp" in targets

        # Materialization is permitted to state that the retired repository was
        # not consulted; it must never request or clone it as an input.
        combined = (completed.stdout + completed.stderr).lower()
        assert "clone" not in combined or "vgmspc" not in combined
        assert "vgmspc was not consulted" in combined

    print("foo_input_vgm materialization regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
