#!/usr/bin/env python3
"""Wire the private SNESAPU 48 kHz host contract and enhanced synthesis path.

All private x64 playback uses one 48 kHz final source/DSP/output timeline,
regardless of source-quality or Spatial settings. The audited parent still keeps
its stored sample-rate preference, but this private build does not expose that
preference to the runtime clock.

Enhanced remains independent: only while enhanced is active do we select
SNESAPU's sinc source interpolator (and any stronger verified source-restoration
rung layered above it). Spatial remains presentation-only.
"""

from __future__ import annotations

import argparse
from pathlib import Path

PRIVATE_PLAYBACK_RATE = 48000


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
\t// Private playback has one final host clock in every combination. Source
\t// quality and Spatial remain independent decisions above/below this clock.
\tm_CnfSampleRate = {PRIVATE_PLAYBACK_RATE};
\tif (cfg_enhanced_enabled)
\t{{
\t\t// Enhanced changes source realization, not the host-rate contract.
\t\tm_CnfInterpolation = INT_SINC;
\t}}
#endif
""",
        "SNES private 48 kHz host and enhanced source policy",
    )

    print("SNESAPU private 48 kHz runtime applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
