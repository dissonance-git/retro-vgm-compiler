#!/usr/bin/env python3
"""Build and run every dependency-free VGM core test without foobar/libvgm.

This is the fallback validation loop while GitHub-hosted Actions runners are
unavailable. It intentionally compiles each test against the complete local
source-native core so newly added translation units cannot quietly sit outside
the test build.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ENHANCEMENT = ROOT / "components" / "vgm" / "enhancement"
TEST_DIR = ROOT / "tests" / "vgm"

CORE_SOURCES = [
    ENHANCEMENT / "genesis_state.cpp",
    ENHANCEMENT / "sn76489_enhanced.cpp",
    ENHANCEMENT / "sn76489_advance.cpp",
    ENHANCEMENT / "ym2612_dac_enhanced.cpp",
    ENHANCEMENT / "ym2612_pcm_stream.cpp",
    ENHANCEMENT / "ym2612_fm_timeline.cpp",
    ENHANCEMENT / "source_stem_mixer.cpp",
]


def find_compiler(explicit: str | None) -> tuple[str, str]:
    candidates = [explicit] if explicit else []
    candidates += [os.environ.get("CXX"), "clang++", "g++", "cl"]
    for candidate in candidates:
        if not candidate:
            continue
        resolved = shutil.which(candidate)
        if resolved:
            name = Path(resolved).name.lower()
            return resolved, "msvc" if name in {"cl", "cl.exe"} else "gnu"
    raise RuntimeError("No C++ compiler found. Set CXX or pass --compiler.")


def compile_test(
    compiler: str,
    family: str,
    test: Path,
    output: Path,
    cwd: Path,
) -> None:
    sources = [str(path) for path in CORE_SOURCES] + [str(test)]
    if family == "msvc":
        command = [
            compiler,
            "/nologo",
            "/std:c++17",
            "/EHsc",
            "/O2",
            "/W4",
            "/WX",
            "/permissive-",
            f"/I{ENHANCEMENT}",
            *sources,
            f"/Fe:{output}",
        ]
    else:
        command = [
            compiler,
            "-std=c++17",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            f"-I{ENHANCEMENT}",
            *sources,
            "-o",
            str(output),
        ]
    subprocess.run(command, cwd=cwd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", help="C++ compiler executable")
    parser.add_argument("--keep", action="store_true", help="keep temporary build directory")
    args = parser.parse_args()

    missing = [path for path in CORE_SOURCES if not path.is_file()]
    if missing:
        print("Missing core source files:", file=sys.stderr)
        for path in missing:
            print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        return 2

    tests = sorted(TEST_DIR.glob("*_test.cpp"))
    if not tests:
        print("No dependency-free tests found.", file=sys.stderr)
        return 2

    try:
        compiler, family = find_compiler(args.compiler)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    build_root = Path(tempfile.mkdtemp(prefix="gameaudio-core-tests-"))
    print(f"compiler: {compiler}")
    print(f"tests: {len(tests)}")
    print(f"build: {build_root}")

    failed: list[str] = []
    try:
        for test in tests:
            name = test.stem
            case_dir = build_root / name
            case_dir.mkdir(parents=True, exist_ok=True)
            executable = case_dir / (name + (".exe" if os.name == "nt" else ""))
            print(f"[build] {name}")
            try:
                compile_test(compiler, family, test, executable, case_dir)
                print(f"[run]   {name}")
                subprocess.run([str(executable)], cwd=case_dir, check=True)
            except subprocess.CalledProcessError as exc:
                failed.append(name)
                print(f"[FAIL]  {name}: exit {exc.returncode}", file=sys.stderr)
            else:
                print(f"[pass]  {name}")
    finally:
        if args.keep:
            print(f"kept build directory: {build_root}")
        else:
            shutil.rmtree(build_root, ignore_errors=True)

    if failed:
        print("\nFailed tests:", file=sys.stderr)
        for name in failed:
            print(f"  {name}", file=sys.stderr)
        return 1

    print(f"\nAll {len(tests)} core tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
