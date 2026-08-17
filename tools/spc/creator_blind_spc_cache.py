#!/usr/bin/env python3
"""Persistent creator-blind SPC song cache over the existing forensic runtime.

The expensive operation is controlled SPC execution through
``spc_forensic_features``. Its output is already label-blind and contains the
runtime persistent-part geometry used by CUBE calibration. This wrapper makes
that sidecar song-centered and reusable across panels and attribution questions.

Routine reuse deliberately does not hash the SPC source. Corpus manifests and
Git own exact source identity. Cache reuse checks the forensic model, source
size, capture duration, and the freezer's label/integrity contract. Use
``--refresh`` after intentional extractor/runtime semantic changes.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from typing import Any

THIS_DIR = pathlib.Path(__file__).resolve().parent
if str(THIS_DIR) not in sys.path:
    sys.path.insert(0, str(THIS_DIR))

import freeze_forensic_sidecars as freeze

DEFAULT_CACHE_ROOT = pathlib.Path("research/cache/spc-song-capsules")
DEFAULT_SECONDS = 5
CACHE_GENERATION = 1
EXPECTED_MODEL = freeze.EXPECTED_MODEL


def corpus_id_from_fixture(path: pathlib.Path) -> str:
    parts = path.as_posix().split("/")
    try:
        index = parts.index("corpus")
    except ValueError:
        return path.parent.name or "spc"
    if index + 1 >= len(parts):
        return path.parent.name or "spc"
    return parts[index + 1]


def destination_for(
    source: pathlib.Path,
    *,
    corpus_id: str,
    cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
) -> pathlib.Path:
    return cache_root / corpus_id / f"{source.name}.json"


def _load_json(path: pathlib.Path) -> dict[str, Any] | None:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError):
        return None
    return value if isinstance(value, dict) else None


def cache_current(
    destination: pathlib.Path,
    source: pathlib.Path,
    *,
    seconds: int,
) -> bool:
    if seconds <= 0 or not destination.is_file() or not source.is_file():
        return False
    value = _load_json(destination)
    if value is None or value.get("model") != EXPECTED_MODEL:
        return False
    controlled = value.get("controlled_execution")
    if not isinstance(controlled, dict):
        return False
    if controlled.get("source_bytes") != source.stat().st_size:
        return False
    if controlled.get("requested_seconds") != seconds:
        return False
    try:
        freeze.load_sidecar("cue-000", destination)
    except (OSError, ValueError, json.JSONDecodeError):
        return False
    return True


def build_one(
    source: pathlib.Path,
    *,
    corpus_id: str,
    extractor: pathlib.Path,
    cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
    seconds: int = DEFAULT_SECONDS,
    refresh: bool = False,
) -> tuple[pathlib.Path, bool]:
    source = source.resolve()
    extractor = extractor.resolve()
    if not source.is_file():
        raise FileNotFoundError(f"SPC source not found: {source}")
    if source.suffix.lower() != ".spc":
        raise ValueError("SPC song cache accepts only .spc sources")
    if not extractor.is_file():
        raise FileNotFoundError(f"SPC forensic extractor not found: {extractor}")
    if seconds <= 0:
        raise ValueError("seconds must be positive")

    destination = destination_for(
        source,
        corpus_id=corpus_id,
        cache_root=cache_root,
    )
    if not refresh and cache_current(destination, source, seconds=seconds):
        return destination, False

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".tmp")
    temporary.unlink(missing_ok=True)
    try:
        subprocess.run(
            [str(extractor), str(source), str(temporary), str(seconds)],
            check=True,
        )
        if not cache_current(temporary, source, seconds=seconds):
            raise ValueError(
                "SPC forensic extractor produced a sidecar that failed the creator-blind cache contract"
            )
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    return destination, True


def build_corpus(
    corpus: pathlib.Path,
    *,
    extractor: pathlib.Path,
    cache_root: pathlib.Path = DEFAULT_CACHE_ROOT,
    seconds: int = DEFAULT_SECONDS,
    refresh: bool = False,
) -> dict[str, object]:
    corpus = corpus.resolve()
    if not corpus.is_dir():
        raise NotADirectoryError(corpus)
    corpus_id = corpus.name
    sources = sorted(corpus.rglob("*.spc"))
    built = 0
    reused = 0
    destinations: list[str] = []
    for source in sources:
        destination, changed = build_one(
            source,
            corpus_id=corpus_id,
            extractor=extractor,
            cache_root=cache_root,
            seconds=seconds,
            refresh=refresh,
        )
        destinations.append(destination.as_posix())
        built += int(changed)
        reused += int(not changed)
    return {
        "cache_generation": CACHE_GENERATION,
        "model": EXPECTED_MODEL,
        "corpus_id": corpus_id,
        "selected_tracks": len(sources),
        "built": built,
        "reused": reused,
        "seconds": seconds,
        "destinations": destinations,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    one = subparsers.add_parser("build-one")
    one.add_argument("source", type=pathlib.Path)
    one.add_argument("--corpus-id")

    corpus = subparsers.add_parser("build-corpus")
    corpus.add_argument("corpus", type=pathlib.Path)

    for child in (one, corpus):
        child.add_argument("--extractor", type=pathlib.Path, required=True)
        child.add_argument("--cache-root", type=pathlib.Path, default=DEFAULT_CACHE_ROOT)
        child.add_argument("--seconds", type=int, default=DEFAULT_SECONDS)
        child.add_argument("--refresh", action="store_true")

    args = parser.parse_args()
    if args.command == "build-one":
        corpus_id = args.corpus_id or corpus_id_from_fixture(args.source)
        destination, changed = build_one(
            args.source,
            corpus_id=corpus_id,
            extractor=args.extractor,
            cache_root=args.cache_root,
            seconds=args.seconds,
            refresh=args.refresh,
        )
        result = {
            "cache_generation": CACHE_GENERATION,
            "model": EXPECTED_MODEL,
            "corpus_id": corpus_id,
            "destination": destination.as_posix(),
            "built": int(changed),
            "reused": int(not changed),
            "seconds": args.seconds,
        }
    else:
        result = build_corpus(
            args.corpus,
            extractor=args.extractor,
            cache_root=args.cache_root,
            seconds=args.seconds,
            refresh=args.refresh,
        )
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
