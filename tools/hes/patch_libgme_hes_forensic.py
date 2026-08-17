#!/usr/bin/env python3
"""Add the one HES observation hook missing from pinned libgme.

libgme already exposes GME_APU_HOOK immediately before normal HuC6280 PSG
writes. Its newer PC Engine CD ADPCM path has no equivalent hook. The forensic
build therefore inserts exactly one callback after libgme computes the bounded
local write time and immediately before Hes_Apu_Adpcm::write_data(). Any source
drift fails closed rather than silently changing the evidence boundary.
"""

from __future__ import annotations

import argparse
from pathlib import Path


OLD = """\tif ( (unsigned) (addr - adpcm.io_addr) < adpcm.io_size )
\t{
\t\ttime_t t = min( time(), end_time() + 6 );
\t\tadpcm.write_data( t, addr, data );
\t\treturn;
\t}
"""

NEW = """\tif ( (unsigned) (addr - adpcm.io_addr) < adpcm.io_size )
\t{
\t\ttime_t t = min( time(), end_time() + 6 );
\t\tRETRO_VGM_HES_ADPCM_HOOK( t, addr - adpcm.io_addr, data );
\t\tadpcm.write_data( t, addr, data );
\t\treturn;
\t}
"""


def replace_once(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(OLD)
    if count != 1:
        raise RuntimeError(
            f"expected exactly one pinned HES ADPCM write sentinel in {path}, found {count}"
        )
    path.write_text(text.replace(OLD, NEW), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    replace_once(root / "gme" / "Hes_Emu.cpp")


if __name__ == "__main__":
    main()
