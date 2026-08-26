#!/usr/bin/env python3
"""Route SNESAPU's historical DSP_SURND preference into the 7.1 source bed.

Runs after the private source-native 7.1 runtime is installed. The saved DSP_SURND bit
continues to live in cfg_dsp_option, but it is masked out of the SNESAPU DSP
options so the old surround algorithm never runs. The same bit gates causal SRCE
capture and 7.1 presentation. Failure still leaves protected stereo intact.
"""

from __future__ import annotations

import argparse
from pathlib import Path


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
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    source = root / "foobar2000" / "foo_snesapu" / "input_snesapu.cpp"

    replace_once(
        source,
        """\tm_Apu.SetAPUOpt(m_CnfMixing, m_CnfChannels, m_CnfBPS, m_CnfSampleRate, m_CnfInterpolation, m_CnfOptions);
""",
        """\t// The persisted DSP_SURND bit now selects the source-native 7.1 bed below. Mask it
\t// from SNESAPU itself so the historical surround DSP is never double-applied.
\tconst int snesapu_runtime_options = m_CnfOptions & ~DSP_SURND;
\tm_Apu.SetAPUOpt(m_CnfMixing, m_CnfChannels, m_CnfBPS, m_CnfSampleRate, m_CnfInterpolation, snesapu_runtime_options);
""",
        "disable legacy SNESAPU surround processing",
    )

    replace_once(
        source,
        """\tm_Sem71Enabled = cfg_sem71_enabled;
\tm_Apu.SetSourceEnabled(m_Sem71Enabled);
""",
        """\tm_Sem71Enabled = (m_CnfOptions & DSP_SURND) != 0;
\tm_Apu.SetSourceEnabled(m_Sem71Enabled);
""",
        "route historical SNESAPU Surround bit to 7.1 bed",
    )

    print("SNESAPU Surround -> source-native 7.1 bridge applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
