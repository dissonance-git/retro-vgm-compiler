#!/usr/bin/env python3
"""Audit time-bearing synthesis-resource mode transitions in VGM/VGZ logs.

This stays at exact register/device semantics. It does not infer musical parts or
instrument identity from a physical channel number.
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib

from vgm_corpus_audit import load_vgm
from vgm_fm_mode_audit import iter_register_writes


MODE_SPECS = {
    "ym2612_dac": ("YM2612", 0, 0x2B, 0x80),
    "ym2413_rhythm": ("YM2413", 0, 0x0E, 0x20),
    "ymf262_rhythm": ("YMF262", 0, 0xBD, 0x20),
}


def _new_mode_report() -> dict:
    return {
        "write_count": 0,
        "enabled_seen": False,
        "state_change_count": 0,
        "transitions": [],
    }


def audit_bytes(raw: bytes, name: str = "<memory>") -> dict:
    report = {
        "file": name,
        "ym2612_dac": _new_mode_report(),
        "ym2413_rhythm": _new_mode_report(),
        "ymf262_rhythm": _new_mode_report(),
        "errors": [],
    }
    current_state: dict[tuple[str, int], bool] = collections.defaultdict(bool)

    try:
        for write in iter_register_writes(raw):
            for mode_name, (target, port, address, enable_mask) in MODE_SPECS.items():
                if (
                    write.target != target
                    or write.port != port
                    or write.address != address
                ):
                    continue

                mode_report = report[mode_name]
                mode_report["write_count"] += 1
                enabled = bool(write.data & enable_mask)
                mode_report["enabled_seen"] |= enabled

                state_key = (mode_name, write.instance)
                previous = current_state[state_key]
                if enabled != previous:
                    mode_report["state_change_count"] += 1
                    mode_report["transitions"].append(
                        {
                            "tick": write.tick,
                            "file_offset": write.file_offset,
                            "instance": write.instance,
                            "enabled": enabled,
                            "raw_data": write.data,
                        }
                    )
                    current_state[state_key] = enabled
                break
    except ValueError as exc:
        report["errors"].append(str(exc))

    return report


def audit(path: pathlib.Path) -> dict:
    return audit_bytes(load_vgm(path), path.name)


def input_paths(inputs: list[str]) -> list[pathlib.Path]:
    result: list[pathlib.Path] = []
    for input_name in inputs:
        path = pathlib.Path(input_name)
        if path.is_dir():
            result.extend(sorted([*path.glob("*.vgm"), *path.glob("*.vgz")]))
        elif path.suffix.lower() in {".vgm", ".vgz"}:
            result.append(path)
    return result


def summarize(reports: list[dict]) -> dict:
    summary = {
        "files": len(reports),
        "files_with_errors": sum(bool(report["errors"]) for report in reports),
    }
    for mode_name in MODE_SPECS:
        summary[f"{mode_name}_files"] = sum(
            report[mode_name]["write_count"] > 0 for report in reports
        )
        summary[f"{mode_name}_enabled_files"] = sum(
            report[mode_name]["enabled_seen"] for report in reports
        )
        summary[f"{mode_name}_transition_files"] = sum(
            report[mode_name]["state_change_count"] > 0 for report in reports
        )
    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    reports = [audit(path) for path in input_paths(args.inputs)]
    aggregate = summarize(reports)

    if args.json:
        print(json.dumps({"summary": aggregate, "files": reports}, indent=2))
    elif not args.quiet:
        print(json.dumps(aggregate, indent=2))

    return 1 if aggregate["files_with_errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
