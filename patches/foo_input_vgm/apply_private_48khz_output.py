#!/usr/bin/env python3
"""Pin the private foo_input_vgm host output contract to 48 kHz.

This is deliberately below source-quality and Spatial policy. Reference and
enhanced playback both render to the same final 48 kHz host timeline; Spatial
receives that already-selected timeline. The stored historical UI preference is
left intact but is not allowed to change this private build's playback rate.
"""

from __future__ import annotations

import argparse
from pathlib import Path


PRIVATE_PLAYBACK_RATE = 48000


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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    root = parser.parse_args().source_dir.resolve()
    base = root / "input_base.cpp"

    replace_once(
        base,
        "\tm_sample_rate = cfg_sample_rate;\n",
        f"\t// Private playback contract: one final host timeline for every mode.\n"
        f"\tm_sample_rate = {PRIVATE_PLAYBACK_RATE};\n",
        "private VGM 48 kHz output rate",
    )

    print("foo_input_vgm private 48 kHz output contract applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
