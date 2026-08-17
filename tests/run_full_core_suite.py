#!/usr/bin/env python3
"""Configure, build and run the repository's full dependency-free core suite.

The private component deletion gate is intentionally a superset of ordinary core
validation. This runner reuses the generator/platform that configured the private
frontier, builds the root CMake project in an isolated temporary directory, and
runs every registered GAMEAUDIO_BUILD_CORE_TESTS test before external dependency
checkout can begin.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def run(command: list[str]) -> None:
    completed = subprocess.run(command, cwd=str(ROOT), check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with exit code {completed.returncode}: "
            + " ".join(command)
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generator", default="")
    parser.add_argument("--platform", default="")
    parser.add_argument("--config", default="Release")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="retro-vgm-full-core-") as temporary:
        build = Path(temporary) / "build"
        configure = [
            "cmake",
            "-S",
            str(ROOT),
            "-B",
            str(build),
            "-DGAMEAUDIO_BUILD_CORE_TESTS=ON",
        ]
        if args.generator:
            configure += ["-G", args.generator]
        if args.platform:
            configure += ["-A", args.platform]
        run(configure)

        build_command = ["cmake", "--build", str(build), "--parallel"]
        if args.config:
            build_command += ["--config", args.config]
        run(build_command)

        test_command = ["ctest", "--test-dir", str(build), "--output-on-failure"]
        if args.config:
            test_command += ["-C", args.config]
        run(test_command)

    print("full dependency-free root core suite passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"full core suite failed: {exc}", file=sys.stderr)
        raise
