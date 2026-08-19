#!/usr/bin/env python3
"""Bridge established decode ordering across the Genesis spatial runtime patch.

Two older transformations legitimately occupy text that the final spatial patch
historically used as anchors:

* the source-bank DAC observer advances PCM state at each command boundary;
* the deferred FM path wraps the delivered decode tail so render-ahead state is
  kept on the engine clock while ordinary delivery still performs QSound replay.

During materialization this bridge temporarily exposes the older spatial patch's
expected anchors. After that patch succeeds, both established behaviors are
restored around the new spatial operations.

Final command-event order:

    resolve absolute sample
    observe authored Genesis route event
    advance source-bank PCM interval to that sample
    enter the block-local source-capture branch

Final delivered-block order:

    deferred engine-clock maintenance OR ordinary QSound projection
    Genesis Omniphony attempt over finalized delivered sources
    ordinary QSound replay only when not in deferred render-ahead
    return protected/final chunk

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

    replace_once(
        shadow,
        """\tif (m_studio_deferred_capture_bypass)
\t{
\t\t// PlayerA may have rendered beyond m_render_done to satisfy the FIR. The
\t\t// command tap was in direct-shadow mode, so advance the continuous state
\t\t// to the actual engine clock rather than the delivered foobar clock.
\t\tif (m_vgm_player != nullptr)
\t\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
\t}
\telse
\t{
\t\tproject_qsound_consumer_sources(block_start, m_render_done);
\t\treplay_captured_sources(m_render_done);
\t}
\treturn true;
""",
        """\tproject_qsound_consumer_sources(block_start, m_render_done);
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        "prepare Genesis spatial delivered-block anchor",
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

    replace_once(
        shadow,
        """\tproject_qsound_consumer_sources(block_start, m_render_done);
\tconst std::uint64_t genesis_block_start = m_genesis_delivered_ordinal;
\tif (m_render_done != 0)
\t{
\t\trender_genesis_spatial_output(
\t\t\tp_chunk,
\t\t\tgenesis_block_start,
\t\t\tstatic_cast<std::size_t>(m_render_done));
\t\tm_genesis_delivered_ordinal += static_cast<std::uint64_t>(m_render_done);
\t}
\treplay_captured_sources(m_render_done);
\treturn true;
""",
        """\tif (m_studio_deferred_capture_bypass)
\t{
\t\t// PlayerA may have rendered beyond m_render_done to satisfy the FIR. The
\t\t// command tap was in direct-shadow mode, so advance the continuous state
\t\t// to the actual engine clock rather than the delivered foobar clock.
\t\tif (m_vgm_player != nullptr)
\t\t\tadvance_shadow_to(static_cast<uint_fast64_t>(m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)));
\t}
\telse
\t{
\t\tproject_qsound_consumer_sources(block_start, m_render_done);
\t}

\tconst std::uint64_t genesis_block_start = m_genesis_delivered_ordinal;
\tif (m_render_done != 0)
\t{
\t\trender_genesis_spatial_output(
\t\t\tp_chunk,
\t\t\tgenesis_block_start,
\t\t\tstatic_cast<std::size_t>(m_render_done));
\t\tm_genesis_delivered_ordinal += static_cast<std::uint64_t>(m_render_done);
\t}

\tif (!m_studio_deferred_capture_bypass)
\t\treplay_captured_sources(m_render_done);
\treturn true;
""",
        "restore deferred delivery around Genesis spatial output",
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
