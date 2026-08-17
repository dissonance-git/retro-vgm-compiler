#!/usr/bin/env python3
"""Expose one in-place PlayerA post-engine / pre-volume processing hook.

The callback runs immediately after PlayerBase::Render fills PlayerA's internal
WAVE_32BS buffer, before song gain, logarithmic fade, channel inversion, clipping
or output packing. That is the exact domain needed for source subtraction and
replacement from SourceAwareVGMPlayer.
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
    hpp = root / "player" / "playera.hpp"
    cpp = root / "player" / "playera.cpp"

    h = hpp.read_text(encoding="utf-8")
    if "LIBVGM_GAMEAUDIO_POSTRENDER_ABI" not in h:
        h = "#define LIBVGM_GAMEAUDIO_POSTRENDER_ABI 1\n" + h

    if "PLR_POST_RENDER_PROCESSOR" not in h:
        h = replace_once(
            h,
            "\ttypedef void (*PLR_SMPL_PACK)(void* buffer, INT32 value);\n",
            "\ttypedef void (*PLR_SMPL_PACK)(void* buffer, INT32 value);\n"
            "\t// In-place engine-output processor. The sample buffer is in the same\n"
            "\t// WAVE_32BS domain returned by PlayerBase::Render, before PlayerA volume.\n"
            "\ttypedef void (*PLR_POST_RENDER_PROCESSOR)(void* user, WAVE_32BS* samples, UINT32 sampleCount, UINT32 basePlaybackSample);\n",
            "PlayerA post-render callback typedef",
        )
        h = replace_once(
            h,
            "\tvoid SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam);\n",
            "\tvoid SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam);\n"
            "\tvoid SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam);\n",
            "PlayerA post-render setter declaration",
        )
        h = replace_once(
            h,
            "\tPLAYER_EVENT_CB _plrCbFunc;\n\tvoid* _plrCbParam;\n",
            "\tPLAYER_EVENT_CB _plrCbFunc;\n\tvoid* _plrCbParam;\n"
            "\tPLR_POST_RENDER_PROCESSOR _postRenderFunc;\n\tvoid* _postRenderParam;\n",
            "PlayerA post-render state",
        )

    c = cpp.read_text(encoding="utf-8")
    if "_postRenderFunc = NULL" not in c:
        c = replace_once(
            c,
            "\t_plrCbFunc = NULL;\n\t_plrCbParam = NULL;\n\t_myPlayState = 0x00;\n",
            "\t_plrCbFunc = NULL;\n\t_plrCbParam = NULL;\n"
            "\t_postRenderFunc = NULL;\n\t_postRenderParam = NULL;\n\t_myPlayState = 0x00;\n",
            "PlayerA post-render initialization",
        )

    if "void PlayerA::SetPostRenderProcessor" not in c:
        anchor = (
            "void PlayerA::SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam)\n"
            "{\n"
            "\t_plrLogFunc = cbFunc;\n"
            "\t_plrLogParam = cbParam;\n"
            "\treturn;\n"
            "}\n"
        )
        # Some libvgm revisions route SetLogCallback directly through players
        # and therefore have a different body. Fall back to inserting before
        # GetState, whose declaration order is stable in the pinned checkout.
        if anchor in c:
            insertion = anchor + (
                "\nvoid PlayerA::SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam)\n"
                "{\n"
                "\t_postRenderFunc = cbFunc;\n"
                "\t_postRenderParam = cbParam;\n"
                "\treturn;\n"
                "}\n"
            )
            c = replace_once(c, anchor, insertion, "PlayerA post-render setter")
        else:
            marker = "UINT8 PlayerA::GetState(void) const\n"
            insertion = (
                "void PlayerA::SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam)\n"
                "{\n"
                "\t_postRenderFunc = cbFunc;\n"
                "\t_postRenderParam = cbParam;\n"
                "\treturn;\n"
                "}\n\n"
                + marker
            )
            c = replace_once(c, marker, insertion, "PlayerA post-render setter fallback")

    if "_postRenderFunc(_postRenderParam" not in c:
        c = replace_once(
            c,
            "\tsmplRendered = _player->Render(smplCount, &_smplBuf[0]);\n\tsmplCount = smplRendered;\n\t\n\tcurVolume = CalcCurrentVolume(basePbSmpl) >> VOL_SHIFT;\n",
            "\tsmplRendered = _player->Render(smplCount, &_smplBuf[0]);\n\tsmplCount = smplRendered;\n"
            "\tif (_postRenderFunc != NULL && smplCount != 0)\n"
            "\t\t_postRenderFunc(_postRenderParam, &_smplBuf[0], smplCount, basePbSmpl);\n\t\n"
            "\tcurVolume = CalcCurrentVolume(basePbSmpl) >> VOL_SHIFT;\n",
            "PlayerA post-render invocation",
        )

    hpp.write_text(h, encoding="utf-8", newline="\n")
    cpp.write_text(c, encoding="utf-8", newline="\n")
    print("patched PlayerA pre-volume post-render processor")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
