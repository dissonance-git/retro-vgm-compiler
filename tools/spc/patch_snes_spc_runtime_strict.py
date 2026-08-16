#!/usr/bin/env python3
"""Strict driver for the pinned snes_spc runtime transformer.

Each edit independently admits exactly one pristine sentinel or exactly one
already-patched replacement. Partial patch state is an error even if another
edit in the same file already inserted the global marker.
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys


PATCHER_PATH = Path(__file__).with_name("patch_snes_spc_runtime.py")
SPEC = importlib.util.spec_from_file_location("patch_snes_spc_runtime", PATCHER_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {PATCHER_PATH}")
patcher = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = patcher
SPEC.loader.exec_module(patcher)


def strict_replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    pristine_count = text.count(old)
    patched_count = text.count(new)

    if pristine_count == 1 and patched_count == 0:
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
        return
    if pristine_count == 0 and patched_count == 1:
        return

    raise RuntimeError(
        f"{path}: {label!r} expected exactly one pristine sentinel or one "
        f"exact patched replacement; pristine={pristine_count}, "
        f"patched={patched_count}. Refusing partial/foreign snes_spc source."
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    args = parser.parse_args()

    patcher.replace_once = strict_replace_once
    patcher.patch(args.source_root.resolve())


if __name__ == "__main__":
    main()
