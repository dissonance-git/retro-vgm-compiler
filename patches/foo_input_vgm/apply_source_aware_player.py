#!/usr/bin/env python3
"""Select SourceAwareVGMPlayer when the guarded libvgm ABI is present.

This patch is intentionally tiny. It does not change ordinary VGMPlayer behavior
when libvgm was built without Retro VGM Compiler's source hooks.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    has_utf8_bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", has_utf8_bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, has_utf8_bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if has_utf8_bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    root = args.source_dir.resolve()
    input_cpp = root / "input_vgm.cpp"

    replace_once(
        input_cpp,
        """#include "my_cfg_external.h"
""",
        """#include "my_cfg_external.h"
#ifdef LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI
#include "source_aware_vgm_player.h"
#endif
""",
        "source-aware player include",
    )

    replace_once(
        input_cpp,
        """void input_vgm::register_player()
{
\tm_vgm_player = new VGMPlayer;
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
""",
        """void input_vgm::register_player()
{
#ifdef LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI
\t// SourceAwareVGMPlayer observes the exact same ordinary libvgm render. It
\t// never advances a second shadow chip and leaves protected stereo untouched.
\tm_vgm_player = new SourceAwareVGMPlayer;
#else
\tm_vgm_player = new VGMPlayer;
#endif
#ifdef LIBVGM_GAMEAUDIO_COMMAND_OBSERVER
""",
        "source-aware player registration",
    )

    print("foo_input_vgm source-aware player selection applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
