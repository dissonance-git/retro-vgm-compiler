#!/usr/bin/env python3
"""Expose PlayerA's exact post-render song/fade gain as a read-only query.

The source plane is captured inside VGMPlayer, before PlayerA applies track gain
and its sample-by-sample logarithmic fade. The foobar replacement path needs the
same 16.16 gain value before subtracting/replacing a source contribution. No
PlayerA rendering behavior is changed.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    path = args.libvgm_root.resolve() / "player" / "playera.hpp"
    text = path.read_text(encoding="utf-8")

    if "GetCurrentVolumeAt" not in text:
        old = "\tUINT32 Render(UINT32 bufSize, void* data);\nprivate:\n"
        new = (
            "\tUINT32 Render(UINT32 bufSize, void* data);\n"
            "\t// Exact 16.16 gain PlayerA applies after VGMPlayer::Render.\n"
            "\tINT32 GetCurrentVolumeAt(UINT32 playbackSmpl) { return CalcCurrentVolume(playbackSmpl); }\n"
            "private:\n"
        )
        count = text.count(old)
        if count != 1:
            raise RuntimeError(f"PlayerA gain-view anchor: expected 1 match, found {count}")
        text = text.replace(old, new, 1)
        path.write_text(text, encoding="utf-8", newline="\n")

    print("patched PlayerA exact gain view")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
