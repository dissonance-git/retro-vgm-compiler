#!/usr/bin/env python3
"""Bridge the DAC source-clock seam across the Genesis spatial runtime patch.

The source-bank DAC observer predates the final spatial route patch and inserts
its PCM interval advance between the two lines used by that patch's historical
route-observation anchor. Both operations are required. During materialization
this bridge temporarily exposes the old structural anchor, then restores the PCM
advance immediately after the newly inserted route event.

Final order:

    resolve absolute sample
    observe authored Genesis route event
    advance source-bank PCM interval to that sample
    enter the block-local source-capture branch

No intermediate source is compiled or executed between prepare and restore.
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


def prepare(shadow: Path) -> None:
    replace_once(
        shadow,
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\t// genesis_state calls this tap before mutating YM controls, so the preceding
\t// interval sees the old DAC-enable/pan state and the next interval sees the
\t// new state at this exact command ordinal.
\t(void)self->advance_pcm_streams_to(absolute_sample);

\tif (self->m_source_capture_active)
""",
        """\tconst uint_fast64_t absolute_sample =
\t\tstatic_cast<uint_fast64_t>(self->m_vgm_player->Tick2Sample(static_cast<UINT32>(event.tick)));

\tif (self->m_source_capture_active)
""",
        "prepare Genesis spatial route observation anchor",
    )


def restore(shadow: Path) -> None:
    replace_once(
        shadow,
        """\tself->m_genesis_spatial_routes.observe(
\t\tevent,
\t\tstatic_cast<std::uint64_t>(absolute_sample));

\tif (self->m_source_capture_active)
""",
        """\tself->m_genesis_spatial_routes.observe(
\t\tevent,
\t\tstatic_cast<std::uint64_t>(absolute_sample));

\t// genesis_state calls this tap before mutating YM controls, so the preceding
\t// interval sees the old DAC-enable/pan state and the next interval sees the
\t// new state at this exact command ordinal.
\t(void)self->advance_pcm_streams_to(absolute_sample);

\tif (self->m_source_capture_active)
""",
        "restore PCM advance after Genesis spatial route observation",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    parser.add_argument("phase", choices=("prepare", "restore"))
    args = parser.parse_args()
    shadow = args.source_dir.resolve() / "input_vgm_shadow.cpp"
    if args.phase == "prepare":
        prepare(shadow)
    else:
        restore(shadow)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
