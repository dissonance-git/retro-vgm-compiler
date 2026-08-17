#!/usr/bin/env python3
"""Wire the first audible SNESAPU Enhanced synthesis path.

The pinned SPCPlay/SNESAPU renderer already has an unusually useful property:
when DSP_ECHOFIR is clear, SetDSPOpt runs the DSP itself at the requested output
rate, adjusts source pitch for that DSP rate, and dispatches the selected voice
interpolator at that rate. INT_SINC is its 8-point sinc interpolator.

So for a user-selected 96 kHz output rate, Enhanced can immediately use a true
source-domain 96 kHz reconstruction path instead of rendering the historical
32 kHz DSP output and merely resampling that final bus.

This patch does not alter the stored quality controls. It treats the configured
sample-rate field as the Enhanced target rate, forces only the interpolation
stage to INT_SINC while Enhanced is checked, and explicitly clears the hidden
DSP_ECHOFIR compatibility mode which would otherwise clamp the DSP to 32 kHz and
invoke the final sampling-rate converter. Spatial/Omniphony remains independent.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    has_utf8_bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", has_utf8_bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, has_utf8_bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if has_utf8_bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source_dir",
        type=Path,
        help="foo_snesapu/foobar2000/foo_snesapu source directory",
    )
    args = parser.parse_args()
    root = args.source_dir.resolve()
    input_cpp = root / "input_snesapu.cpp"

    replace_once(
        input_cpp,
        """\tm_CnfInterpolation = cfg_interpolation;
\tm_CnfStereo = cfg_stereo;
\tm_CnfOptions = cfg_dsp_option;
""",
        """\tm_CnfInterpolation = cfg_interpolation;
\tm_CnfStereo = cfg_stereo;
\tm_CnfOptions = cfg_dsp_option;
#ifdef _WIN64
\tif (cfg_enhanced_enabled)
\t{
\t\t// Enhanced changes the reconstruction ceiling, not the stored reference
\t\t// preference. INT_SINC is SNESAPU's 8-point source interpolator.
\t\tm_CnfInterpolation = INT_SINC;
\t\t// DSP_ECHOFIR is the compatibility mode that clamps DSP execution to
\t\t// 32 kHz and then resamples the finished bus. Enhanced explicitly uses
\t\t// the configured output rate as the DSP/source-reconstruction rate.
\t\tm_CnfOptions &= ~DSP_ECHOFIR;
\t}
#endif
""",
        "SNES Enhanced source-domain interpolation policy",
    )

    print("SNESAPU source-domain Enhanced runtime applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
