#!/usr/bin/env python3
"""Print deterministic repository projections for navigation.

The catalog is an on-demand navigation projection, not committed documentation and
not a source of corpus provenance. `tests/corpus/manifest.json` owns corpus
identity; README/AGENTS/docs own human and agent guidance.

Use --focus before broad repository search. It intentionally returns a small,
ranked, cross-owner path set so humans and language models can spend context on
canonical files rather than inventory.
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re
import subprocess
import sys
from typing import Iterable


FOCUS_TEXT_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".h",
    ".hpp",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".txt",
    ".yaml",
    ".yml",
}
FOCUS_TEXT_NAMES = {"CMakeLists.txt", "CMakePresets.json"}
FOCUS_MAX_BYTES = 1_000_000


def repo_root_from(start: pathlib.Path) -> pathlib.Path:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            cwd=start,
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return start.resolve()
    return pathlib.Path(result.stdout.strip()).resolve()


def tracked_files(repo_root: pathlib.Path) -> list[str]:
    try:
        result = subprocess.run(
            ["git", "ls-files", "-z"],
            cwd=repo_root,
            check=True,
            capture_output=True,
        )
    except (OSError, subprocess.CalledProcessError):
        files = [
            path.relative_to(repo_root).as_posix()
            for path in repo_root.rglob("*")
            if path.is_file() and ".git" not in path.parts
        ]
    else:
        files = [
            item.decode("utf-8", errors="surrogateescape")
            for item in result.stdout.split(b"\0")
            if item
        ]
    return sorted(files)


def first_level_dirs(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    values: set[str] = set()
    for file in files:
        if file.startswith(root):
            remainder = file[len(root):]
            if "/" in remainder:
                values.add(remainder.split("/", 1)[0])
    return sorted(values)


def root_files(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    return sorted(
        file[len(root):]
        for file in files
        if file.startswith(root) and "/" not in file[len(root):]
    )


def files_under(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    return sorted(file[len(root):] for file in files if file.startswith(root))


def corpus_ids(files: Iterable[str]) -> list[str]:
    values: set[str] = set()
    for file in files:
        parts = pathlib.PurePosixPath(file).parts
        if len(parts) >= 4 and parts[:2] == ("tests", "corpus"):
            values.add(parts[2])
    return sorted(values)


def corpus_extensions(files: Iterable[str]) -> dict[str, int]:
    counts: collections.Counter[str] = collections.Counter()
    for file in files:
        parts = pathlib.PurePosixPath(file).parts
        if len(parts) < 4 or parts[:2] != ("tests", "corpus"):
            continue
        suffix = pathlib.PurePosixPath(file).suffix.lower()
        if suffix in {".sha256", ".json", ".md", ".txt"}:
            continue
        counts[suffix or "<none>"] += 1
    return dict(sorted(counts.items()))


def top_level_counts(files: Iterable[str]) -> dict[str, int]:
    counts: collections.Counter[str] = collections.Counter()
    for file in files:
        parts = pathlib.PurePosixPath(file).parts
        counts[parts[0] if len(parts) > 1 else "<root>"] += 1
    return dict(sorted(counts.items()))


def tool_inventory(files: Iterable[str]) -> list[str]:
    result = []
    for relative in files_under(files, "tools"):
        path = pathlib.PurePosixPath(relative)
        if path.name == "README.md":
            continue
        if path.name == "CMakeLists.txt" or path.suffix.lower() in {
            ".py", ".cpp", ".cc", ".c", ".h", ".hpp", ".cmake", ".ps1", ".sh"
        }:
            result.append(relative)
    return sorted(result)


def build_inventory(files: list[str]) -> dict[str, object]:
    return {
        "schema_version": 3,
        "purpose": "on_demand_navigation_projection_not_source_truth",
        "tracked_file_count": len(files),
        "top_level_tracked_file_counts": top_level_counts(files),
        "component_families": first_level_dirs(files, "components"),
        "research_trunks": first_level_dirs(files, "research"),
        "research_projects": first_level_dirs(files, "research/projects"),
        "corpus": {
            "set_count": len(corpus_ids(files)),
            "ids": corpus_ids(files),
            "runnable_extension_counts": corpus_extensions(files),
        },
        "tool_families": first_level_dirs(files, "tools"),
        "tools": tool_inventory(files),
        "docs": {
            "root_files": root_files(files, "docs"),
            "subdirectories": first_level_dirs(files, "docs"),
        },
        "workflows": root_files(files, ".github/workflows"),
    }


def focus_terms(query: str) -> tuple[str, ...]:
    terms = re.findall(r"[a-z0-9_+.-]+", query.lower())
    return tuple(dict.fromkeys(term for term in terms if term not in {"the", "and", "or"}))


def focus_owner(file: str) -> str:
    parts = pathlib.PurePosixPath(file).parts
    return parts[0] if len(parts) > 1 else "<root>"


def focus_text(repo_root: pathlib.Path, file: str) -> str:
    path = repo_root / file
    pure = pathlib.PurePosixPath(file)
    if pure.name not in FOCUS_TEXT_NAMES and pure.suffix.lower() not in FOCUS_TEXT_SUFFIXES:
        return ""
    try:
        if path.stat().st_size > FOCUS_MAX_BYTES:
            return ""
        return path.read_text(encoding="utf-8", errors="ignore").lower()
    except OSError:
        return ""


def focus_entry(
    repo_root: pathlib.Path,
    file: str,
    query: str,
    terms: tuple[str, ...],
) -> dict[str, object] | None:
    path_lower = file.lower()
    pure = pathlib.PurePosixPath(path_lower)
    parts = set(pure.parts)
    stem = pure.stem
    phrase = query.strip().lower()
    score = 0
    path_hit = False

    if phrase and phrase in path_lower:
        score += 120
        path_hit = True
    for term in terms:
        if term == stem or term in parts:
            score += 100
            path_hit = True
        elif term in path_lower:
            score += 70
            path_hit = True

    text = focus_text(repo_root, file)
    content_hits = 0
    if text:
        if phrase and phrase in text:
            content_hits += 4
        for term in terms:
            content_hits += min(text.count(term), 6)
    if content_hits:
        score += 20 + min(content_hits, 30)

    if score == 0:
        return None
    signal = "path+content" if path_hit and content_hits else "path" if path_hit else "content"
    return {
        "path": file,
        "owner": focus_owner(file),
        "signal": signal,
        "_score": score,
    }


def select_cross_owner(entries: list[dict[str, object]], limit: int) -> list[dict[str, object]]:
    ranked = sorted(entries, key=lambda entry: (-int(entry["_score"]), str(entry["path"])))
    if len(ranked) <= limit:
        return ranked

    seed_limit = max(1, limit // 2)
    seeds: list[dict[str, object]] = []
    seeded_owners: set[str] = set()
    for entry in ranked:
        owner = str(entry["owner"])
        if owner in seeded_owners:
            continue
        seeds.append(entry)
        seeded_owners.add(owner)
        if len(seeds) >= seed_limit:
            break

    selected = list(seeds)
    selected_paths = {str(entry["path"]) for entry in selected}
    for entry in ranked:
        if len(selected) >= limit:
            break
        path = str(entry["path"])
        if path not in selected_paths:
            selected.append(entry)
            selected_paths.add(path)
    return sorted(selected, key=lambda entry: (-int(entry["_score"]), str(entry["path"])))


def build_focus_projection(
    repo_root: pathlib.Path,
    files: list[str],
    query: str,
    limit: int,
) -> dict[str, object]:
    terms = focus_terms(query)
    if not terms:
        raise ValueError("focus query must contain at least one searchable term")
    entries = [
        entry
        for file in files
        if (entry := focus_entry(repo_root, file, query, terms)) is not None
    ]
    selected = select_cross_owner(entries, limit)
    projected = [
        {key: value for key, value in entry.items() if not key.startswith("_")}
        for entry in selected
    ]
    return {
        "schema_version": 1,
        "purpose": "llm_context_compression_projection_not_source_truth",
        "focus": query,
        "tracked_file_count": len(files),
        "matched_file_count": len(entries),
        "selected_file_count": len(projected),
        "selection_ratio": round(len(projected) / len(files), 6) if files else 0.0,
        "files": projected,
    }


def bullet(values: Iterable[str]) -> str:
    values = list(values)
    return "\n".join(f"- `{value}`" for value in values) if values else "- _(none)_"


def render_markdown(inventory: dict[str, object]) -> str:
    corpus = inventory["corpus"]
    docs = inventory["docs"]
    counts = inventory["top_level_tracked_file_counts"]
    assert isinstance(corpus, dict) and isinstance(docs, dict) and isinstance(counts, dict)
    count_lines = "\n".join(f"| `{owner}` | {count} |" for owner, count in counts.items())
    ext_counts = corpus["runnable_extension_counts"]
    assert isinstance(ext_counts, dict)
    ext_lines = "\n".join(f"| `{ext}` | {count} |" for ext, count in ext_counts.items()) or "| _(none)_ | 0 |"
    return f"""# VGM Compiler repository catalog

On-demand mechanical projection from tracked paths. Do not treat this output as
technical truth or commit it as documentation.

Tracked files: **{inventory['tracked_file_count']}**

## Size by shelf

| Shelf | Tracked files |
| --- | ---: |
{count_lines}

## Component families
{bullet(inventory['component_families'])}

## Research trunks
{bullet(inventory['research_trunks'])}

### Named research projects
{bullet(inventory['research_projects'])}

## Corpus
Detected corpus directories: **{corpus['set_count']}**

### Corpus IDs
{bullet(corpus['ids'])}

### Runnable/source extensions
| Extension | Files |
| --- | ---: |
{ext_lines}

## Tool families
{bullet(inventory['tool_families'])}

## Tools (recursive)
{bullet(inventory['tools'])}

## Documentation
### Root documents
{bullet(docs['root_files'])}

### Subdirectories
{bullet(docs['subdirectories'])}

## Workflows
{bullet(inventory['workflows'])}
"""


def render_focus_markdown(projection: dict[str, object]) -> str:
    files = projection["files"]
    assert isinstance(files, list)
    rows = "\n".join(
        f"| `{entry['owner']}` | `{entry['path']}` | {entry['signal']} |"
        for entry in files
        if isinstance(entry, dict)
    )
    if not rows:
        rows = "| _(none)_ | _(no matching tracked text/path)_ | - |"
    ratio = float(projection["selection_ratio"]) * 100.0
    return f"""# VGM Compiler focus: `{projection['focus']}`

Selected **{projection['selected_file_count']}** of **{projection['tracked_file_count']}** tracked files ({ratio:.2f}%).
Matched before cap: **{projection['matched_file_count']}**. Ranked across owners to maximize navigation signal per context token; canonical files remain source truth.

| Owner | File | Match |
| --- | --- | --- |
{rows}
"""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=pathlib.Path, default=None)
    parser.add_argument("--json", action="store_true", dest="print_json")
    parser.add_argument("--focus", help="emit a compact cross-owner projection for one concept")
    parser.add_argument("--limit", type=int, default=16, help="maximum files in --focus projection")
    args = parser.parse_args()

    if args.limit < 1:
        parser.error("--limit must be at least 1")

    repo_root = args.repo_root.resolve() if args.repo_root else repo_root_from(pathlib.Path.cwd())
    files = tracked_files(repo_root)
    if args.focus:
        try:
            projection = build_focus_projection(repo_root, files, args.focus, args.limit)
        except ValueError as exc:
            parser.error(str(exc))
        if args.print_json:
            sys.stdout.write(json.dumps(projection, indent=2, sort_keys=True) + "\n")
        else:
            sys.stdout.write(render_focus_markdown(projection))
        return 0

    inventory = build_inventory(files)
    if args.print_json:
        sys.stdout.write(json.dumps(inventory, indent=2, sort_keys=True) + "\n")
    else:
        sys.stdout.write(render_markdown(inventory))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())