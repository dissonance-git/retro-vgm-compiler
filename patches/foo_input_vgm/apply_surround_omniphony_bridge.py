#!/usr/bin/env python3
"""Make the historical VGM Surround preference own the Genesis-only 7.1 bed.

The old libvgm surround effect is a channel-inversion trick. Disable that effect
and route the same persisted cfg_surround_sound switch only into exact primary
YM2612/SN76489 source spreading. Unsupported chips remain protected passthrough;
a VGM with no supported Genesis topology stays stereo even when Surround is on.
The generated runtime remains fail-closed when exact source delivery fails.
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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    root = parser.parse_args().source_dir.resolve()
    input_base = root / "input_base.cpp"
    shadow = root / "input_vgm_shadow.cpp"

    replace_once(
        input_base,
        "\tpa_cfg.chnInvert = cfg_surround_sound ? 0x02 : 0x00;\n",
        "\t// The persisted Surround preference now selects the source-native 7.1 bed.\n"
        "\t// Do not also run libvgm's historical inversion effect.\n"
        "\tpa_cfg.chnInvert = 0x00;\n",
        "disable legacy VGM surround inversion",
    )

    replace_once(
        shadow,
        """\tif (!cfg_vgm_sem71_enabled || !m_genesis_surround_eligible
\t\t|| !sources_ready || !episodes_ready || frame_count == 0
\t\t|| frame_count > 8192u || chunk.get_channels() != 2
\t\t|| chunk.get_srate() != m_sample_rate)
""",
        """\tif (!cfg_surround_sound || !m_genesis_surround_eligible
\t\t|| !sources_ready || !episodes_ready || frame_count == 0
\t\t|| frame_count > 8192u || chunk.get_channels() != 2
\t\t|| chunk.get_srate() != m_sample_rate)
""",
        "route existing VGM Surround preference to 7.1 bed",
    )

    print("foo_input_vgm Surround -> source-native 7.1 bridge applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
