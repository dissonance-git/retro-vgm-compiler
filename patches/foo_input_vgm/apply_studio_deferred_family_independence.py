#!/usr/bin/env python3
"""Keep deferred FM and PSG quality admission dynamically independent.

SourceAware's source_block_complete() is an all-family convenience predicate. It
is too strong for the deferred Enhanced path: a transient invalid PSG source
block must fail only PSG, not destroy an otherwise exact YM/FM Studio block.
Static source_topology_supported() remains required, then each family proves its
own current block with its own source evidence.
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
    shadow = args.source_dir.resolve() / "input_vgm_shadow.cpp"

    replace_once(
        shadow,
        """\t\t\tstudio_block = source_player != nullptr
\t\t\t\t&& source_player->source_topology_supported()
\t\t\t\t&& source_player->source_block_complete()
\t\t\t\t&& source_player->source_output_count() == rendered_count
\t\t\t\t&& source_player->ym_source_expected()
\t\t\t\t&& source_player->ym_source_block_valid()
\t\t\t\t&& source_player->hq_fm_source_block_valid()
""",
        """\t\t\tstudio_block = source_player != nullptr
\t\t\t\t&& source_player->source_topology_supported()
\t\t\t\t&& source_player->source_output_count() == rendered_count
\t\t\t\t&& source_player->ym_source_expected()
\t\t\t\t&& source_player->ym_source_block_valid()
\t\t\t\t&& source_player->hq_fm_source_block_valid()
""",
        "deferred FM family-local block admission",
    )

    print("foo_input_vgm deferred Enhanced family independence applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
