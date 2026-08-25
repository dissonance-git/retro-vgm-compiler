#!/usr/bin/env python3
"""Print a deterministic mechanical inventory of the tracked repository.

The catalog is an on-demand navigation projection, not committed documentation and
not a source of corpus provenance. `tests/corpus/manifest.json` owns corpus
identity; README/AGENTS/docs own human and agent guidance.
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import subprocess
import sys
from typing import Iterable


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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=pathlib.Path, default=None)
    parser.add_argument("--json", action="store_true", dest="print_json")
    args = parser.parse_args()

    repo_root = args.repo_root.resolve() if args.repo_root else repo_root_from(pathlib.Path.cwd())
    inventory = build_inventory(tracked_files(repo_root))
    if args.print_json:
        sys.stdout.write(json.dumps(inventory, indent=2, sort_keys=True) + "\n")
    else:
        sys.stdout.write(render_markdown(inventory))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
