#!/usr/bin/env python3
"""Configure, build and run the dependency-free deletion-gate core suites.

The private component deletion gate is intentionally a superset of ordinary core
validation. This runner reuses the generator/platform that configured the private
frontier, builds the root CMake project in an isolated temporary directory, runs
every registered GAMEAUDIO_BUILD_CORE_TESTS test, then runs the four existing
SNESAPU pre-BRR/studio provider contracts that historically lived outside the
root registry. All of this finishes before external dependency checkout begins.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SPC_PROVIDER_CONTRACTS = ROOT / "tests" / "spc_provider_contracts"


def run(command: list[str]) -> None:
    completed = subprocess.run(command, cwd=str(ROOT), check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with exit code {completed.returncode}: "
            + " ".join(command)
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
    configure = [
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        *extra_configure,
    ]
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generator", default="")
    parser.add_argument("--platform", default="")
    parser.add_argument("--config", default="Release")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="retro-vgm-full-core-") as temporary:
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

    print("full dependency-free core and SPC provider contract suites passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"full core/provider suite failed: {exc}", file=sys.stderr)
        raise
