#!/usr/bin/env python3
"""Add an optional exact pre-sum source tap to libvgm's MAME SN76496 core.

The callback is inert unless installed. It reports four stereo channel
contributions after the core's own frequency limiting, mute and Game Gear
stereo logic, after the same negate/>>1 output scaling used by the reference
mix, plus that exact reference mix sample for reconstruction validation.

Pass the libvgm checkout root. Exact singular replacements fail closed on
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
    header = root / "emu" / "cores" / "sn76496.h"
    source = root / "emu" / "cores" / "sn76496.c"

    h = header.read_text(encoding="utf-8")
    if "SN76496_SOURCE_TAP" not in h:
        h = replace_once(
            h,
            "extern DEV_DEF devDef_SN76496_MAME;\n",
            "extern DEV_DEF devDef_SN76496_MAME;\n\n"
            "// Optional source observer for the default MAME core. Each call\n"
            "// corresponds to exactly one native-rate sample emitted by Update.\n"
            "typedef void (*SN76496_SOURCE_TAP)(void* user, const DEV_SMPL left[4], const DEV_SMPL right[4], DEV_SMPL mix_left, DEV_SMPL mix_right);\n"
            "void sn76496_set_source_tap(void* chip, SN76496_SOURCE_TAP tap, void* user);\n",
            "SN76496 source-tap declaration",
        )

    c = source.read_text(encoding="utf-8")
    struct_old = (
        "\tUINT32 MuteMsk[4];\n"
        "\tUINT8 NgpFlags;         // bit 7 - NGP Mode on/off, bit 0 - is 2nd NGP chip\n"
        "\tsn76496_state* NgpChip2;    // pointer to other chip instance of T6W28\n"
        "};\n"
    )
    struct_new = (
        "\tUINT32 MuteMsk[4];\n"
        "\tUINT8 NgpFlags;         // bit 7 - NGP Mode on/off, bit 0 - is 2nd NGP chip\n"
        "\tsn76496_state* NgpChip2;    // pointer to other chip instance of T6W28\n"
        "\tSN76496_SOURCE_TAP sourceTap;\n"
        "\tvoid* sourceTapUser;\n"
        "};\n"
    )
    if "SN76496_SOURCE_TAP sourceTap" not in c:
        c = replace_once(c, struct_old, struct_new, "SN76496 tap state")

    setter_anchor = (
        "static UINT8 sn76496_ready_r(void *chip, UINT8 offset)\n"
        "{\n"
        "\tsn76496_state *R = (sn76496_state*)chip;\n"
        "\treturn R->ready_state ? 1 : 0;\n"
        "}\n"
    )
    setter = setter_anchor + (
        "\nvoid sn76496_set_source_tap(void* chip, SN76496_SOURCE_TAP tap, void* user)\n"
        "{\n"
        "\tsn76496_state* R = (sn76496_state*)chip;\n"
        "\tif (R == NULL) return;\n"
        "\tR->sourceTap = tap;\n"
        "\tR->sourceTapUser = user;\n"
        "}\n"
    )
    if "void sn76496_set_source_tap" not in c:
        c = replace_once(c, setter_anchor, setter, "SN76496 tap setter")

    locals_old = (
        "\tDEV_SMPL out = 0;\n"
        "\tDEV_SMPL out2 = 0;\n"
        "\tINT32 vol[4];\n"
        "\tINT32 ggst[2];\n"
    )
    locals_new = (
        "\tDEV_SMPL out = 0;\n"
        "\tDEV_SMPL out2 = 0;\n"
        "\tDEV_SMPL sourceL[4];\n"
        "\tDEV_SMPL sourceR[4];\n"
        "\tINT32 vol[4];\n"
        "\tINT32 ggst[2];\n"
    )
    if "DEV_SMPL sourceL[4]" not in c:
        c = replace_once(c, locals_old, locals_new, "SN76496 source locals")

    silent_old = (
        "\t\tif (! out)\n"
        "\t\t{\n"
        "\t\t\tmemset(lbuffer, 0x00, sizeof(DEV_SMPL) * samples);\n"
        "\t\t\tmemset(rbuffer, 0x00, sizeof(DEV_SMPL) * samples);\n"
        "\t\t\treturn;\n"
        "\t\t}\n"
    )
    silent_new = (
        "\t\tif (! out)\n"
        "\t\t{\n"
        "\t\t\tmemset(lbuffer, 0x00, sizeof(DEV_SMPL) * samples);\n"
        "\t\t\tmemset(rbuffer, 0x00, sizeof(DEV_SMPL) * samples);\n"
        "\t\t\tif (R->sourceTap != NULL)\n"
        "\t\t\t{\n"
        "\t\t\t\tDEV_SMPL zero[4] = {0, 0, 0, 0};\n"
        "\t\t\t\tfor (j = 0; j < samples; ++j)\n"
        "\t\t\t\t\tR->sourceTap(R->sourceTapUser, zero, zero, 0, 0);\n"
        "\t\t\t}\n"
        "\t\t\treturn;\n"
        "\t\t}\n"
    )
    if "R->sourceTap(R->sourceTapUser, zero, zero, 0, 0)" not in c:
        c = replace_once(c, silent_old, silent_new, "silent source callbacks")

    sample_begin_old = (
        "\tfor (j = 0; j < samples; j++)\n"
        "\t{\n"
        "\t\t// disabled, because dividing the output sample rate is easier and faster\n"
    )
    sample_begin_new = (
        "\tfor (j = 0; j < samples; j++)\n"
        "\t{\n"
        "\t\tmemset(sourceL, 0x00, sizeof(sourceL));\n"
        "\t\tmemset(sourceR, 0x00, sizeof(sourceR));\n"
        "\t\t// disabled, because dividing the output sample rate is easier and faster\n"
    )
    if "memset(sourceL, 0x00" not in c:
        c = replace_once(c, sample_begin_old, sample_begin_new, "per-sample source clear")

    normal_sum_old = (
        "\t\t\t\tif (R->period[i] > 1 || i == 3)\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\tout += vol[i] * R->volume[i] * ggst[0];\n"
        "\t\t\t\t\tout2 += vol[i] * R->volume[i] * ggst[1];\n"
        "\t\t\t\t}\n"
        "\t\t\t\telse if (R->MuteMsk[i])\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\t// Make Bipolar Output with PCM possible\n"
        "\t\t\t\t\tout += R->volume[i] * ggst[0];\n"
        "\t\t\t\t\tout2 += R->volume[i] * ggst[1];\n"
        "\t\t\t\t}\n"
    )
    normal_sum_new = (
        "\t\t\t\tif (R->period[i] > 1 || i == 3)\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\tsourceL[i] = vol[i] * R->volume[i] * ggst[0];\n"
        "\t\t\t\t\tsourceR[i] = vol[i] * R->volume[i] * ggst[1];\n"
        "\t\t\t\t}\n"
        "\t\t\t\telse if (R->MuteMsk[i])\n"
        "\t\t\t\t{\n"
        "\t\t\t\t\t// Make Bipolar Output with PCM possible\n"
        "\t\t\t\t\tsourceL[i] = R->volume[i] * ggst[0];\n"
        "\t\t\t\t\tsourceR[i] = R->volume[i] * ggst[1];\n"
        "\t\t\t\t}\n"
        "\t\t\t\tout += sourceL[i];\n"
        "\t\t\t\tout2 += sourceR[i];\n"
    )
    if "sourceL[i] = vol[i] * R->volume[i]" not in c:
        c = replace_once(c, normal_sum_old, normal_sum_new, "normal PSG pre-sum capture")

    output_old = (
        "\t\tif(R->negate) { out = -out; out2 = -out2; }\n\n"
        "\t\tlbuffer[j] = out >> 1;\t// >>1 to make up for bipolar output\n"
        "\t\trbuffer[j] = out2 >> 1;\n"
    )
    output_new = (
        "\t\tif(R->negate)\n"
        "\t\t{\n"
        "\t\t\tout = -out; out2 = -out2;\n"
        "\t\t\tif (! R->NgpFlags)\n"
        "\t\t\t\tfor (i = 0; i < 4; ++i) { sourceL[i] = -sourceL[i]; sourceR[i] = -sourceR[i]; }\n"
        "\t\t}\n\n"
        "\t\tlbuffer[j] = out >> 1;\t// >>1 to make up for bipolar output\n"
        "\t\trbuffer[j] = out2 >> 1;\n"
        "\t\tif (R->sourceTap != NULL && ! R->NgpFlags)\n"
        "\t\t{\n"
        "\t\t\tfor (i = 0; i < 4; ++i) { sourceL[i] >>= 1; sourceR[i] >>= 1; }\n"
        "\t\t\tR->sourceTap(R->sourceTapUser, sourceL, sourceR, lbuffer[j], rbuffer[j]);\n"
        "\t\t}\n"
    )
    if "R->sourceTap(R->sourceTapUser, sourceL, sourceR, lbuffer[j], rbuffer[j])" not in c:
        c = replace_once(c, output_old, output_new, "PSG source callback")

    header.write_text(h, encoding="utf-8", newline="\n")
    source.write_text(c, encoding="utf-8", newline="\n")
    print("patched default MAME SN76496 source tap")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
