#!/usr/bin/env python3
"""Import immutable user-supplied VGM Tooling corpus ZIPs.

Preserves the original archive, safely extracts runnable files, and records
SHA-256/size metadata in a deterministic JSON manifest. The script never
rewrites, retags, recompresses, or normalizes media payloads.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

ALLOWED_SUFFIXES = {".vgm", ".vgz", ".spc"}
MANIFEST_VERSION = 1


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_member_path(name: str) -> PurePosixPath:
    candidate = PurePosixPath(name.replace("\\", "/"))
    if candidate.is_absolute() or ".." in candidate.parts:
        raise ValueError(f"unsafe ZIP member path: {name!r}")
    if not candidate.parts:
        raise ValueError("empty ZIP member path")
    return candidate


def member_is_symlink(info: zipfile.ZipInfo) -> bool:
    mode = (info.external_attr >> 16) & 0xFFFF
    return stat.S_ISLNK(mode)


def extract_archive(archive: Path, repo_root: Path, corpus_id: str) -> dict:
    corpus_root = repo_root / "tests" / "corpus"
    archive_dir = corpus_root / "archives"
    extracted_dir = corpus_root / corpus_id
    archive_dir.mkdir(parents=True, exist_ok=True)
    extracted_dir.mkdir(parents=True, exist_ok=True)

    preserved_archive = archive_dir / archive.name
    if preserved_archive.exists():
        if sha256_file(preserved_archive) != sha256_file(archive):
            raise ValueError(f"archive already exists with different bytes: {preserved_archive}")
    else:
        shutil.copyfile(archive, preserved_archive)

    extracted: list[dict] = []
    with zipfile.ZipFile(archive, "r") as zf:
        for info in zf.infolist():
            if info.is_dir():
                continue
            member = safe_member_path(info.filename)
            if member_is_symlink(info):
                raise ValueError(f"refusing symlink ZIP member: {info.filename!r}")
            if member.suffix.lower() not in ALLOWED_SUFFIXES:
                continue

            target = extracted_dir.joinpath(*member.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                raise ValueError(f"refusing to overwrite existing fixture: {target}")

            with zf.open(info, "r") as source, target.open("wb") as dest:
                shutil.copyfileobj(source, dest)

            extracted.append(
                {
                    "path": target.relative_to(repo_root).as_posix(),
                    "sha256": sha256_file(target),
                    "size": target.stat().st_size,
                    "source_member": info.filename,
                }
            )

    extracted.sort(key=lambda item: item["path"].casefold())
    return {
        "corpus_id": corpus_id,
        "source_archive": {
            "path": preserved_archive.relative_to(repo_root).as_posix(),
            "sha256": sha256_file(preserved_archive),
            "size": preserved_archive.stat().st_size,
        },
        "files": extracted,
    }


def update_manifest(repo_root: Path, record: dict) -> Path:
    manifest_path = repo_root / "tests" / "corpus" / "manifest.json"
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    else:
        manifest = {"version": MANIFEST_VERSION, "sets": []}

    if manifest.get("version") != MANIFEST_VERSION or not isinstance(manifest.get("sets"), list):
        raise ValueError(f"unsupported corpus manifest: {manifest_path}")

    existing = {item.get("corpus_id"): item for item in manifest["sets"]}
    old = existing.get(record["corpus_id"])
    if old is not None and old != record:
        raise ValueError(f"manifest already contains different record for {record['corpus_id']!r}")
    existing[record["corpus_id"]] = record
    manifest["sets"] = [existing[key] for key in sorted(existing)]

    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return manifest_path


def verify_manifest(repo_root: Path) -> int:
    manifest_path = repo_root / "tests" / "corpus" / "manifest.json"
    if not manifest_path.exists():
        print(f"corpus manifest not present: {manifest_path}", file=sys.stderr)
        return 2
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    failures = 0
    for record in manifest.get("sets", []):
        objects = [record["source_archive"], *record.get("files", [])]
        for item in objects:
            path = repo_root / item["path"]
            if not path.is_file():
                print(f"missing: {item['path']}", file=sys.stderr)
                failures += 1
                continue
            actual_size = path.stat().st_size
            actual_hash = sha256_file(path)
            if actual_size != item["size"] or actual_hash != item["sha256"]:
                print(f"mismatch: {item['path']}", file=sys.stderr)
                failures += 1
    if failures == 0:
        print(f"verified {len(manifest.get('sets', []))} corpus set(s)")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path.cwd(), help="vgm-tooling repository root")
    parser.add_argument("--verify", action="store_true", help="verify the committed manifest instead of importing")
    parser.add_argument("--archive", type=Path, help="source ZIP to import")
    parser.add_argument("--id", dest="corpus_id", help="stable corpus id, e.g. sonic-3-knuckles")
    args = parser.parse_args()

    repo_root = args.repo.resolve()
    if args.verify:
        return verify_manifest(repo_root)
    if args.archive is None or not args.corpus_id:
        parser.error("--archive and --id are required when importing")

    archive = args.archive.resolve()
    if not archive.is_file():
        parser.error(f"archive not found: {archive}")

    record = extract_archive(archive, repo_root, args.corpus_id)
    manifest_path = update_manifest(repo_root, record)
    print(f"imported {len(record['files'])} runnable fixture(s)")
    print(f"manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
