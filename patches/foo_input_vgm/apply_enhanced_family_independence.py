#!/usr/bin/env python3
"""Make ordinary enhanced source-family admission transactional.

SourceAwareVGMPlayer::source_block_complete() is an all-family convenience
predicate. It is too strong at the audible replacement boundary: a transient
failure in one captured family must not demote independently proven FM, PSG, or
DAC descendants. Keep topology and output-count agreement global, then let each
family's existing evidence checks decide its own admission.
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
    shadow = parser.parse_args().source_dir.resolve() / "input_vgm_shadow.cpp"

    replace_once(
        shadow,
        """\tauto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
\tif (source_player == nullptr || !source_player->source_topology_supported()
\t\t|| !source_player->source_block_complete()
\t\t|| source_player->source_output_count() != sample_count)
\t\treturn;
""",
        """\tauto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
\tif (source_player == nullptr || !source_player->source_topology_supported()
\t\t|| source_player->source_output_count() != sample_count)
\t\treturn;
""",
        "ordinary family-local source admission",
    )

    print("foo_input_vgm ordinary enhanced family independence applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
