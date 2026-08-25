#!/usr/bin/env python3
"""Configure, build, and run the dependency-free VGM Compiler core suites.

This is the broad local/CI route for the repository-owned core before external
source checkout begins. It runs the root CMake/CTest registry, the SNESAPU causal
source-provider contracts, repository representation contracts, and portable
package/import + YM2151 patch guards.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SPC_PROVIDER_CONTRACTS = ROOT / "tests" / "spc_provider_contracts"
ROOT_PYTHON_CONTRACT_PATTERNS = (
    "test_private_component_import_contract.py",
    "test_repository_catalog_projection.py",
)
VGM_PYTHON_CONTRACT_PATTERNS = (
    "test_ym2151_source_tap_patch.py",
    "test_ym2151_reference_capture_patch.py",
)


def run(command: list[str]) -> None:
    completed = subprocess.run(command, cwd=str(ROOT), check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with exit code {completed.returncode}: " + " ".join(command)
        )


def configure_build_test(
    source: Path,
    build: Path,
    *,
    generator: str,
    platform: str,
    config: str,
    extra_configure: tuple[str, ...] = (),
) -> None:
    configure = ["cmake", "-S", str(source), "-B", str(build), *extra_configure]
    if generator:
        configure += ["-G", generator]
    if platform:
        configure += ["-A", platform]
    run(configure)

    build_command = ["cmake", "--build", str(build), "--parallel"]
    if config:
        build_command += ["--config", config]
    run(build_command)

    test_command = ["ctest", "--test-dir", str(build), "--output-on-failure"]
    if config:
        test_command += ["-C", config]
    run(test_command)


def run_python_contract(pattern: str, start_dir: Path) -> None:
    run([
        sys.executable,
        "-B",
        "-m",
        "unittest",
        "discover",
        "-s",
        str(start_dir),
        "-p",
        pattern,
    ])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generator", default="")
    parser.add_argument("--platform", default="")
    parser.add_argument("--config", default="Release")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="vgm-compiler-full-core-") as temporary:
        root = Path(temporary)
        configure_build_test(
            ROOT,
            root / "core",
            generator=args.generator,
            platform=args.platform,
            config=args.config,
            extra_configure=("-DGAMEAUDIO_BUILD_CORE_TESTS=ON",),
        )
        configure_build_test(
            SPC_PROVIDER_CONTRACTS,
            root / "spc-provider-contracts",
            generator=args.generator,
            platform=args.platform,
            config=args.config,
        )

    for pattern in ROOT_PYTHON_CONTRACT_PATTERNS:
        run_python_contract(pattern, ROOT / "tests")
    for pattern in VGM_PYTHON_CONTRACT_PATTERNS:
        run_python_contract(pattern, ROOT / "tests" / "vgm")

    print("VGM Compiler dependency-free core/provider/package contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"VGM Compiler core/provider suite failed: {exc}", file=sys.stderr)
        raise
