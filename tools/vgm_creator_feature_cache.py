#!/usr/bin/env python3
"""Extract reusable creator-blind Genesis VGM feature capsules once per song.

The expensive VGM parse belongs at ingestion time.  Later attribution research
should load these JSON capsules and never reopen the source VGM/VGZ unless a new
representation needs evidence the cache does not contain.

Composer/artist labels are intentionally excluded.  Keep documentary labels in
separate policy/index files so extraction stays blind and reusable.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
from typing import Iterable

import vgm_creator_feature_audit as gen1
import vgm_creator_part_matcher as gen2


SCHEMA_VERSION = 1
MODEL = "creator-blind Genesis VGM reusable feature capsule"


def _slug(text: str) -> str:
    value = re.sub(r"[^A-Za-z0-9._-]+", "-", text.strip()).strip("-")
    return value or "track"


def _motion_part(part: dict[str, object]) -> dict[str, object]:
    intervals = dict(part.get("interval_histogram_semitones", {}))
    bigrams = dict(part.get("interval_bigram_histogram", {}))

    motion_intervals = {
        key: int(value)
        for key, value in intervals.items()
        if str(key) != "0"
    }
    motion_bigrams: dict[str, int] = {}
    for key, value in bigrams.items():
        left, sep, right = str(key).partition(",")
        if not sep or left == "0" or right == "0":
            continue
        motion_bigrams[str(key)] = int(value)

    return {
        "channel": int(part["channel"]),
        "key_ons": int(part.get("key_ons", 0)),
        "interval_histogram_semitones": motion_intervals,
        "interval_bigram_histogram": motion_bigrams,
    }


def extract_capsule(path: pathlib.Path, soundtrack_id: str) -> dict[str, object]:
    base = gen1.audit_file(path)
    parts = gen2.extract_part_features(path)
    raw_parts = list(parts.get("parts", []))  # type: ignore[arg-type]
    clean_parts = [
        {
            "channel": int(part["channel"]),
            "key_ons": int(part.get("key_ons", 0)),
            "interval_histogram_semitones": dict(part["interval_histogram_semitones"]),
            "interval_bigram_histogram": dict(part["interval_bigram_histogram"]),
        }
        for part in raw_parts
    ]

    return {
        "schema_version": SCHEMA_VERSION,
        "model": MODEL,
        "label_policy": "No composer/artist metadata is stored in this capsule.",
        "source": {
            "soundtrack_id": soundtrack_id,
            "file": path.name,
            "source_path": str(path),
        },
        "views": {
            "gen1": base,
            "gen2_parts": {
                "parts": clean_parts,
                "definition": (
                    "Per-physical-channel relative-semitone interval and interval-bigram "
                    "histograms before cross-track part assignment."
                ),
            },
            "gen3_motion_parts": {
                "parts": [_motion_part(part) for part in clean_parts],
                "definition": (
                    "Gen-2 parts with zero-semitone intervals removed and every bigram "
                    "touching zero removed; no score or creator label is cached."
                ),
            },
        },
    }


def _source_paths(inputs: Iterable[str]) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for raw in inputs:
        path = pathlib.Path(raw)
        if path.is_dir():
            found.extend(
                candidate
                for candidate in path.rglob("*")
                if candidate.is_file() and candidate.suffix.lower() in {".vgm", ".vgz"}
            )
        elif path.is_file() and path.suffix.lower() in {".vgm", ".vgz"}:
            found.append(path)
        else:
            raise SystemExit(f"not a VGM/VGZ file or directory: {path}")
    return sorted(dict.fromkeys(found), key=lambda item: str(item).lower())


def cache_corpus(
    paths: list[pathlib.Path],
    *,
    soundtrack_id: str,
    output_dir: pathlib.Path,
    refresh: bool = False,
) -> dict[str, object]:
    output_dir.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, object]] = []

    for index, path in enumerate(paths, start=1):
        filename = f"{index:03d}-{_slug(path.stem)}.json"
        destination = output_dir / filename
        state = "cached"
        if refresh or not destination.exists():
            payload = extract_capsule(path, soundtrack_id)
            destination.write_text(
                json.dumps(payload, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            state = "extracted"
        rows.append(
            {
                "source_file": path.name,
                "cache_file": filename,
                "state": state,
            }
        )

    manifest = {
        "schema_version": SCHEMA_VERSION,
        "model": "creator-blind Genesis VGM feature-cache manifest",
        "soundtrack_id": soundtrack_id,
        "track_count": len(rows),
        "label_policy": (
            "Composer/artist labels do not belong in this directory. Overlay labels from "
            "documentary policy/index files only after feature extraction."
        ),
        "cache_policy": (
            "One JSON capsule per source song. Existing capsules are reused by default; "
            "use --refresh only when the extractor/schema intentionally changes."
        ),
        "tracks": rows,
    }
    (output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", help="VGM/VGZ files or corpus directories")
    parser.add_argument("--soundtrack-id", required=True)
    parser.add_argument(
        "--out",
        type=pathlib.Path,
        help=(
            "Cache directory. Defaults to research/music/creator-feature-cache/tracks/<soundtrack-id>."
        ),
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Re-extract existing song capsules. Normal research should not need this.",
    )
    args = parser.parse_args()

    paths = _source_paths(args.inputs)
    if not paths:
        raise SystemExit("no VGM/VGZ inputs found")
    output_dir = args.out or pathlib.Path(
        "research/music/creator-feature-cache/tracks"
    ) / _slug(args.soundtrack_id)
    manifest = cache_corpus(
        paths,
        soundtrack_id=args.soundtrack_id,
        output_dir=output_dir,
        refresh=args.refresh,
    )
    print(json.dumps(manifest, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
