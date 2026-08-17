#!/usr/bin/env python3
"""Wire the first audible SNESAPU enhanced synthesis path.

The audited parent stores the user's reference sample-rate and interpolation
preferences. While enhanced is active the playback contract instead uses one
48 kHz source/DSP/output rate plus SNESAPU's sinc interpolator. Verified
upstream sources may still replace that BRR reconstruction at the stronger
source-restoration rung. Spatial remains independent.
"""

from __future__ import annotations

import argparse
from pathlib import Path

ENHANCED_PLAYBACK_RATE = 48000


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    root = parser.parse_args().source_dir.resolve()
    input_cpp = root / "input_snesapu.cpp"

    replace_once(
        input_cpp,
        """\tm_CnfSampleRate\t\t         = cfg_samplerate;
\tm_CnfBPS\t\t\t         = cfg_bitspersample;
\tm_CnfChannels\t\t         = cfg_channels;
\tm_CnfMixing\t\t\t         = 3;
\tm_CnfInterpolation\t         = cfg_interpolation;
\tm_CnfOptions\t\t         = cfg_dsp_option;
""",
        f"""\tm_CnfSampleRate\t\t         = cfg_samplerate;
\tm_CnfBPS\t\t\t         = cfg_bitspersample;
\tm_CnfChannels\t\t         = cfg_channels;
\tm_CnfMixing\t\t\t         = 3;
\tm_CnfInterpolation\t         = cfg_interpolation;
\tm_CnfOptions\t\t         = cfg_dsp_option;
#ifdef _WIN64
\tif (cfg_enhanced_enabled)
\t{{
\t\t// Preserve the stored reference preferences but use one source/DSP/output
\t\t// rate while enhanced playback is active.
\t\tm_CnfSampleRate = {ENHANCED_PLAYBACK_RATE};
\t\tm_CnfInterpolation = INT_SINC;
\t}}
#endif
""",
        "SNES enhanced 48 kHz source-domain policy",
    )

    print("SNESAPU 48 kHz source-domain enhanced runtime applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
