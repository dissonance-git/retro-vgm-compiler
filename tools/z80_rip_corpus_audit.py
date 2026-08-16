#!/usr/bin/env python3
"""Audit preserved KSS/SGC containers and their extended-M3U subsong routes.

This is an admission audit, not an emulator. It establishes the observed
container signature and whether every retained playlist route names a retained
container with a unique integer subsong selector. It does not establish correct
execution, playback, timing, device behavior, or authorship.
"""

from __future__ import annotations

import argparse
import json
import pathlib


CONTAINER_SUFFIXES = {".kss", ".sgc"}


def decode_playlist(path: pathlib.Path) -> str:
    raw = path.read_bytes()
    try:
        return raw.decode("utf-8-sig")
    except UnicodeDecodeError:
        return raw.decode("cp1252")


def audit_container(path: pathlib.Path) -> dict[str, object]:
    raw = path.read_bytes()
    errors: list[str] = []
    format_name = path.suffix.lower().lstrip(".").upper()
    if path.suffix.lower() == ".sgc":
        if len(raw) < 0xA0:
            errors.append("file too small for observed SGC header")
        if raw[:4] != b"SGC\x1a":
            errors.append("missing SGC signature")
        if len(raw) >= 5 and raw[4] != 1:
            errors.append(f"unsupported SGC version {raw[4]}")
    elif path.suffix.lower() == ".kss":
        if len(raw) < 16:
            errors.append("file too small for KSS header")
        if raw[:4] not in {b"KSCC", b"KSSX"}:
            errors.append("missing KSCC/KSSX signature")
    else:
        errors.append(f"unsupported extension {path.suffix}")
    return {
        "file": path.name,
        "format": format_name,
        "size": len(raw),
        "signature": raw[:4].hex(),
        "valid": not errors,
        "errors": errors,
    }


def playlist_route(path: pathlib.Path) -> tuple[str, int]:
    lines = [line.strip() for line in decode_playlist(path).splitlines()]
    entries = [line for line in lines if line and not line.startswith("#")]
    if len(entries) != 1:
        raise ValueError(f"expected one extended-M3U entry, found {len(entries)}")
    prefix, marker, fields = entries[0].partition("::KSS,")
    if not marker:
        raise ValueError("missing ::KSS subsong route")
    selector_text = fields.split(",", 1)[0].strip()
    try:
        selector = int(selector_text, 10)
    except ValueError as exc:
        raise ValueError(f"invalid subsong selector {selector_text!r}") from exc
    return prefix, selector


def audit_directory(directory: pathlib.Path) -> dict[str, object]:
    containers = sorted(
        (path for path in directory.rglob("*") if path.is_file() and path.suffix.lower() in CONTAINER_SUFFIXES),
        key=lambda path: path.relative_to(directory).as_posix().casefold(),
    )
    playlists = sorted(
        directory.rglob("*.m3u"),
        key=lambda path: path.relative_to(directory).as_posix().casefold(),
    )
    errors: list[str] = []
    reports = [audit_container(path) for path in containers]
    if not containers:
        errors.append("no KSS/SGC container found")
    if any(not report["valid"] for report in reports):
        errors.append("one or more containers failed admission")

    routes: list[dict[str, object]] = []
    seen: set[tuple[str, int]] = set()
    for playlist in playlists:
        try:
            target_name, selector = playlist_route(playlist)
            target = playlist.parent / target_name
            if not target.is_file() or target.suffix.lower() not in CONTAINER_SUFFIXES:
                raise ValueError(f"route target is not a retained KSS/SGC container: {target_name}")
            key = (target.relative_to(directory).as_posix().casefold(), selector)
            if key in seen:
                raise ValueError(f"duplicate subsong route {target_name} selector {selector}")
            seen.add(key)
            routes.append(
                {
                    "playlist": playlist.relative_to(directory).as_posix(),
                    "container": target.relative_to(directory).as_posix(),
                    "selector": selector,
                }
            )
        except ValueError as exc:
            errors.append(f"{playlist.relative_to(directory).as_posix()}: {exc}")

    return {
        "directory": directory.as_posix(),
        "valid": not errors,
        "container_count": len(containers),
        "playlist_route_count": len(routes),
        "containers": reports,
        "routes": routes,
        "errors": errors,
        "boundary": "container and playlist-route admission only; runtime and playback not executed",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directories", nargs="+", type=pathlib.Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    reports = [audit_directory(path) for path in args.directories]
    if args.json:
        print(json.dumps(reports, indent=2))
    else:
        for report in reports:
            print(
                f"{report['directory']}: containers={report['container_count']} "
                f"playlist_routes={report['playlist_route_count']} errors={len(report['errors'])}"
            )
    return 1 if any(not report["valid"] for report in reports) else 0


if __name__ == "__main__":
    raise SystemExit(main())
