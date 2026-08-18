#!/usr/bin/env python3
"""Restore YM2151 startup/overflow handling after the Genesis HQ-FM transform."""

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
        "        if (m_starting) {\n"
        "            promote_initial_pregen(m_ym);\n"
        "            promote_initial_hq_pregen(m_ym);\n"
        "            promote_initial_pregen(m_psg);\n"
        "        }\n",
        "        if (m_starting) {\n"
        "            promote_initial_pregen(m_ym);\n"
        "            promote_initial_hq_pregen(m_ym);\n"
        "            promote_initial_pregen(m_psg);\n"
        "            promote_initial_pregen(m_opm);\n"
        "        }\n",
        "YM2151 startup pre-generation after HQ-FM",
    )

    replace_once(
        header,
        "            if (rendered > kOutputCapacity) {\n"
        "                m_ym_block_valid = false;\n"
        "                m_hq_fm_block_valid = false;\n"
        "                m_psg_block_valid = false;\n"
        "            }\n",
        "            if (rendered > kOutputCapacity) {\n"
        "                m_ym_block_valid = false;\n"
        "                m_hq_fm_block_valid = false;\n"
        "                m_psg_block_valid = false;\n"
        "                m_opm_block_valid = false;\n"
        "            }\n",
        "YM2151 overflow invalidation after HQ-FM",
    )

    print("restored YM2151 startup and overflow handling after Genesis HQ-FM patch")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
