from __future__ import annotations

import base64
import hashlib
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "imports" / "bootstrap" / "foo_input_vgm-0.31.base64-parts"
TARGET = ROOT / "imports" / "foo_input_vgm-0.31.zip"

PARTS = (
    "pre00", "pre01", "pre02",
    "chunk00", "chunk01", "chunk02", "chunk03", "chunk04",
    "chunk05a", "chunk05b", "chunk06", "chunk07", "chunk08", "chunk09", "chunk10",
)

EXPECTED_BASE64_LENGTH = 88_336
EXPECTED_BASE64_SHA256 = "e0774db2137a56d3c592b9238be3727ed7521b1da4673f1f2f109c9e2d78b5b1"
EXPECTED_ARCHIVE_LENGTH = 66_250
EXPECTED_ARCHIVE_SHA256 = "e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1"
EXPECTED_ARCHIVE_ENTRIES = 43
EXPECTED_SOURCE_FILES = 41


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_transport() -> bytes:
    encoded_text = "".join(
        "".join((SOURCE_DIR / name).read_text(encoding="ascii").split())
        for name in PARTS
    )
    encoded = encoded_text.encode("ascii")
    if len(encoded) != EXPECTED_BASE64_LENGTH:
        raise RuntimeError(
            f"base64 length mismatch: expected {EXPECTED_BASE64_LENGTH}, got {len(encoded)}"
        )
    if sha256(encoded) != EXPECTED_BASE64_SHA256:
        raise RuntimeError("foo_input_vgm 0.31 base64 transport SHA-256 mismatch")
    try:
        archive = base64.b64decode(encoded, validate=True)
    except Exception as exc:
        raise RuntimeError(f"base64 decode failed: {exc}") from exc
    if len(archive) != EXPECTED_ARCHIVE_LENGTH:
        raise RuntimeError(
            f"archive length mismatch: expected {EXPECTED_ARCHIVE_LENGTH}, got {len(archive)}"
        )
    if sha256(archive) != EXPECTED_ARCHIVE_SHA256:
        raise RuntimeError("foo_input_vgm 0.31 archive SHA-256 mismatch")
    return archive


def validate_archive(path: Path) -> None:
    with zipfile.ZipFile(path) as zf:
        bad = zf.testzip()
        if bad is not None:
            raise RuntimeError(f"ZIP CRC failure: {bad}")
        entries = zf.infolist()
        source_files = sum(not item.is_dir() for item in entries)
        if len(entries) != EXPECTED_ARCHIVE_ENTRIES:
            raise RuntimeError(
                f"ZIP entry-count mismatch: expected {EXPECTED_ARCHIVE_ENTRIES}, got {len(entries)}"
            )
        if source_files != EXPECTED_SOURCE_FILES:
            raise RuntimeError(
                f"source file-count mismatch: expected {EXPECTED_SOURCE_FILES}, got {source_files}"
            )
        marker = zf.read("foo_input_vgm/src/my_component_client.cpp").decode("utf-8-sig")
        if '"0.31"' not in marker:
            raise RuntimeError("foo_input_vgm component version marker is not 0.31")


def main() -> int:
    archive = load_transport()
    TARGET.parent.mkdir(parents=True, exist_ok=True)
    TARGET.write_bytes(archive)
    validate_archive(TARGET)
    print(
        f"verified foo_input_vgm 0.31 bootstrap: {TARGET.relative_to(ROOT)} "
        f"sha256={EXPECTED_ARCHIVE_SHA256}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
