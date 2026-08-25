#!/usr/bin/env python3
"""Add minimal source-render hooks to a pinned libvgm VGMPlayer checkout.

The hooks are inert in libvgm itself. A derived foobar player can install an
exact device source tap after Resmpl_DevConnect but before Resmpl_Init, then
mirror every exact Resmpl_Execute segment without copying VGMPlayer::Render.

Pass the libvgm checkout root itself, not the VGM Compiler repository.
Every edit is an exact singular replacement and therefore fails closed on
upstream drift.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    root = args.libvgm_root.resolve()
    hpp = root / "player" / "vgmplayer.hpp"
    cpp = root / "player" / "vgmplayer.cpp"

    h = hpp.read_text(encoding="utf-8")
    marker = "\tUINT32 Render(UINT32 smplCnt, WAVE_32BS* data);\n\t\nprotected:\n"
    hooks = (
        "\tUINT32 Render(UINT32 smplCnt, WAVE_32BS* data);\n\t\n"
        "protected:\n"
        "\t// VGM Compiler source-render extension points. Base libvgm behavior is\n"
        "\t// unchanged; derived players may observe/replace a device StreamUpdate\n"
        "\t// before Resmpl_Init and mirror the exact per-command render segments.\n"
        "\tvirtual void SourceTapOnResamplerConnected(CHIP_DEVICE&, VGM_BASEDEV&, UINT8) {}\n"
        "\tvirtual void SourceTapOnResampleBegin(CHIP_DEVICE&, VGM_BASEDEV&, UINT32, UINT32) {}\n"
        "\tvirtual void SourceTapOnResampleEnd(CHIP_DEVICE&, VGM_BASEDEV&, UINT32, UINT32) {}\n"
        "\t\n"
    )
    if "SourceTapOnResamplerConnected" not in h:
        h = replace_once(h, marker, hooks, "VGMPlayer hook declarations")

    # Let the foobar shell select the source-aware subclass only when the exact
    # patched ABI is present. Unpatched libvgm therefore remains buildable as the
    # protected-reference fallback rather than failing on missing virtual hooks.
    if "LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI" not in h:
        h = "#define LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI 1\n" + h

    c = cpp.read_text(encoding="utf-8")
    connect_old = (
        "\t\t\tResmpl_SetVals(&clDev->resmpl, resmplMode, chipVol, _outSmplRate);\n"
        "\t\t\tResmpl_DevConnect(&clDev->resmpl, &clDev->defInf);\n"
        "\t\t\tResmpl_Init(&clDev->resmpl);\n"
    )
    connect_new = (
        "\t\t\tResmpl_SetVals(&clDev->resmpl, resmplMode, chipVol, _outSmplRate);\n"
        "\t\t\tResmpl_DevConnect(&clDev->resmpl, &clDev->defInf);\n"
        "\t\t\tSourceTapOnResamplerConnected(chipDev, *clDev, linkCntr);\n"
        "\t\t\tResmpl_Init(&clDev->resmpl);\n"
    )
    if "SourceTapOnResamplerConnected(chipDev" not in c:
        c = replace_once(c, connect_old, connect_new, "resampler-connect hook")

    render_old = (
        "\t\t\tfor (clDev = &cDev->base; clDev != NULL; clDev = clDev->linkDev, disable >>= 1)\n"
        "\t\t\t{\n"
        "\t\t\t\tif (clDev->defInf.dataPtr != NULL && ! (disable & 0x01))\n"
        "\t\t\t\t\tResmpl_Execute(&clDev->resmpl, smplStep, &data[curSmpl]);\n"
        "\t\t\t}\n"
    )
    render_new = (
        "\t\t\tfor (clDev = &cDev->base; clDev != NULL; clDev = clDev->linkDev, disable >>= 1)\n"
        "\t\t\t{\n"
        "\t\t\t\tif (clDev->defInf.dataPtr != NULL && ! (disable & 0x01))\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\tSourceTapOnResampleBegin(*cDev, *clDev, curSmpl, (UINT32)smplStep);\n"
        "\t\t\t\t\tResmpl_Execute(&clDev->resmpl, smplStep, &data[curSmpl]);\n"
        "\t\t\t\t\tSourceTapOnResampleEnd(*cDev, *clDev, curSmpl, (UINT32)smplStep);\n"
        "\t\t\t\t}\n"
        "\t\t\t}\n"
    )
    if "SourceTapOnResampleBegin(*cDev" not in c:
        c = replace_once(c, render_old, render_new, "per-segment source hooks")

    hpp.write_text(h, encoding="utf-8", newline="\n")
    cpp.write_text(c, encoding="utf-8", newline="\n")
    print("patched libvgm VGMPlayer source-resampler hooks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
