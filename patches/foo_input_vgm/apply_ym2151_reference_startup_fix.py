#!/usr/bin/env python3
"""Promote YM2151's one-sample linear-resampler startup pre-generation."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    root = parser.parse_args().source_dir.resolve()
    header = root / "source_aware_vgm_player.h"
    text = header.read_text(encoding="utf-8")
    old = (
        "        if (m_starting) {\n"
        "            promote_initial_pregen(m_ym);\n"
        "            promote_initial_pregen(m_psg);\n"
        "        }\n"
    )
    new = (
        "        if (m_starting) {\n"
        "            promote_initial_pregen(m_ym);\n"
        "            promote_initial_pregen(m_psg);\n"
        "            promote_initial_pregen(m_opm);\n"
        "        }\n"
    )
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"YM2151 startup pregen: expected exactly one match in {header}, found {count}"
        )
    header.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")
    print("patched YM2151 linear-resampler startup pre-generation")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
