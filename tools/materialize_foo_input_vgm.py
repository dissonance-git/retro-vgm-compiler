#!/usr/bin/env python3
"""Materialize a buildable foo_input_vgm source tree from canonical inputs.

The exact user-supplied foo_input_vgm 0.31 source archive is the private
component bootstrap. Current Retro VGM Compiler additions live under
components/vgm/ and guarded transformations live under patches/foo_input_vgm/.
This tool combines those sources into a disposable build tree without consulting
the retired vgmspc repository or copying its stale patched host tree.

The materialized layout intentionally mirrors the historical foobar SDK layout:

    <sdk-root>/foo_input_vgm/
    <sdk-root>/enhancement/

External dependencies such as libvgm, WTL and the foobar SDK itself remain the
responsibility of the caller/build workflow. The current patch chain is applied
exactly once to the pristine bootstrap source.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


# These files are project-owned additions. The similarly named base plugin
# files (input_vgm.cpp, input_base.cpp, stdafx.*, etc.) deliberately do not
# appear here: they must come from the immutable bootstrap and be transformed by
# the current guarded patch chain rather than copied from a previously patched
# snapshot.
PROJECT_OWNED_VGM_SOURCE_FILES = (
    "genesis_source_plane.h",
    "ym2151_source_plane.h",
    "input_vgm_qsound_consumer.cpp",
    "input_vgm_shadow.cpp",
    "linear_source_resampler.h",
    "nuked_opn2_hq_lift.h",
    "nuked_opn2_source_capture.h",
    "source_aware_vgm_player.h",
    "studio_alignment_queue.h",
    "studio_frame_transport.h",
    "studio_hq_fm_observer.h",
    "studio_source_resampler.h",
    "studio_source_stream.h",
    "studio_source_timeline.h",
)

REQUIRED_BOOTSTRAP_FILES = (
    "foo_input_vgm.vcxproj",
    "src/input_vgm.cpp",
    "src/input_vgm.h",
    "src/input_base.cpp",
    "src/config_foo_input_vgm.cpp",
    "src/config_foo_input_vgm.rc",
)


def run(command: list[str], *, cwd: Path | None = None) -> None:
    completed = subprocess.run(command, cwd=cwd, check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with exit code {completed.returncode}: "
            + " ".join(command)
        )


def find_bootstrap_root(extracted: Path) -> Path:
    matches = sorted(extracted.rglob("foo_input_vgm.vcxproj"))
    if len(matches) != 1:
        raise RuntimeError(
            "expected exactly one foo_input_vgm.vcxproj in bootstrap archive, "
            f"found {len(matches)}"
        )
    return matches[0].parent


def require_files(root: Path, relative_paths: tuple[str, ...], label: str) -> None:
    missing = [path for path in relative_paths if not (root / path).is_file()]
    if missing:
        joined = ", ".join(missing)
        raise RuntimeError(f"{label} missing required files: {joined}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--sdk-root",
        type=Path,
        required=True,
        help="foobar SDK/workspace root that will contain foo_input_vgm",
    )
    parser.add_argument(
        "--archive",
        type=Path,
        default=None,
        help="override the canonical foo_input_vgm 0.31 bootstrap archive",
    )
    parser.add_argument(
        "--seven-zip",
        default="7z",
        help="7-Zip executable (default: 7z)",
    )
    parser.add_argument(
        "--no-patches",
        action="store_true",
        help="only expand/bootstrap the tree; do not run current VGM patch chain",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    env_archive = os.environ.get("RETRO_VGM_BOOTSTRAP_ARCHIVE")
    selected_archive = args.archive or (Path(env_archive) if env_archive else None)
    archive = (
        selected_archive or (repo / "imports" / "foo_input_vgm-0.31.zip")
    ).resolve()
    sdk_root = args.sdk_root.resolve()
    component = sdk_root / "foo_input_vgm"
    enhancement = sdk_root / "enhancement"
    owned_component = repo / "components" / "vgm" / "foo_input_vgm"
    owned_source = owned_component / "src"
    owned_enhancement = repo / "components" / "vgm" / "enhancement"

    if not archive.is_file():
        raise RuntimeError(f"bootstrap archive not found: {archive}")
    require_files(owned_source, PROJECT_OWNED_VGM_SOURCE_FILES, "project VGM overlay")
    if not (owned_component / "Directory.Build.targets").is_file():
        raise RuntimeError("project VGM overlay missing Directory.Build.targets")
    if not owned_enhancement.is_dir():
        raise RuntimeError(f"Genesis/VGM enhancement directory not found: {owned_enhancement}")

    sdk_root.mkdir(parents=True, exist_ok=True)
    if component.exists():
        shutil.rmtree(component)
    if enhancement.exists():
        shutil.rmtree(enhancement)

    with tempfile.TemporaryDirectory(prefix="retro-vgm-bootstrap-") as temporary:
        extracted = Path(temporary)
        run([args.seven_zip, "x", str(archive), f"-o{extracted}", "-y"])
        bootstrap = find_bootstrap_root(extracted)
        require_files(bootstrap, REQUIRED_BOOTSTRAP_FILES, "foo_input_vgm bootstrap")
        shutil.copytree(bootstrap, component)

    # Keep the historical base pristine until after extraction. Only files
    # which do not belong to that base are overlaid before the guarded patchers
    # run, so each base-file transformation has exactly one authoritative path.
    for filename in PROJECT_OWNED_VGM_SOURCE_FILES:
        shutil.copy2(owned_source / filename, component / "src" / filename)
    shutil.copy2(
        owned_component / "Directory.Build.targets",
        component / "Directory.Build.targets",
    )
    shutil.copytree(owned_enhancement, enhancement)

    if not args.no_patches:
        patcher = repo / "patches" / "foo_input_vgm" / "apply_enhanced_component.py"
        run([sys.executable, str(patcher), str(component / "src")], cwd=repo)

    require_files(component, REQUIRED_BOOTSTRAP_FILES, "materialized foo_input_vgm")
    require_files(component / "src", PROJECT_OWNED_VGM_SOURCE_FILES, "materialized VGM overlay")

    print(f"materialized foo_input_vgm: {component}")
    print(f"materialized VGM enhancement sources: {enhancement}")
    print("vgmspc was not consulted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
