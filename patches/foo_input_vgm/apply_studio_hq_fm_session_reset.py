#!/usr/bin/env python3
"""Reset deferred Studio transport at every decode session boundary.

input_base::decode_initialize starts PlayerA. A reused input_vgm instance must
therefore unregister any prior deferred callback and discard retained future
frames before that Start, not after the first decode_run of the new session.
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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    root = args.source_dir.resolve()
    header = root / "input_vgm.h"
    shadow = root / "input_vgm_shadow.cpp"

    replace_once(
        header,
        """\tstatic bool g_is_our_path(const char *p_path, const char *p_extension);
\tbool decode_run(audio_chunk &p_chunk, abort_callback &p_abort) override;
""",
        """\tstatic bool g_is_our_path(const char *p_path, const char *p_extension);
\tvoid decode_initialize(unsigned int p_flags, abort_callback &p_abort);
\tbool decode_run(audio_chunk &p_chunk, abort_callback &p_abort) override;
""",
        "Studio decode-session override declaration",
    )

    implementation = r'''void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)
{
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI)
	// This must precede input_base::decode_initialize(), which calls PlayerA::Start.
	// A retained frame from the previous session has no valid ordinal in the new
	// source timeline even if the same input object is reused.
	m_main_player.SetDeferredPostRenderProcessor(nullptr, nullptr);
	m_studio_fm_transport.reset();
	m_studio_deferred_engaged = false;
	m_studio_deferred_active = false;
	m_studio_deferred_failed = false;
	m_studio_deferred_capture_bypass = false;
#endif
	input_base::decode_initialize(p_flags, p_abort);
}

'''
    replace_once(
        shadow,
        """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
""",
        implementation + """bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
{
""",
        "Studio decode-session reset implementation",
    )

    print("foo_input_vgm Studio HQ FM session reset applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
