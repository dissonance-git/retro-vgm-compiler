#!/usr/bin/env python3
"""Wire the first audible SNESAPU Enhanced synthesis path.

The pinned SPCPlay/SNESAPU renderer already has an unusually useful property:
when DSP_ECHOFIR is clear, SetDSPOpt runs the DSP itself at the requested output
rate, adjusts source pitch for that DSP rate, and dispatches the selected voice
interpolator at that rate. INT_SINC is its 8-point sinc interpolator.

Enhanced playback is intentionally standardized at 48 kHz. That is the final
playback rate, so running the source/DSP reconstruction at 48 kHz avoids spending
roughly twice the realtime work on a 96 kHz intermediate that would immediately
be discarded. The verified-upstream studio rung still uses its longer 64-tap
source-domain reconstruction; this change only sets the host/DSP presentation
rate used by normal Enhanced playback.

The stored reference quality controls are left untouched. While Enhanced is
checked, the runtime forces 48 kHz plus INT_SINC and explicitly clears the hidden
DSP_ECHOFIR compatibility mode which would otherwise clamp DSP execution to
32 kHz and invoke the final sampling-rate converter. Spatial/Omniphony remains
independent. 96 kHz remains useful as an offline/research comparison, not the
normal playback contract.
"""

from __future__ import annotations

import argparse
from pathlib import Path


ENHANCED_PLAYBACK_RATE = 48000


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
        f"""\tm_CnfInterpolation = cfg_interpolation;
\tm_CnfStereo = cfg_stereo;
\tm_CnfOptions = cfg_dsp_option;
#ifdef _WIN64
\tif (cfg_enhanced_enabled)
\t{{
\t\t// Normal Enhanced playback is deliberately one-rate end to end. The
\t\t// protected reference preference remains stored but is not used while
\t\t// Enhanced is active.
\t\tm_CnfSampleRate = {ENHANCED_PLAYBACK_RATE};
\t\t// INT_SINC is SNESAPU's 8-point source interpolator. Verified upstream
\t\t// sources can replace this waveform stage with the 64-tap studio sampler.
\t\tm_CnfInterpolation = INT_SINC;
\t\t// DSP_ECHOFIR is the compatibility mode that clamps DSP execution to
\t\t// 32 kHz and then resamples the finished bus. Enhanced instead executes
\t\t// the DSP/source reconstruction directly at the 48 kHz playback rate.
\t\tm_CnfOptions &= ~DSP_ECHOFIR;
\t}}
#endif
""",
        "SNES Enhanced 48 kHz source-domain policy",
    )

    print("SNESAPU 48 kHz source-domain Enhanced runtime applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
