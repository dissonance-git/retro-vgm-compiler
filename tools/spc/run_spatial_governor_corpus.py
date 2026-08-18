#!/usr/bin/env python3
"""Execute the frozen SPC half of realtime-spatial-governor-corpus-001.

This command is outer experiment routing only. It verifies immutable corpus bytes,
passes only the exact fixture SHA-256 and SPC bytes to the creator-blind runtime,
and stores each trace sidecar under its hash. Titles, game names, creator tags,
ID666 text and corpus labels never enter the runtime control path.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
PREREG = ROOT / "research" / "validation" / "realtime-spatial-governor-corpus-preregistration.json"
AMENDMENT_ROUTE = ROOT / "research" / "validation" / "realtime-spatial-governor-corpus-001-amendment-001.json"
AMENDMENT_HORIZON = ROOT / "research" / "validation" / "realtime-spatial-governor-corpus-001-amendment-002.json"
CORPUS_ROOT = ROOT / "tests" / "corpus"
SUMMARY_NAME = "spc-governor-corpus-summary.json"


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise RuntimeError(f"{path}: expected JSON object")
    return value


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_sha_inventory(path: Path) -> list[tuple[str, str]]:
    entries: list[tuple[str, str]] = []
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if "  " not in line:
            raise RuntimeError(f"{path}:{line_number}: expected SHA-256 + two-space filename")
        digest, filename = line.split("  ", 1)
        digest = digest.strip().lower()
        filename = filename.strip()
        if len(digest) != 64 or any(ch not in "0123456789abcdef" for ch in digest):
            raise RuntimeError(f"{path}:{line_number}: invalid SHA-256")
        if not filename or Path(filename).is_absolute() or ".." in Path(filename).parts:
            raise RuntimeError(f"{path}:{line_number}: unsafe fixture path")
        entries.append((digest, filename))
    return entries


def frozen_spc_corpora(prereg: dict[str, Any]) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    for entry in prereg.get("frozen_corpora", []):
        if not isinstance(entry, dict) or entry.get("source_family") != "SPC":
            continue
        corpus_id = entry.get("corpus_id")
        fixture_count = entry.get("fixture_count")
        if not isinstance(corpus_id, str) or not isinstance(fixture_count, int):
            raise RuntimeError("preregistration contains malformed SPC corpus entry")
        result.append((corpus_id, fixture_count))
    if not result:
        raise RuntimeError("preregistration contains no frozen SPC corpora")
    return result


def validate_contracts(
    prereg: dict[str, Any],
    route_amendment: dict[str, Any],
    horizon_amendment: dict[str, Any],
) -> int:
    experiment_id = prereg.get("experiment_id")
    if experiment_id != "realtime-spatial-governor-corpus-001":
        raise RuntimeError("unexpected governor experiment id")
    if route_amendment.get("experiment_id") != experiment_id:
        raise RuntimeError("route amendment belongs to another experiment")
    if horizon_amendment.get("experiment_id") != experiment_id:
        raise RuntimeError("horizon amendment belongs to another experiment")
    execution = horizon_amendment.get("spc_execution")
    if not isinstance(execution, dict):
        raise RuntimeError("horizon amendment lacks spc_execution")
    seconds = execution.get("seconds_per_fixture")
    if seconds != 24:
        raise RuntimeError("frozen SPC execution horizon is not 24 seconds")
    if execution.get("native_sample_rate_hz") != 32000:
        raise RuntimeError("frozen SPC native rate is not 32000 Hz")
    if execution.get("governor_window_frames") != 2048:
        raise RuntimeError("frozen SPC governor window is not 2048 frames")
    totals = prereg.get("fixture_totals")
    if not isinstance(totals, dict) or totals.get("spc") != 176:
        raise RuntimeError("frozen SPC fixture total is not 176")
    return seconds


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--runner",
        type=Path,
        required=True,
        help="built spc_spatial_governor_trace executable",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help="directory for SHA-keyed creator-blind trace sidecars",
    )
    args = parser.parse_args()

    prereg = load_json(PREREG)
    route_amendment = load_json(AMENDMENT_ROUTE)
    horizon_amendment = load_json(AMENDMENT_HORIZON)
    seconds = validate_contracts(prereg, route_amendment, horizon_amendment)
    corpora = frozen_spc_corpora(prereg)

    runner = args.runner.resolve()
    if not runner.is_file():
        raise RuntimeError(f"governor runner does not exist: {runner}")
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    fixture_rows: list[dict[str, Any]] = []
    total_expected = 0
    total_traces = 0
    seen_hashes: set[str] = set()

    for corpus_id, expected_count in corpora:
        inventory_path = CORPUS_ROOT / f"{corpus_id}.sha256"
        corpus_dir = CORPUS_ROOT / corpus_id
        if not inventory_path.is_file() or not corpus_dir.is_dir():
            raise RuntimeError(f"missing frozen corpus storage for {corpus_id}")
        inventory = parse_sha_inventory(inventory_path)
        if len(inventory) != expected_count:
            raise RuntimeError(
                f"{corpus_id}: frozen count {expected_count}, inventory has {len(inventory)}"
            )
        total_expected += expected_count

        for expected_sha, filename in inventory:
            fixture = corpus_dir / filename
            if not fixture.is_file():
                raise RuntimeError(f"{corpus_id}: missing fixture {filename}")
            actual_sha = sha256_file(fixture)
            if actual_sha != expected_sha:
                raise RuntimeError(
                    f"{corpus_id}: SHA-256 mismatch for immutable fixture {filename}"
                )
            if actual_sha in seen_hashes:
                raise RuntimeError(
                    f"duplicate fixture SHA-256 across frozen SPC surface: {actual_sha}"
                )
            seen_hashes.add(actual_sha)

            sidecar = output_dir / f"{actual_sha}.json"
            subprocess.run(
                [
                    str(runner),
                    str(fixture),
                    str(sidecar),
                    actual_sha,
                    str(seconds),
                ],
                check=True,
            )

            payload = load_json(sidecar)
            if payload.get("fixture_sha256") != actual_sha:
                raise RuntimeError(f"{actual_sha}: runner sidecar identity mismatch")
            execution = payload.get("execution")
            summary = payload.get("summary")
            if not isinstance(execution, dict) or execution.get("seconds") != seconds:
                raise RuntimeError(f"{actual_sha}: runner used wrong playback horizon")
            if not isinstance(summary, dict) or summary.get("all_traces_admitted") is not True:
                raise RuntimeError(f"{actual_sha}: runner did not admit every trace")
            trace_count = summary.get("trace_count")
            if not isinstance(trace_count, int) or trace_count <= 0:
                raise RuntimeError(f"{actual_sha}: invalid trace count")
            traces = payload.get("traces")
            if not isinstance(traces, list) or len(traces) != trace_count:
                raise RuntimeError(f"{actual_sha}: trace array/count mismatch")
            for trace in traces:
                if not isinstance(trace, dict) or trace.get("fixture_sha256") != actual_sha:
                    raise RuntimeError(f"{actual_sha}: per-block fixture identity mismatch")
                validation = trace.get("validation")
                if not isinstance(validation, dict) or validation.get("valid") is not True:
                    raise RuntimeError(f"{actual_sha}: invalid admitted block found in sidecar")
                continuity = trace.get("continuity")
                if continuity is not None and (
                    not isinstance(continuity, dict) or continuity.get("valid") is not True
                ):
                    raise RuntimeError(f"{actual_sha}: continuity failure found in sidecar")

            total_traces += trace_count
            fixture_rows.append(
                {
                    "fixture_sha256": actual_sha,
                    "trace_sidecar": sidecar.name,
                    "trace_count": trace_count,
                }
            )

    if total_expected != 176 or len(fixture_rows) != 176:
        raise RuntimeError(
            f"frozen SPC surface expected 176 fixtures, executed {len(fixture_rows)}"
        )

    summary = {
        "experiment_id": prereg["experiment_id"],
        "amendments": [
            route_amendment["amendment_id"],
            horizon_amendment["amendment_id"],
        ],
        "claim_boundary": (
            "Outer routing/provenance only. Runtime trace sidecars are keyed by immutable "
            "SHA-256; creator/game/track/genre labels are not runtime inputs."
        ),
        "seconds_per_fixture": seconds,
        "fixture_count": len(fixture_rows),
        "trace_count": total_traces,
        "all_fixtures_executed": True,
        "all_sidecars_admitted": True,
        "fixtures": fixture_rows,
    }
    summary_path = output_dir / SUMMARY_NAME
    summary_path.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(summary_path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"run_spatial_governor_corpus: {error}", file=sys.stderr)
        raise SystemExit(1)
