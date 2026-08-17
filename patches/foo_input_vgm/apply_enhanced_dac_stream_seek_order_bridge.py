#!/usr/bin/env python3
"""Keep source-bank DAC seek reset compatible with deferred PSG patch ordering.

The source-bank observer is intentionally installed before the deferred patches,
so it can bind to the stable base seek function. The deferred FM patch later
inserts its own guarded reset block immediately after configure_enhancement_shadow.
That places reset_pcm_streams() between the deferred guard and the historical
m_source_capture_active line, which would hide the stable anchor used by the
following deferred PSG patch. Move only the PCM reset farther down the same seek
function, still before input_base::decode_seek(), so both lifecycle contracts
remain true without depending on incidental adjacency.
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
    shadow = parser.parse_args().source_dir.resolve() / "input_vgm_shadow.cpp"

    replace_once(
        shadow,
        """\tm_studio_deferred_capture_bypass = false;
#endif
\treset_pcm_streams();
\tm_source_capture_active = false;
""",
        """\tm_studio_deferred_capture_bypass = false;
#endif
\tm_source_capture_active = false;
""",
        "source-bank DAC seek reset adjacency",
    )

    replace_once(
        shadow,
        """\tinput_base::decode_seek(p_seconds, p_abort);
""",
        """\treset_pcm_streams();
\tinput_base::decode_seek(p_seconds, p_abort);
""",
        "source-bank DAC seek reset relocation",
    )

    print("foo_input_vgm source-bank DAC seek-order bridge applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
