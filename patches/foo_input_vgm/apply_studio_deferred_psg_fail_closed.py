#!/usr/bin/env python3
"""Fail deferred Enhanced PSG immediately when exact subtraction authority is lost.

Run after apply_studio_deferred_psg.py. Command-time engine-clock synthesis may
already have queued a prefix of the current render before SourceAware's completed
block validity is known. If the exact PSG source block is invalid, keeping that
queue alive would leave unconsumed older ordinals in front of the next block.
Treat the first invalid exact block as a family-local quality failure instead:
protected reference PSG survives and Studio FM remains independently eligible.
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
        """\t\tbool deferred_psg_block = m_studio_deferred_psg_active
\t\t\t&& rendered_count <= m_enhanced_family_scratch.size()
\t\t\t&& source_player != nullptr
\t\t\t&& m_psg_present[0]
\t\t\t&& m_psg_shadow_valid[0]
\t\t\t&& source_player->psg_source_expected()
\t\t\t&& source_player->psg_source_block_valid()
\t\t\t&& source_player->source_output_count() == rendered_count;
\t\tif (deferred_psg_block && !advance_studio_deferred_psg_to(rendered_end))
""",
        """\t\tbool deferred_psg_block = m_studio_deferred_psg_active
\t\t\t&& rendered_count <= m_enhanced_family_scratch.size()
\t\t\t&& source_player != nullptr
\t\t\t&& m_psg_present[0]
\t\t\t&& m_psg_shadow_valid[0]
\t\t\t&& source_player->psg_source_expected()
\t\t\t&& source_player->psg_source_block_valid()
\t\t\t&& source_player->source_output_count() == rendered_count;
\t\tif (m_studio_deferred_psg_active && !deferred_psg_block)
\t\t\tfail_studio_deferred_psg();
\t\tif (deferred_psg_block && !advance_studio_deferred_psg_to(rendered_end))
""",
        "deferred PSG exact-block fail close",
    )

    print("foo_input_vgm deferred Enhanced PSG fail-close guard applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
