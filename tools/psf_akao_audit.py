#!/usr/bin/env python3
"""Audit reconstructed PSF1 memory for bounded AKAO structural candidates.

This tool performs no CPU/SPU execution and no full AKAO event decoding.  It
uses structural invariants only, so accepted objects remain candidates rather
than proof of exact driver version or runtime use.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from components.psf.akao_probe import scan_psf1_akao
from components.psf.psf1 import build_psf1_effective_image
from components.xsf.envelope import resolve_xsf


PSF_SUFFIXES = {".psf", ".minipsf", ".psf1", ".minipsf1"}


def audit_psf(path: Path) -> dict:
    resolved = resolve_xsf(path, expected_version=0x01)
    image = build_psf1_effective_image(resolved)
    assessments = scan_psf1_akao(image)
    accepted = [item for item in assessments if item.accepted]
    return {
        "file": path.name,
        "memory_base": image.memory_base,
        "memory_bytes": len(image.memory),
        "runtime_available": image.runtime_available,
        "akao_signature_count": len(assessments),
        "accepted_sequence_candidates": [
            {
                "address": item.sequence.address,
                "declared_length": item.sequence.declared_length,
                "sequence_id": item.sequence.sequence_id,
                "sample_set_id": item.sequence.sample_set_id,
                "track_count": item.sequence.track_count,
                "track_addresses": item.sequence.track_addresses,
                "instrument_address": item.sequence.instrument_address,
                "drumkit_address": item.sequence.drumkit_address,
                "raw_fe13_pairs": item.sequence.raw_fe13_pairs,
                "warnings": item.sequence.warnings,
                "version_evidence": item.sequence.version_evidence,
            }
            for item in accepted
            if item.sequence is not None
        ],
        "rejected_or_nonsequence": [
            {
                "address": item.address,
                "classification": item.classification,
                "reasons": item.reasons,
            }
            for item in assessments
            if not item.accepted
        ],
    }


def audit_path(path: Path) -> dict:
    path = path.resolve()
    if path.is_file():
        paths = [path]
    else:
        paths = sorted(
            (
                item
                for item in path.rglob("*")
                if item.is_file() and item.suffix.lower() in PSF_SUFFIXES
            ),
            key=lambda item: item.relative_to(path).as_posix().casefold(),
        )
    if not paths:
        raise ValueError(f"no PSF1 roots under {path}")
    reports = [audit_psf(item) for item in paths]
    return {
        "schema": "psf-akao-structural-audit-1",
        "scope": str(path),
        "root_count": len(reports),
        "accepted_candidate_count": sum(
            len(report["accepted_sequence_candidates"]) for report in reports
        ),
        "reports": reports,
        "claim_boundary": {
            "structural_candidate": True,
            "exact_akao_version_proven": False,
            "event_stream_decoded": False,
            "runtime_use_proven": False,
            "spu_correspondence_proven": False,
            "raw_fe13_is_decoded_event": False,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    result = audit_path(args.path)
    if not args.quiet:
        print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
