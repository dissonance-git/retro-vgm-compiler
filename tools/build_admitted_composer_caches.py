#!/usr/bin/env python3
"""Build creator-blind song capsules for every admitted composer in a credit index.

The role-credit index owns historical labels. The song cache stays creator-blind.
This helper simply fans the existing ``build_creator`` operation across every
admitted composer so a research world can be parsed once and reused thereafter.
"""
from __future__ import annotations

import argparse
import json
import pathlib
from typing import Iterable

from creator_blind_song_cache import (
    DEFAULT_CACHE_INDEX,
    DEFAULT_CACHE_ROOT,
    build_creator,
)

DEFAULT_CREDITS = pathlib.Path("research/projects/sonic3/role-credit-index.jsonl")
DEFAULT_ROLE = "composer"
DEFAULT_ADMITTED_STATUSES = frozenset({"exact", "derived", "whole_soundtrack"})


def read_jsonl(path: pathlib.Path) -> list[dict[str, object]]:
    if not path.is_file():
        return []
    return [
        json.loads(line)
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]


def admitted_creators(
    records: Iterable[dict[str, object]],
    *,
    role: str = DEFAULT_ROLE,
    admitted_statuses: set[str] | frozenset[str] = DEFAULT_ADMITTED_STATUSES,
) -> list[str]:
    return sorted({
        creator
        for record in records
        if record.get("role") == role
        and record.get("status") in admitted_statuses
        and isinstance((creator := record.get("creator")), str)
        and creator
    })


def build_all(
    *,
    credit_index: pathlib.Path,
    role: str,
    repo_root: pathlib.Path,
    cache_root: pathlib.Path,
    cache_index: pathlib.Path,
    refresh: bool,
    admitted_statuses: set[str],
) -> dict[str, object]:
    creators = admitted_creators(
        read_jsonl(credit_index),
        role=role,
        admitted_statuses=admitted_statuses,
    )
    results = [
        build_creator(
            credit_index=credit_index,
            creator=creator,
            role=role,
            repo_root=repo_root,
            cache_root=cache_root,
            cache_index=cache_index,
            refresh=refresh,
            admitted_statuses=admitted_statuses,
        )
        for creator in creators
    ]
    return {
        "role": role,
        "creator_count": len(creators),
        "creators": creators,
        "selected_tracks": sum(int(result["selected_tracks"]) for result in results),
        "built": sum(int(result["built"]) for result in results),
        "reused": sum(int(result["reused"]) for result in results),
        "results": results,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--credits", type=pathlib.Path, default=DEFAULT_CREDITS)
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
