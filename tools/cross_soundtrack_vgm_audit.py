#!/usr/bin/env python3
"""Cross-soundtrack wrapper around the blind Genesis VGM creator audit.

The underlying extractor remains creator-label blind. This wrapper adds only
soundtrack identity from corpus-directory names and can restrict neighbor
searches to other soundtracks.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import pathlib
import sys
from typing import Callable


def _load_base():
    path = pathlib.Path(__file__).with_name("vgm_creator_feature_audit.py")
    spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


base = _load_base()


def _track_id(track: dict[str, object]) -> str:
    return f"{track['soundtrack_id']}::{track['file']}"


def _neighbors(
    tracks: list[dict[str, object]],
    score_fn: Callable[[dict[str, object], dict[str, object]], float],
    limit: int,
    cross_soundtrack_only: bool,
) -> dict[str, list[dict[str, object]]]:
    result: dict[str, list[dict[str, object]]] = {}
    for index, track in enumerate(tracks):
        candidates: list[tuple[float, str, str]] = []
        for other_index, other in enumerate(tracks):
            if index == other_index:
                continue
            if (
                cross_soundtrack_only
                and track["soundtrack_id"] == other["soundtrack_id"]
            ):
                continue
            score = score_fn(track, other)
            candidates.append(
                (score, str(other["soundtrack_id"]), str(other["file"]))
            )
        candidates.sort(key=lambda item: (-item[0], item[1], item[2]))
        result[_track_id(track)] = [
            {"soundtrack_id": soundtrack, "file": file_name, "score": score}
            for score, soundtrack, file_name in candidates[:limit]
        ]
    return result


def audit_soundtracks(
    corpora: list[pathlib.Path],
    neighbor_count: int = 5,
    cross_soundtrack_only: bool = True,
) -> dict[str, object]:
    if not corpora:
        raise ValueError("at least one corpus directory is required")

    seen_ids: set[str] = set()
    tracks: list[dict[str, object]] = []

    for corpus in corpora:
        soundtrack_id = corpus.name
        if soundtrack_id in seen_ids:
            raise ValueError(
                f"duplicate soundtrack identity {soundtrack_id!r}; "
                "use uniquely named corpus directories"
            )
        seen_ids.add(soundtrack_id)

        paths = sorted(
            path
            for path in corpus.iterdir()
            if path.is_file() and path.suffix.lower() in (".vgm", ".vgz")
        )
        if not paths:
            raise ValueError(f"no VGM/VGZ files found in {corpus}")

        for path in paths:
            track = base.audit_file(path)
            track["soundtrack_id"] = soundtrack_id
            track["track_id"] = f"{soundtrack_id}::{path.name}"
            tracks.append(track)

    return {
        "model": "blind cross-soundtrack Genesis VGM creator audit",
        "label_policy": (
            "Soundtrack identity comes only from corpus-directory names. "
            "No composer/artist metadata or candidate labels are read."
        ),
        "claim_boundary": (
            "Cross-soundtrack neighbors are evidence-discovery candidates, not "
            "composer attributions. musical_trajectory is still physical-channel "
            "full-key-on evidence until persistent-part recovery is integrated; "
            "realization similarity remains a separate creator-role coordinate."
        ),
        "soundtracks": sorted(seen_ids),
        "track_count": len(tracks),
        "tracks": tracks,
        "cross_soundtrack_only": cross_soundtrack_only,
        "top_structural_neighbors": _neighbors(
            tracks,
            base.structural_similarity,
            max(0, neighbor_count),
            cross_soundtrack_only,
        ),
        "top_realization_neighbors": _neighbors(
            tracks,
            base.realization_similarity,
            max(0, neighbor_count),
            cross_soundtrack_only,
        ),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpora", type=pathlib.Path, nargs="+")
    parser.add_argument("--json", type=pathlib.Path)
    parser.add_argument("--neighbors", type=int, default=5)
    parser.add_argument(
        "--include-within-soundtrack",
        action="store_true",
        help="allow neighbors from the same soundtrack as well as other soundtracks",
    )
    args = parser.parse_args()

    result = audit_soundtracks(
        args.corpora,
        neighbor_count=args.neighbors,
        cross_soundtrack_only=not args.include_within_soundtrack,
    )
    text = json.dumps(result, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
