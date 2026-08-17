#!/usr/bin/env python3
"""Reset source-bank DAC transport before every reused decode session."""

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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    shadow = parser.parse_args().source_dir.resolve() / "input_vgm_shadow.cpp"

    replace_once(
        shadow,
        """#endif
\tinput_base::decode_initialize(p_flags, p_abort);
}

bool input_vgm::decode_run""",
        """#endif
\t// Source-bank transport has its own authoritative ordinal state. Reset it
\t// before PlayerA::Start just like the deferred frame transport above.
\treset_pcm_streams();
\tinput_base::decode_initialize(p_flags, p_abort);
}

bool input_vgm::decode_run""",
        "source-bank DAC decode-session reset",
    )

    print("foo_input_vgm source-bank DAC session reset applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
