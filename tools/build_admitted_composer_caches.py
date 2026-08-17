#!/usr/bin/env python3
"""Build creator-blind song capsules for all canonically admitted composers.

Historical/evidential ownership stays in the files that already own it. The
Genesis routing index supplies established VGM/VGZ controls; the Sonic 3
attribution admissions supply grounded cross-format controls such as CUBE SPC
cues. This helper joins those sources at runtime, normalizes only the fields the
cache router needs, and sends compatible fixtures to the current Genesis
capsule backend.

Unsupported formats remain visible in the report for future capsule backends.
No second composer-credit truth is written.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import tempfile
from typing import Iterable

from creator_blind_song_cache import (
    DEFAULT_CACHE_INDEX,
    DEFAULT_CACHE_ROOT,
    build_creator,
)

DEFAULT_CREDITS = pathlib.Path("research/projects/sonic3/role-credit-index.jsonl")
DEFAULT_ADMISSIONS = pathlib.Path(
    "research/projects/sonic3/attribution-control-admissions.jsonl"
)
DEFAULT_ROLE = "composer"
DEFAULT_ADMITTED_STATUSES = frozenset({"exact", "derived", "whole_soundtrack"})
GENESIS_VGM_SUFFIXES = frozenset({".vgm", ".vgz"})
CACHE_BACKEND = "creator-blind-genesis-song-capsule"


def read_jsonl(path: pathlib.Path) -> list[dict[str, object]]:
    if not path.is_file():
        return []
    return [
        json.loads(line)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]


def _corpus_id_from_fixture(fixture_path: str) -> str | None:
    parts = pathlib.PurePosixPath(fixture_path).parts
    if len(parts) >= 4 and parts[:2] == ("tests", "corpus"):
        return parts[2]
    return None


def normalize_admission_records(
    records: Iterable[dict[str, object]],
    *,
    admissions_path: pathlib.Path = DEFAULT_ADMISSIONS,
) -> list[dict[str, object]]:
    """Project canonical admission rows into the small cache-routing schema."""
    normalized: list[dict[str, object]] = []
    for record in records:
        candidate = record.get("candidate")
        fixture = record.get("fixture_path")
        if not isinstance(candidate, str) or not candidate:
            continue
        if not isinstance(fixture, str) or not fixture:
            continue
        projected = dict(record)
        projected["creator"] = candidate
        projected.setdefault("mapping_state", "canonical_attribution_control_admission")
        projected.setdefault("source_policy", admissions_path.as_posix())
        corpus_id = _corpus_id_from_fixture(fixture)
        if corpus_id is not None:
            projected.setdefault("corpus_id", corpus_id)
        projected["control_source"] = "canonical_attribution_admission"
        normalized.append(projected)
    return normalized


def combined_control_records(
    *,
    credit_index: pathlib.Path,
    admissions_path: pathlib.Path | None,
) -> list[dict[str, object]]:
    """Join routing rows and canonical admissions without duplicating controls."""
    routing = read_jsonl(credit_index)
    admissions = (
        normalize_admission_records(
            read_jsonl(admissions_path), admissions_path=admissions_path
        )
        if admissions_path is not None
        else []
    )

    merged: dict[tuple[str, str, str], dict[str, object]] = {}
    for record in routing:
        key = (
            str(record.get("creator", "")),
            str(record.get("role", "")),
            str(record.get("fixture_path", "")),
        )
        merged[key] = record
    # Canonical admission rows override a routing duplicate if one ever appears.
    for record in admissions:
        key = (
            str(record.get("creator", "")),
            str(record.get("role", "")),
            str(record.get("fixture_path", "")),
        )
        merged[key] = record
    return sorted(
        merged.values(),
        key=lambda record: (
            str(record.get("creator", "")),
            str(record.get("fixture_path", "")),
        ),
    )


def admitted_records(
    records: Iterable[dict[str, object]],
    *,
    role: str = DEFAULT_ROLE,
    admitted_statuses: set[str] | frozenset[str] = DEFAULT_ADMITTED_STATUSES,
) -> list[dict[str, object]]:
    admitted = [
        record
        for record in records
        if record.get("role") == role
        and record.get("status") in admitted_statuses
        and isinstance(record.get("creator"), str)
        and bool(record.get("creator"))
    ]
    admitted.sort(
        key=lambda record: (
            str(record.get("creator", "")),
            str(record.get("fixture_path", "")),
        )
    )
    return admitted


def admitted_creators(
    records: Iterable[dict[str, object]],
    *,
    role: str = DEFAULT_ROLE,
    admitted_statuses: set[str] | frozenset[str] = DEFAULT_ADMITTED_STATUSES,
) -> list[str]:
    return sorted({
        str(record["creator"])
        for record in admitted_records(
            records,
            role=role,
            admitted_statuses=admitted_statuses,
        )
    })


def cacheable_by_current_backend(record: dict[str, object]) -> bool:
    fixture = record.get("fixture_path")
    if not isinstance(fixture, str):
        return False
    return pathlib.PurePosixPath(fixture).suffix.lower() in GENESIS_VGM_SUFFIXES


def _write_jsonl(path: pathlib.Path, records: Iterable[dict[str, object]]) -> None:
    path.write_text(
        "".join(json.dumps(record, sort_keys=True) + "\n" for record in records),
        encoding="utf-8",
    )


def build_all(
    *,
    credit_index: pathlib.Path,
    admissions_path: pathlib.Path | None,
    role: str,
    repo_root: pathlib.Path,
    cache_root: pathlib.Path,
    cache_index: pathlib.Path,
    refresh: bool,
    admitted_statuses: set[str],
) -> dict[str, object]:
    records = combined_control_records(
        credit_index=credit_index,
        admissions_path=admissions_path,
    )
    admitted = admitted_records(
        records,
        role=role,
        admitted_statuses=admitted_statuses,
    )
    creators = sorted({str(record["creator"]) for record in admitted})

    results: list[dict[str, object]] = []
    with tempfile.TemporaryDirectory(prefix="rvc-composer-cache-") as temp_dir:
        temporary_root = pathlib.Path(temp_dir)
        for creator in creators:
            creator_records = [
                record for record in admitted if record.get("creator") == creator
            ]
            compatible = [
                record for record in creator_records if cacheable_by_current_backend(record)
            ]
            unsupported = [
                record for record in creator_records if not cacheable_by_current_backend(record)
            ]

            backend_result: dict[str, object]
            if compatible:
                filtered_index = temporary_root / f"credits-{len(results):04d}.jsonl"
                _write_jsonl(filtered_index, compatible)
                backend_result = build_creator(
                    credit_index=filtered_index,
                    creator=creator,
                    role=role,
                    repo_root=repo_root,
                    cache_root=cache_root,
                    cache_index=cache_index,
                    refresh=refresh,
                    admitted_statuses=admitted_statuses,
                )
            else:
                backend_result = {
                    "creator": creator,
                    "role": role,
                    "selected_tracks": 0,
                    "built": 0,
                    "reused": 0,
                    "cache_root": cache_root.as_posix(),
                    "cache_index": cache_index.as_posix(),
                }

            result = dict(backend_result)
            result.update(
                {
                    "role_selected_tracks": len(creator_records),
                    "cacheable_tracks": len(compatible),
                    "unsupported_tracks": len(unsupported),
                    "unsupported_fixtures": [
                        str(record.get("fixture_path", "")) for record in unsupported
                    ],
                }
            )
            results.append(result)

    cacheable_total = sum(int(result["cacheable_tracks"]) for result in results)
    return {
        "role": role,
        "creator_count": len(creators),
        "creators": creators,
        "cache_backend": CACHE_BACKEND,
        "control_sources": [
            credit_index.as_posix(),
            admissions_path.as_posix() if admissions_path is not None else None,
        ],
        "role_selected_tracks": len(admitted),
        "cacheable_tracks": cacheable_total,
        "unsupported_tracks": sum(int(result["unsupported_tracks"]) for result in results),
        # Compatibility projection retained for existing consumers.
        "selected_tracks": cacheable_total,
        "built": sum(int(result["built"]) for result in results),
        "reused": sum(int(result["reused"]) for result in results),
        "results": results,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--credits", type=pathlib.Path, default=DEFAULT_CREDITS)
    parser.add_argument("--admissions", type=pathlib.Path, default=DEFAULT_ADMISSIONS)
    parser.add_argument("--role", default=DEFAULT_ROLE)
    parser.add_argument("--repo-root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--cache-root", type=pathlib.Path, default=DEFAULT_CACHE_ROOT)
    parser.add_argument("--cache-index", type=pathlib.Path, default=DEFAULT_CACHE_INDEX)
    parser.add_argument("--refresh", action="store_true")
    parser.add_argument(
        "--status",
        action="append",
        dest="statuses",
        help="Admitted credit status. Repeat to override the default admitted statuses.",
    )
    args = parser.parse_args()
    statuses = set(args.statuses) if args.statuses else set(DEFAULT_ADMITTED_STATUSES)
    result = build_all(
        credit_index=args.credits,
        admissions_path=args.admissions,
        role=args.role,
        repo_root=args.repo_root,
        cache_root=args.cache_root,
        cache_index=args.cache_index,
        refresh=args.refresh,
        admitted_statuses=statuses,
    )
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
