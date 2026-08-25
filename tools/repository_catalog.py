#!/usr/bin/env python3
"""Generate compact repository inventory projections.

This tool answers mechanical navigation questions without making agents rediscover
repository contents by repeated broad searches. It intentionally does not hash
files or duplicate corpus provenance; tests/corpus/manifest.json owns corpus
identity and provenance.
"""
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import subprocess
import sys
from typing import Iterable

GENERATED_MD = pathlib.Path("docs/generated/repository-catalog.md")
GENERATED_JSON = pathlib.Path("docs/generated/repository-catalog.json")
SELF_OUTPUTS = {GENERATED_MD.as_posix(), GENERATED_JSON.as_posix()}


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
    return sorted(path for path in files if path not in SELF_OUTPUTS)


def first_level_dirs(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    values: set[str] = set()
    for file in files:
        if not file.startswith(root):
            continue
        remainder = file[len(root) :]
        if "/" in remainder:
            values.add(remainder.split("/", 1)[0])
    return sorted(values)


def root_files(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    values = []
    for file in files:
        if not file.startswith(root):
            continue
        remainder = file[len(root) :]
        if "/" not in remainder:
            values.append(remainder)
    return sorted(values)


def files_under(files: Iterable[str], prefix: str) -> list[str]:
    root = prefix.rstrip("/") + "/"
    return sorted(file[len(root) :] for file in files if file.startswith(root))


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
        owner = parts[0] if len(parts) > 1 else "<root>"
        counts[owner] += 1
    return dict(sorted(counts.items()))


def tool_inventory(files: Iterable[str]) -> list[str]:
    """List nested executable/source tool entries, not only tools/*.py."""
    result = []
    for relative in files_under(files, "tools"):
        path = pathlib.PurePosixPath(relative)
        if path.name == "README.md":
            continue
        if path.name == "CMakeLists.txt" or path.suffix.lower() in {
            ".py",
            ".cpp",
            ".cc",
            ".c",
            ".h",
            ".hpp",
            ".cmake",
            ".ps1",
            ".sh",
        }:
            result.append(relative)
    return sorted(result)


def build_inventory(files: list[str]) -> dict[str, object]:
    return {
        "schema_version": 2,
        "purpose": "navigation_projection_not_source_truth",
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


def bullet(values: Iterable[str], indent: str = "") -> str:
    values = list(values)
    if not values:
        return f"{indent}- _(none)_"
    return "\n".join(f"{indent}- `{value}`" for value in values)


def render_markdown(inventory: dict[str, object]) -> str:
    corpus = inventory["corpus"]
    docs = inventory["docs"]
    assert isinstance(corpus, dict)
    assert isinstance(docs, dict)

    counts = inventory["top_level_tracked_file_counts"]
    assert isinstance(counts, dict)
    count_lines = "\n".join(
        f"| `{owner}` | {count} |" for owner, count in counts.items()
    )

    ext_counts = corpus["runnable_extension_counts"]
    assert isinstance(ext_counts, dict)
    ext_lines = "\n".join(
        f"| `{extension}` | {count} |" for extension, count in ext_counts.items()
    ) or "| _(none)_ | 0 |"

    return f"""# Generated repository catalog

This file is a deterministic **navigation projection** generated by
`tools/repository_catalog.py`. Do not hand-edit it and do not treat it as a
source of technical truth.

For project identity and operating law, read `README.md` and `AGENTS.md`. For
human-oriented navigation, read the repository map in root `README.md`.

## Size by shelf

Tracked files represented here: **{inventory['tracked_file_count']}**

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


def render_json(inventory: dict[str, object]) -> str:
    return json.dumps(inventory, indent=2, sort_keys=True) + "\n"


def write_if_changed(path: pathlib.Path, content: str) -> bool:
    if path.is_file() and path.read_text(encoding="utf-8") == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo-root",
        type=pathlib.Path,
        default=None,
        help="Repository root. Defaults to git rev-parse from the current directory.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Exit nonzero when committed generated projections are stale.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="print_json",
        help="Print the JSON inventory to stdout instead of writing files.",
    )
    args = parser.parse_args()

    repo_root = (
        args.repo_root.resolve()
        if args.repo_root is not None
        else repo_root_from(pathlib.Path.cwd())
    )
    files = tracked_files(repo_root)
    inventory = build_inventory(files)
    markdown = render_markdown(inventory)
    json_text = render_json(inventory)

    if args.print_json:
        sys.stdout.write(json_text)
        return 0

    md_path = repo_root / GENERATED_MD
    json_path = repo_root / GENERATED_JSON

    if args.check:
        stale = []
        if not md_path.is_file() or md_path.read_text(encoding="utf-8") != markdown:
            stale.append(GENERATED_MD.as_posix())
        if not json_path.is_file() or json_path.read_text(encoding="utf-8") != json_text:
            stale.append(GENERATED_JSON.as_posix())
        if stale:
            print("stale repository catalog: " + ", ".join(stale), file=sys.stderr)
            return 1
        return 0

    changed = [
        path.as_posix()
        for path, content in ((md_path, markdown), (json_path, json_text))
        if write_if_changed(path, content)
    ]
    print(
        json.dumps(
            {
                "tracked_files": len(files),
                "corpus_sets": inventory["corpus"]["set_count"],
                "changed": changed,
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
