#!/usr/bin/env python3
"""Add an optional exact eight-channel source tap to libvgm's MAME YM2151 core.

The tap observes the complete OPM channel outputs after channel synthesis (and
therefore after algorithm/operator/feedback/LFO/envelope/noise semantics) but
before the ordinary stereo sum. Each callback reports eight stereo source
contributions after the core's own authored pan masks plus the exact reference
mix sample. It is inert unless installed.

This intentionally does not expose individual operators as replacement sources:
the four-operator modulation network is part of one channel's timbral identity.
Exact singular replacements fail closed on upstream drift.
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
    header = root / "emu" / "cores" / "ym2151.h"
    source = root / "emu" / "cores" / "ym2151.c"

    h = header.read_text(encoding="utf-8")
    if "YM2151_SOURCE_TAP" not in h:
        h = replace_once(
            h,
            "extern DEV_DEF devDef_YM2151_MAME;\n",
            "extern DEV_DEF devDef_YM2151_MAME;\n\n"
            "// Optional exact source observer for the default MAME OPM core.\n"
            "// Each call corresponds to one native-rate sample after complete\n"
            "// channel synthesis and authored pan, immediately before summation.\n"
            "typedef void (*YM2151_SOURCE_TAP)(void* user, const DEV_SMPL left[8], const DEV_SMPL right[8], DEV_SMPL mix_left, DEV_SMPL mix_right);\n"
            "void ym2151_set_source_tap(void* chip, YM2151_SOURCE_TAP tap, void* user);\n",
            "YM2151 source-tap declaration",
        )

    c = source.read_text(encoding="utf-8")
    struct_old = (
        "\tUINT8       lastreg;\n"
        "\tvoid (*irqhandler)(void *param, UINT8 irq);\n"
        "\tvoid (*portwritehandler)(void *param, UINT8 ofs, UINT8 data);\n"
        "} YM2151;\n"
    )
    struct_new = (
        "\tUINT8       lastreg;\n"
        "\tvoid (*irqhandler)(void *param, UINT8 irq);\n"
        "\tvoid (*portwritehandler)(void *param, UINT8 ofs, UINT8 data);\n"
        "\tYM2151_SOURCE_TAP sourceTap;\n"
        "\tvoid* sourceTapUser;\n"
        "} YM2151;\n"
    )
    if "YM2151_SOURCE_TAP sourceTap" not in c:
        c = replace_once(c, struct_old, struct_new, "YM2151 tap state")

    setter_anchor = (
        "static UINT8 ym2151_r(void *chip, UINT8 offset)\n"
        "{\n"
        "\tif (offset & 1)\n"
        "\t\treturn ym2151_read_status(chip);\n"
        "\telse\n"
        "\t\treturn 0xff;    /* confirmed on a real YM2151 */\n"
        "}\n"
    )
    setter = setter_anchor + (
        "\nvoid ym2151_set_source_tap(void* chip, YM2151_SOURCE_TAP tap, void* user)\n"
        "{\n"
        "\tYM2151* PSG = (YM2151*)chip;\n"
        "\tif (PSG == NULL) return;\n"
        "\tPSG->sourceTap = tap;\n"
        "\tPSG->sourceTapUser = user;\n"
        "}\n"
    )
    if "void ym2151_set_source_tap" not in c:
        c = replace_once(c, setter_anchor, setter, "YM2151 tap setter")

    locals_old = (
        "\tUINT32 i;\n"
        "\tint ch;\n"
        "\tDEV_SMPL outl, outr;\n"
    )
    locals_new = (
        "\tUINT32 i;\n"
        "\tint ch;\n"
        "\tDEV_SMPL outl, outr;\n"
        "\tDEV_SMPL sourceL[8], sourceR[8];\n"
    )
    if "DEV_SMPL sourceL[8], sourceR[8]" not in c:
        c = replace_once(c, locals_old, locals_new, "YM2151 source locals")

    sum_old = (
        "\t\toutl = 0;\n"
        "\t\toutr = 0;\n"
        "\t\tfor(ch=0; ch<8; ch++) {\n"
        "\t\t\toutl += PSG->chanout[ch] & PSG->pan[2*ch];\n"
        "\t\t\toutr += PSG->chanout[ch] & PSG->pan[2*ch+1];\n"
        "\t\t}\n"
        "\t\tbuffers[0][i] = outl;\n"
        "\t\tbuffers[1][i] = outr;\n"
    )
    sum_new = (
        "\t\toutl = 0;\n"
        "\t\toutr = 0;\n"
        "\t\tfor(ch=0; ch<8; ch++) {\n"
        "\t\t\tsourceL[ch] = PSG->chanout[ch] & PSG->pan[2*ch];\n"
        "\t\t\tsourceR[ch] = PSG->chanout[ch] & PSG->pan[2*ch+1];\n"
        "\t\t\toutl += sourceL[ch];\n"
        "\t\t\toutr += sourceR[ch];\n"
        "\t\t}\n"
        "\t\tbuffers[0][i] = outl;\n"
        "\t\tbuffers[1][i] = outr;\n"
        "\t\tif (PSG->sourceTap != NULL)\n"
        "\t\t\tPSG->sourceTap(PSG->sourceTapUser, sourceL, sourceR, outl, outr);\n"
    )
    if "PSG->sourceTap(PSG->sourceTapUser, sourceL, sourceR, outl, outr)" not in c:
        c = replace_once(c, sum_old, sum_new, "YM2151 exact pre-sum capture")

    header.write_text(h, encoding="utf-8", newline="\n")
    source.write_text(c, encoding="utf-8", newline="\n")
    print("patched default MAME YM2151 exact source tap")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
