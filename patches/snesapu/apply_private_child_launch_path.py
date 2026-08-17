#!/usr/bin/env python3
"""Normalize foo_snesapu's component URI before launching sibling spcplayer.exe.

foobar2000 core_api::get_my_full_path() identifies the calling component DLL but
uses foobar's filesystem path form, which may carry a ``file://`` prefix. The
Win32 CreateProcess call below expects a native path. Keep the useful sibling
lookup while stripping only that filesystem protocol marker before converting
the final UTF-8 command line to the OS-native string type.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    controller = root / "foobar2000" / "foo_snesapu" / "spcplayer_controller.cpp"

    old = r'''	std::string szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	size_t slash = szCmdLine.find_last_of('\\');
	if (slash != std::string::npos)
		szCmdLine.erase(szCmdLine.begin() + slash + 1, szCmdLine.end());
	szCmdLine += "spcplayer.exe\"";
'''
    new = r'''	// core_api identifies this component DLL, not foobar2000.exe. Its path
	// is in foobar filesystem form, so remove the local-file protocol marker
	// before handing the eventual command line to Win32 CreateProcess.
	std::string componentPath = core_api::get_my_full_path();
	static const std::string fileScheme = "file://";
	if (componentPath.compare(0, fileScheme.size(), fileScheme) == 0)
		componentPath.erase(0, fileScheme.size());

	std::string szCmdLine = "\"";
	szCmdLine += componentPath;
	size_t slash = szCmdLine.find_last_of("\\/");
	if (slash != std::string::npos)
		szCmdLine.erase(szCmdLine.begin() + slash + 1, szCmdLine.end());
	szCmdLine += "spcplayer.exe\"";
'''
    replace_once(
        controller,
        old,
        new,
        "SPC sibling child native launch path",
    )

    print("foo_snesapu sibling spcplayer launch path normalized")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
