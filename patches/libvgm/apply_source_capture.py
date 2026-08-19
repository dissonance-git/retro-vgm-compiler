#!/usr/bin/env python3
"""Apply guarded libvgm changes required by source-aware VGM rendering."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


def run(script: Path, libvgm_root: Path) -> None:
    completed = subprocess.run(
        [sys.executable, str(script), str(libvgm_root)],
        cwd=str(libvgm_root),
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{script.name} failed with exit code {completed.returncode}")


def apply_exact_hunks(patch: Path, libvgm_root: Path) -> None:
    """Apply unified-diff hunks by exact old-text identity, never fuzzy context."""
    current_path: Path | None = None
    old_lines: list[str] | None = None
    new_lines: list[str] | None = None
    hunks: list[tuple[Path, str, str]] = []

    def flush() -> None:
        nonlocal old_lines, new_lines
        if current_path is not None and old_lines is not None and new_lines is not None:
            hunks.append((current_path, "".join(old_lines), "".join(new_lines)))
        old_lines = None
        new_lines = None

    for line in patch.read_text(encoding="utf-8").splitlines(keepends=True):
        if line.startswith("diff --git "):
            flush()
            current_path = None
            continue
        if line.startswith("+++ b/"):
            current_path = Path(line[6:].rstrip("\r\n"))
            continue
        if line.startswith("@@ "):
            flush()
            if current_path is None:
                raise RuntimeError(f"{patch.name}: hunk before target path")
            old_lines = []
            new_lines = []
            continue
        if old_lines is None or new_lines is None:
            continue
        if line.startswith("\\ No newline at end of file"):
            continue
        if line.startswith(" "):
            old_lines.append(line[1:])
            new_lines.append(line[1:])
        elif line.startswith("-"):
            old_lines.append(line[1:])
        elif line.startswith("+"):
            new_lines.append(line[1:])
    flush()

    if not hunks:
        raise RuntimeError(f"{patch.name}: no hunks parsed")

    by_path: dict[Path, list[tuple[str, str]]] = {}
    for rel, old, new in hunks:
        by_path.setdefault(rel, []).append((old, new))

    for rel, file_hunks in by_path.items():
        target = libvgm_root / rel
        text = target.read_text(encoding="utf-8")
        for index, (old, new) in enumerate(file_hunks, start=1):
            count = text.count(old)
            if count == 1:
                text = text.replace(old, new, 1)
                continue
            if count == 0 and text.count(new) == 1:
                continue
            raise RuntimeError(
                f"{patch.name} hunk {index} for {rel} is not exact: "
                f"old matches={count}, new matches={text.count(new)}"
            )
        target.write_text(text, encoding="utf-8", newline="\n")


def apply_numbered_patch(patch: Path, libvgm_root: Path) -> None:
    # Prefer git's strict checker when the historical patch metadata still
    # agrees. If its generated hunk metadata has drifted, fall back to exact
    # old-text replacement. The fallback has no fuzz factor: each complete hunk
    # must identify exactly one source region (or exactly one already-applied
    # region), so genuine upstream drift still fails closed.
    check = subprocess.run(
        ["git", "apply", "--recount", "--check", str(patch)],
        cwd=str(libvgm_root),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if check.returncode == 0:
        applied = subprocess.run(
            ["git", "apply", "--recount", str(patch)],
            cwd=str(libvgm_root),
            check=False,
        )
        if applied.returncode != 0:
            raise RuntimeError(f"{patch.name} passed --check but failed to apply")
        return

    reverse = subprocess.run(
        ["git", "apply", "--recount", "--reverse", "--check", str(patch)],
        cwd=str(libvgm_root),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if reverse.returncode == 0:
        return

    apply_exact_hunks(patch, libvgm_root)
    print(f"applied {patch.name} through exact hunk identity")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    root = args.libvgm_root.resolve()
    here = Path(__file__).resolve().parent
    repo_root = here.parents[1]

    run(repo_root / "tools" / "libvgm_apply_realtime_command_observer.py", root)

    # These observational ABIs are prerequisites for source-native DAC
    # enhancement. 0003 exposes original PCM-bank identity and authored stream
    # frequency; 0004/0005 preserve refreshed-bank and length semantics.
    for name in (
        "0002-resolved-ym2612-dac-observer.patch",
        "0003-dac-stream-source-observer.patch",
        "0004-refresh-dac-stream-pcm-bank.patch",
        "0005-fix-dac-stream-millisecond-length.patch",
    ):
        apply_numbered_patch(here / name, root)

    run(here / "apply_source_resampler_hooks.py", root)
    run(here / "apply_sn76496_source_tap.py", root)
    run(here / "apply_ym2151_source_tap.py", root)
    run(here / "apply_playera_gain_view.py", root)
    run(here / "apply_playera_postrender_hook.py", root)
    run(here / "apply_playera_deferred_postrender.py", root)
    print("libvgm source-aware VGM capture/replacement patches applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
