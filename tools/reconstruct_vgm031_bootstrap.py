from __future__ import annotations

import base64
import hashlib
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / ".delivery-safe"
TARGET = ROOT / "imports" / "foo_input_vgm-0.31.zip"

PARTS = (
    "pre00",
    "pre01",
    "pre02",
    "chunk00",
    "chunk01",
    "chunk02",
    "chunk03",
    "chunk04",
    "chunk05a",
    "chunk05b",
    "chunk06",
    "chunk07",
    "chunk08",
    "chunk09",
    "chunk10",
)

EXPECTED_BASE64_LENGTH = 88_336
EXPECTED_BASE64_SHA256 = "e0774db2137a56d3c592b9238be3727ed7521b1da4673f1f2f109c9e2d78b5b1"
EXPECTED_ARCHIVE_LENGTH = 66_250
EXPECTED_ARCHIVE_SHA256 = "e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1"
EXPECTED_ARCHIVE_ENTRIES = 43
EXPECTED_SOURCE_FILES = 41


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    pieces: list[str] = []
    offset = 0
    for name in PARTS:
        path = SOURCE_DIR / name
        if not path.is_file():
            raise RuntimeError(f"missing transfer part: {path.relative_to(ROOT)}")
        text = "".join(path.read_text(encoding="ascii").split())
        digest = sha256(text.encode("ascii"))
        print(f"{name}: offset={offset} length={len(text)} sha256={digest}")
        pieces.append(text)
        offset += len(text)

    encoded_text = "".join(pieces)
    encoded = encoded_text.encode("ascii")
    encoded_sha = sha256(encoded)
    print(f"base64: length={len(encoded)} sha256={encoded_sha}")
    if len(encoded) != EXPECTED_BASE64_LENGTH:
        raise RuntimeError(
            f"base64 length mismatch: expected {EXPECTED_BASE64_LENGTH}, got {len(encoded)}"
        )
    if encoded_sha != EXPECTED_BASE64_SHA256:
        raise RuntimeError(
            f"base64 SHA-256 mismatch: expected {EXPECTED_BASE64_SHA256}, got {encoded_sha}"
        )

    try:
        archive = base64.b64decode(encoded, validate=True)
    except Exception as exc:
        raise RuntimeError(f"base64 decode failed: {exc}") from exc

    archive_sha = sha256(archive)
    print(f"archive: length={len(archive)} sha256={archive_sha}")
    if len(archive) != EXPECTED_ARCHIVE_LENGTH:
        raise RuntimeError(
            f"archive length mismatch: expected {EXPECTED_ARCHIVE_LENGTH}, got {len(archive)}"
        )
    if archive_sha != EXPECTED_ARCHIVE_SHA256:
        raise RuntimeError(
            f"archive SHA-256 mismatch: expected {EXPECTED_ARCHIVE_SHA256}, got {archive_sha}"
        )

    TARGET.write_bytes(archive)
    with zipfile.ZipFile(TARGET) as zf:
        bad = zf.testzip()
        if bad is not None:
            raise RuntimeError(f"ZIP CRC failure: {bad}")
        entries = zf.infolist()
        source_files = sum(not item.is_dir() for item in entries)
        print(f"zip: entries={len(entries)} files={source_files}")
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

    print(f"verified exact foo_input_vgm 0.31 bootstrap: {TARGET.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
