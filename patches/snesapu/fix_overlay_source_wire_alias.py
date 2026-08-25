#!/usr/bin/env python3
"""Make the staged SPC model reuse the one materialized SRCE-v2 ABI header.

The x64 parent includes spcplayer.h, which already reaches the root-level
snesapu_source_wire_v2.h shared with the x86 child. The current model overlay is
a copy of components/spc and therefore otherwise carries a second physical copy
of that same struct definition. #pragma once cannot deduplicate two different
files, so the staged copy must be a forwarding header rather than another ABI
owner.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()

    canonical = root / "snesapu_source_wire_v2.h"
    staged = root / "retro_vgm" / "components" / "spc" / "snesapu_source_wire_v2.h"
    if not canonical.is_file() or not staged.is_file():
        raise RuntimeError(
            f"SRCE-v2 header geometry missing: canonical={canonical}, staged={staged}"
        )

    canonical_bytes = canonical.read_bytes()
    staged_bytes = staged.read_bytes()
    if staged_bytes != canonical_bytes:
        raise RuntimeError(
            "staged SRCE-v2 header drifted from the materialized canonical ABI before alias repair"
        )

    newline = b"\r\n" if b"\r\n" in staged_bytes else b"\n"
    forwarder = newline.join(
        (
            b"#pragma once",
            b"",
            b"// The process ABI is owned by the materialized root header shared with spcplayer.",
            b"#include \"../../../snesapu_source_wire_v2.h\"",
            b"",
        )
    )
    staged.write_bytes(forwarder)
    print(f"staged SRCE-v2 wire header now aliases canonical process ABI: {staged}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
