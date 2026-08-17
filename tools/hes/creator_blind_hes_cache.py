#!/usr/bin/env python3
"""Cache one explicitly selected creator-blind HES track capture."""
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys

THIS_DIR = pathlib.Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))
import validate_forensic_sidecar as sidecar

DEFAULT_CACHE_ROOT = pathlib.Path("research/cache/hes-song-capsules")
DEFAULT_SECONDS = 60
CACHE_GENERATION = 1


def corpus_id_from_fixture(path: pathlib.Path) -> str:
    parts = path.as_posix().split("/")
    try:
        index = parts.index("corpus")
    except ValueError:
        return path.parent.name or "hes"
    return parts[index + 1] if index + 1 < len(parts) else (path.parent.name or "hes")


def destination_for(
    source: pathlib.Path, *, corpus_id: str, track_index: int,
    seconds: int = DEFAULT_SECONDS, cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
    playlist: pathlib.Path | None = None,
) -> pathlib.Path:
    if track_index < 0 or seconds <= 0:
        raise ValueError("track_index must be nonnegative and seconds positive")
    playlist_key = "raw" if playlist is None else f"m3u-{playlist.name}"
    return (
        cache_root / corpus_id / f"{seconds}s" / playlist_key
        / f"track-{track_index:03d}" / f"{source.name}.json"
    )


def cache_current(
    destination: pathlib.Path, source: pathlib.Path, *, track_index: int,
    seconds: int, playlist: pathlib.Path | None = None,
) -> bool:
    if not destination.is_file() or not source.is_file():
        return False
    if playlist is not None and not playlist.is_file():
        return False
    try:
        sidecar.load_and_validate(
            destination,
            source_size=source.stat().st_size,
            playlist_size=playlist.stat().st_size if playlist is not None else 0,
            playlist_loaded=playlist is not None,
            track_index=track_index,
            seconds=seconds,
        )
    except (OSError, UnicodeDecodeError, json.JSONDecodeError, ValueError):
        return False
    return True


def build_one(
    source: pathlib.Path, *, corpus_id: str, track_index: int,
    extractor: pathlib.Path, seconds: int = DEFAULT_SECONDS,
    cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
    playlist: pathlib.Path | None = None, refresh: bool = False,
) -> tuple[pathlib.Path, bool]:
    source = source.resolve()
    extractor = extractor.resolve()
    playlist = playlist.resolve() if playlist is not None else None
    if not source.is_file() or source.suffix.lower() != ".hes":
        raise FileNotFoundError(f"HES source not found or not .hes: {source}")
    if playlist is not None and not playlist.is_file():
        raise FileNotFoundError(f"HES M3U not found: {playlist}")
    if not extractor.is_file():
        raise FileNotFoundError(f"HES forensic extractor not found: {extractor}")
    if track_index < 0 or seconds <= 0:
        raise ValueError("track_index must be nonnegative and seconds positive")

    destination = destination_for(
        source, corpus_id=corpus_id, track_index=track_index, seconds=seconds,
        cache_root=cache_root, playlist=playlist,
    )
    if not refresh and cache_current(
        destination, source, track_index=track_index, seconds=seconds, playlist=playlist,
    ):
        return destination, False

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".tmp")
    temporary.unlink(missing_ok=True)
    command = [str(extractor), str(source), "--track", str(track_index)]
    if playlist is not None:
        command += ["--m3u", str(playlist)]
    command += ["--seconds", str(seconds), "--json", str(temporary)]
    try:
        subprocess.run(command, check=True)
        if not cache_current(
            temporary, source, track_index=track_index, seconds=seconds, playlist=playlist,
        ):
            raise ValueError("HES extractor output failed the creator-blind cache contract")
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    return destination, True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--track", type=int, required=True)
    parser.add_argument("--m3u", type=pathlib.Path)
    parser.add_argument("--corpus-id")
    parser.add_argument("--extractor", type=pathlib.Path, required=True)
    parser.add_argument("--cache-root", type=pathlib.Path, default=DEFAULT_CACHE_ROOT)
    parser.add_argument("--seconds", type=int, default=DEFAULT_SECONDS)
    parser.add_argument("--refresh", action="store_true")
    args = parser.parse_args()
    corpus_id = args.corpus_id or corpus_id_from_fixture(args.source)
    destination, changed = build_one(
        args.source, corpus_id=corpus_id, track_index=args.track,
        extractor=args.extractor, seconds=args.seconds, cache_root=args.cache_root,
        playlist=args.m3u, refresh=args.refresh,
    )
    print(json.dumps({
        "cache_generation": CACHE_GENERATION,
        "model": sidecar.EXPECTED_MODEL,
        "corpus_id": corpus_id,
        "track_index": args.track,
        "seconds": args.seconds,
        "destination": destination.as_posix(),
        "built": int(changed),
        "reused": int(not changed),
    }, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
