#!/usr/bin/env python3
"""Index, import, and verify immutable Game Music Interpreter corpus files."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath

ALLOWED_SUFFIXES = {
    ".vgm", ".vgz", ".spc", ".nsf", ".nsfe",
    ".psf", ".minipsf", ".psflib", ".psf1", ".minipsf1", ".psf1lib",
    ".usf", ".miniusf", ".usflib",
    ".2sf", ".mini2sf", ".2sflib",
}
MANIFEST_VERSION = 2


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_object_sha(kind: str, payload: bytes) -> str:
    header = f"{kind} {len(payload)}\0".encode("ascii")
    return hashlib.sha1(header + payload).hexdigest()


def git_blob_sha(path: Path) -> str:
    return git_object_sha("blob", path.read_bytes())


def git_tree_sha(directory: Path) -> str:
    entries: list[tuple[bytes, bytes]] = []
    for path in directory.iterdir():
        name = path.name.encode("utf-8")
        if path.is_dir():
            sha = git_tree_sha(path)
            sort_name = name + b"/"
            entry = b"40000 " + name + b"\0" + bytes.fromhex(sha)
        elif path.is_file():
            sha = git_blob_sha(path)
            sort_name = name
            entry = b"100644 " + name + b"\0" + bytes.fromhex(sha)
        else:
            continue
        entries.append((sort_name, entry))
    payload = b"".join(entry for _, entry in sorted(entries, key=lambda item: item[0]))
    return git_object_sha("tree", payload)


def safe_member_path(name: str) -> PurePosixPath:
    candidate = PurePosixPath(name.replace("\\", "/"))
    if candidate.is_absolute() or ".." in candidate.parts or not candidate.parts:
        raise ValueError(f"unsafe ZIP member path: {name!r}")
    return candidate


def member_is_symlink(info: zipfile.ZipInfo) -> bool:
    mode = (info.external_attr >> 16) & 0xFFFF
    return stat.S_ISLNK(mode)


def runnable_files(directory: Path) -> list[Path]:
    return sorted(
        (p for p in directory.rglob("*") if p.is_file() and p.suffix.lower() in ALLOWED_SUFFIXES),
        key=lambda p: p.relative_to(directory).as_posix().casefold(),
    )


def inventory_text(directory: Path) -> str:
    return "".join(
        f"{sha256_file(path)}  {path.relative_to(directory).as_posix()}\n"
        for path in runnable_files(directory)
    )


def parse_inventory(path: Path) -> list[tuple[str, str]]:
    items: list[tuple[str, str]] = []
    for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw:
            continue
        if len(raw) < 67 or raw[64:66] != "  ":
            raise ValueError(f"invalid SHA-256 inventory line {line_no}: {path}")
        digest, name = raw[:64], raw[66:]
        if len(digest) != 64 or any(ch not in "0123456789abcdef" for ch in digest):
            raise ValueError(f"invalid SHA-256 on line {line_no}: {path}")
        items.append((name, digest))
    return items


def set_sha256(record: dict, items: list[tuple[str, str]], corpus_dir: Path) -> str:
    digest = hashlib.sha256()
    base = record["path"].rstrip("/") + "/"
    for name, file_hash in sorted(items, key=lambda item: (base + item[0]).casefold()):
        file_path = corpus_dir / name
        digest.update((base + name).encode("utf-8"))
        digest.update(b"\0")
        digest.update(file_hash.encode("ascii"))
        digest.update(b"\0")
        digest.update(str(file_path.stat().st_size).encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def load_manifest(path: Path) -> dict:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("version") != MANIFEST_VERSION or not isinstance(manifest.get("sets"), list):
        raise ValueError(f"unsupported corpus manifest: {path}")
    return manifest


def update_direct_record(repo_root: Path, corpus_id: str) -> Path:
    manifest_path = repo_root / "tests" / "corpus" / "manifest.json"
    manifest = load_manifest(manifest_path)
    records = {record["corpus_id"]: record for record in manifest["sets"]}
    record = records.get(corpus_id, {"corpus_id": corpus_id})
    corpus_dir = repo_root / record.get("path", f"tests/corpus/{corpus_id}")
    files = runnable_files(corpus_dir)
    if not files:
        raise ValueError(f"no runnable fixtures found: {corpus_dir}")

    inventory_rel = record.get("sha256_inventory", f"tests/corpus/{corpus_id}.sha256")
    inventory_path = repo_root / inventory_rel
    inventory_path.write_text(inventory_text(corpus_dir), encoding="utf-8")
    items = parse_inventory(inventory_path)

    record.update(
        {
            "path": corpus_dir.relative_to(repo_root).as_posix(),
            "delivery": "direct-files",
            "fixture_count": len(items),
            "sha256_inventory": inventory_rel,
            "git_tree_sha1": git_tree_sha(corpus_dir),
        }
    )
    record["set_sha256"] = set_sha256(record, items, corpus_dir)
    records[corpus_id] = record
    manifest["sets"] = [records[key] for key in sorted(records)]
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return manifest_path


def extract_archive(archive: Path, repo_root: Path, corpus_id: str) -> None:
    corpus_root = repo_root / "tests" / "corpus"
    archive_dir = corpus_root / "archives"
    extracted_dir = corpus_root / corpus_id
    archive_dir.mkdir(parents=True, exist_ok=True)
    extracted_dir.mkdir(parents=True, exist_ok=True)

    preserved = archive_dir / archive.name
    if preserved.exists() and sha256_file(preserved) != sha256_file(archive):
        raise ValueError(f"archive already exists with different bytes: {preserved}")
    if not preserved.exists():
        shutil.copyfile(archive, preserved)

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


def verify_manifest(repo_root: Path) -> int:
    manifest_path = repo_root / "tests" / "corpus" / "manifest.json"
    if not manifest_path.exists():
        print(f"corpus manifest not present: {manifest_path}", file=sys.stderr)
        return 2
    manifest = load_manifest(manifest_path)
    failures = 0
    verified = 0

    for record in manifest["sets"]:
        corpus_dir = repo_root / record["path"]
        inventory_path = repo_root / record["sha256_inventory"]
        if not corpus_dir.is_dir() or not inventory_path.is_file():
            print(f"missing corpus or inventory: {record['corpus_id']}", file=sys.stderr)
            failures += 1
            continue

        try:
            items = parse_inventory(inventory_path)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            failures += 1
            continue

        names = [name for name, _ in items]
        actual_names = [p.relative_to(corpus_dir).as_posix() for p in runnable_files(corpus_dir)]
        if names != actual_names or len(items) != record["fixture_count"]:
            print(f"inventory membership mismatch: {record['corpus_id']}", file=sys.stderr)
            failures += 1
            continue

        for name, expected_hash in items:
            path = corpus_dir / name
            if sha256_file(path) != expected_hash:
                print(f"hash mismatch: {record['path']}/{name}", file=sys.stderr)
                failures += 1
            else:
                verified += 1

        if set_sha256(record, items, corpus_dir) != record["set_sha256"]:
            print(f"set digest mismatch: {record['corpus_id']}", file=sys.stderr)
            failures += 1
        if git_tree_sha(corpus_dir) != record["git_tree_sha1"]:
            print(f"Git tree mismatch: {record['corpus_id']}", file=sys.stderr)
            failures += 1

    if failures == 0:
        print(f"verified {len(manifest['sets'])} corpus set(s), {verified} fixture(s)")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=Path.cwd(), help="vgm-tooling repository root")
    parser.add_argument("--verify", action="store_true", help="verify committed corpus bytes and inventories")
    parser.add_argument("--record-existing", action="store_true", help="record files already under tests/corpus/<id>")
    parser.add_argument("--archive", type=Path, help="optional source ZIP to preserve and extract")
    parser.add_argument("--id", dest="corpus_id", help="stable corpus id")
    args = parser.parse_args()

    repo_root = args.repo.resolve()
    if args.verify:
        return verify_manifest(repo_root)
    if not args.corpus_id:
        parser.error("--id is required when recording or importing")
    if bool(args.archive) == bool(args.record_existing):
        parser.error("choose exactly one of --archive or --record-existing")

    if args.archive:
        archive = args.archive.resolve()
        if not archive.is_file():
            parser.error(f"archive not found: {archive}")
        extract_archive(archive, repo_root, args.corpus_id)

    manifest_path = update_direct_record(repo_root, args.corpus_id)
    print(f"manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
