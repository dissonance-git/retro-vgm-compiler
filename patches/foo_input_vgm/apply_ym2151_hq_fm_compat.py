#!/usr/bin/env python3
"""Normalize two OPM-added host shapes so the pinned Genesis HQ-FM patch applies.

This is transformation-order glue only. It temporarily lets the existing HQ-FM
patch own the shared overflow/member anchors; the post-HQ OPM fix then restores
OPM-specific invalidation. No audio or source-selection semantics change here.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    header = parser.parse_args().source_dir.resolve() / "source_aware_vgm_player.h"

    replace_once(
        header,
        "            if (rendered > kOutputCapacity) {\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n"
        "                m_opm_block_valid = false;\n"
        "            }\n",
        "            if (rendered > kOutputCapacity)\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n",
        "pre-HQ shared overflow anchor",
    )

    replace_once(
        header,
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_unsupported_opm_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n"
        "    bool m_opm_block_valid = false;\n",
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n"
        "    bool m_unsupported_opm_topology = false;\n"
        "    bool m_opm_block_valid = false;\n",
        "pre-HQ shared validity-member anchor",
    )

    print("prepared YM2151 host additions for exact Genesis HQ-FM patch ordering")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
