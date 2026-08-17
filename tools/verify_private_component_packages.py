#!/usr/bin/env python3
"""Verify the final private foobar component archives before bundling them.

The build stages files in disposable directories, but the deletion gate is about
what is actually shipped.  This verifier reopens each renamed ZIP
(`.fb2k-component`) and checks the exact sibling payload expected by the runtime.
It also rejects path traversal, duplicate case-insensitive names, nested layout,
and zero-byte runtime files.
"""

from __future__ import annotations

import argparse
from pathlib import Path, PurePosixPath
import zipfile


VGM_EXPECTED = {
    "foo_input_vgm.dll",
    "omniphony_source.dll",
}

SPC_EXPECTED = {
    "foo_snesapu.dll",
    "spcplayer.exe",
    "SNESAPU.dll",
    "omniphony_source.dll",
}


def verify_archive(path: Path, expected: set[str], label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"{label} package missing: {path}")
    if not zipfile.is_zipfile(path):
        raise RuntimeError(f"{label} package is not a ZIP/fb2k-component archive: {path}")

    with zipfile.ZipFile(path, "r") as archive:
        infos = [info for info in archive.infolist() if not info.is_dir()]
        names = [PurePosixPath(info.filename).as_posix() for info in infos]

        unsafe = [
            name
            for name in names
            if not name
            or name.startswith("/")
            or ".." in PurePosixPath(name).parts
            or len(PurePosixPath(name).parts) != 1
        ]
        if unsafe:
            raise AssertionError(f"{label} package has unsafe/nested entries: {unsafe}")

        folded: dict[str, str] = {}
        duplicates: list[tuple[str, str]] = []
        for name in names:
            key = name.casefold()
            previous = folded.get(key)
            if previous is not None:
                duplicates.append((previous, name))
            else:
                folded[key] = name
        if duplicates:
            raise AssertionError(
                f"{label} package has case-insensitive duplicate entries: {duplicates}"
            )

        actual = set(names)
        if actual != expected:
            missing = sorted(expected - actual)
            extra = sorted(actual - expected)
            raise AssertionError(
                f"{label} package payload mismatch; missing={missing}, extra={extra}"
            )

        empty = sorted(info.filename for info in infos if info.file_size == 0)
        if empty:
            raise AssertionError(f"{label} package contains zero-byte runtime files: {empty}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("vgm_component", type=Path)
    parser.add_argument("spc_component", type=Path)
    args = parser.parse_args()

    verify_archive(args.vgm_component.resolve(), VGM_EXPECTED, "VGM")
    verify_archive(args.spc_component.resolve(), SPC_EXPECTED, "SPC")
    print("private foobar component package payloads verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
