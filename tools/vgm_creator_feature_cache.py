#!/usr/bin/env python3
"""Extract reusable creator-blind Genesis VGM feature capsules once per song.

The expensive VGM parse belongs at ingestion time. Later attribution research
should load these JSON capsules and never reopen the source VGM/VGZ unless a new
representation needs evidence the cache does not contain.

Each capsule also keeps a canonical ordinary-YM2612-key-on event lane. That is a
small analysis-ready copy of the observed song geometry, so many future feature
ideas can be derived from cache rather than by reopening the VGM.

Composer/artist labels are intentionally excluded. Keep documentary labels in
separate policy/index files so extraction stays blind and reusable.
"""

from __future__ import annotations

import argparse
import gzip
import json
import pathlib
import re
from typing import Iterable

import vgm_creator_feature_audit as gen1
import vgm_creator_part_matcher as gen2


SCHEMA_VERSION = 2
MODEL = "creator-blind Genesis VGM reusable feature capsule"
DEFAULT_CACHE_ROOT = pathlib.Path("research/cache/creator-feature-cache/tracks")


def _slug(text: str) -> str:
    value = re.sub(r"[^A-Za-z0-9._-]+", "-", text.strip()).strip("-")
    return value or "track"


def _raw_vgm(path: pathlib.Path) -> bytes:
    packed = path.read_bytes()
    return gzip.decompress(packed) if path.suffix.lower() == ".vgz" else packed


def _canonical_event_lane(path: pathlib.Path) -> dict[str, object]:
    """Keep reusable creator-blind ordinary full-FM key-on evidence.

    This is intentionally closer to the parsed song than any particular
    similarity metric. It preserves timing, channel, pitch registers, and the
    realization state already available at each accepted key-on. Future
    composition-facing lenses can therefore usually be derived from JSON only.
    """
    state = gen1.GenesisAuditState()
    channel_map = {0: 0, 1: 1, 2: 2, 4: 3, 5: 4, 6: 5}
    rows: list[dict[str, object]] = []
    last_tick = 0

    for command in gen1.command_stream(_raw_vgm(path)):
        last_tick = max(last_tick, command.tick)
        if command.opcode not in (0x52, 0x53):
            continue

        register, value = command.args
        port = 0 if command.opcode == 0x52 else 1
        state.update(port, register, value)

        if port != 0 or register != 0x28:
            continue
        key_mask = value & 0xF0
        encoded_channel = value & 0x07
        if key_mask != 0xF0 or encoded_channel not in channel_map:
            continue
        channel = channel_map[encoded_channel]
        onset = state.onset(command.tick, channel)
        if onset is None:
            continue
        rows.append(
            {
                "tick": onset.tick,
                "channel": onset.channel,
                "fnum": onset.fnum,
                "block": onset.block,
                "patch_core": onset.patch_core,
                "patch_full": onset.patch_full,
                "algorithm": onset.algorithm,
                "feedback": onset.feedback,
                "ams": onset.ams,
                "fms": onset.fms,
                "pan": onset.pan,
            }
        )

    return {
        "duration_vgm_samples": last_tick,
        "ordinary_full_fm_key_on_count": len(rows),
        "ordinary_full_fm_key_ons": rows,
        "claim_boundary": (
            "Observed ordinary full YM2612 key-ons after DAC and CH3-special exclusions. "
            "This is execution evidence, not original notation or persistent-part identity."
        ),
    }


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
    # These are all ingestion-time operations. Normal research uses only the
    # resulting capsule. The canonical lane is retained so future views can be
    # derived without another source-file pass.
    canonical = _canonical_event_lane(path)
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
            "canonical_events": canonical,
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
            "Cache directory. Defaults to "
            "research/cache/creator-feature-cache/tracks/<soundtrack-id>."
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
    output_dir = args.out or DEFAULT_CACHE_ROOT / _slug(args.soundtrack_id)
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
