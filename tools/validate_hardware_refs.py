#!/usr/bin/env python3
"""Validate compact primary-hardware reference records.

This intentionally validates only repository invariants that do not require a
third-party JSON Schema implementation. The formal shape remains documented in
references/hardware/schema.json.
"""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REF_ROOT = ROOT / "references" / "hardware"
SOURCES_PATH = REF_ROOT / "sources.json"
DEVICES_DIR = REF_ROOT / "devices"
PLATFORMS_DIR = REF_ROOT / "platforms"


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError(f"{path}: top level must be an object")
    return value


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def validate_claims(
    path: Path,
    doc: dict,
    sources: dict[str, dict],
    claim_ids: set[str],
) -> int:
    declared_sources = doc.get("sources")
    require(isinstance(declared_sources, list) and declared_sources, f"{path}: sources must be non-empty")
    for source_id in declared_sources:
        require(source_id in sources, f"{path}: unknown declared source {source_id}")

    claims = doc.get("claims")
    require(isinstance(claims, list), f"{path}: claims must be a list")

    count = 0
    for claim in claims:
        require(isinstance(claim, dict), f"{path}: claim entries must be objects")
        claim_id = claim.get("id")
        require(isinstance(claim_id, str) and claim_id, f"{path}: claim missing id")
        require(claim_id not in claim_ids, f"duplicate claim id {claim_id}")
        claim_ids.add(claim_id)

        source_id = claim.get("source_id")
        require(source_id in sources, f"{claim_id}: unknown source {source_id}")
        require(source_id in declared_sources, f"{claim_id}: source not declared by {path.name}")

        page = claim.get("pdf_page")
        require(isinstance(page, int) and page > 0, f"{claim_id}: invalid pdf_page")
        require(page <= sources[source_id]["page_count"], f"{claim_id}: page {page} exceeds source page count")

        require(claim.get("evidence_class") == sources[source_id]["evidence_class"], f"{claim_id}: evidence class disagrees with source catalog")
        require(claim.get("claim_status") in {"documented", "derived", "uncertain"}, f"{claim_id}: invalid claim_status")
        require(claim.get("interpretation") in {"literal", "translated", "project_derived"}, f"{claim_id}: invalid interpretation")
        require(isinstance(claim.get("field"), str) and claim["field"], f"{claim_id}: missing field")
        require("value" in claim, f"{claim_id}: missing value")
        count += 1

    return count


def main() -> int:
    catalog = load_json(SOURCES_PATH)
    require(catalog.get("schema_version") == 1, "sources.json: unsupported schema_version")
    require(catalog.get("kind") == "hardware_source_catalog", "sources.json: wrong kind")

    source_list = catalog.get("sources")
    require(isinstance(source_list, list), "sources.json: sources must be a list")

    sources: dict[str, dict] = {}
    for source in source_list:
        require(isinstance(source, dict), "sources.json: source entries must be objects")
        source_id = source.get("id")
        require(isinstance(source_id, str) and source_id, "sources.json: every source needs an id")
        require(source_id not in sources, f"sources.json: duplicate source id {source_id}")
        page_count = source.get("page_count")
        require(isinstance(page_count, int) and page_count > 0, f"{source_id}: invalid page_count")
        require(source.get("repository_binary") is False, f"{source_id}: large source binary should not be committed by default")

        checksum = source.get("checksum")
        require(isinstance(checksum, dict), f"{source_id}: checksum must be an object")
        require(checksum.get("algorithm") == "sha256", f"{source_id}: checksum algorithm must be sha256")
        status = checksum.get("status")
        require(status in {"verified", "not_recorded"}, f"{source_id}: invalid checksum status")
        if status == "verified":
            value = checksum.get("value")
            require(isinstance(value, str) and len(value) == 64, f"{source_id}: verified SHA-256 must be 64 hex characters")
        else:
            require(checksum.get("value") is None, f"{source_id}: unrecorded checksum must have null value")

        sources[source_id] = source

    device_files = sorted(DEVICES_DIR.glob("*.json"))
    platform_files = sorted(PLATFORMS_DIR.glob("*.json"))
    require(device_files, "no hardware device reference files found")

    claim_ids: set[str] = set()
    claim_count = 0

    for path in device_files:
        doc = load_json(path)
        require(doc.get("schema_version") == 1, f"{path}: unsupported schema_version")
        require(doc.get("kind") == "hardware_device_reference", f"{path}: wrong kind")

        device = doc.get("device")
        require(isinstance(device, dict), f"{path}: missing device object")
        require(isinstance(device.get("id"), str) and device["id"], f"{path}: missing device id")

        claim_count += validate_claims(path, doc, sources, claim_ids)

    for path in platform_files:
        doc = load_json(path)
        require(doc.get("schema_version") == 1, f"{path}: unsupported schema_version")
        require(doc.get("kind") == "hardware_platform_reference", f"{path}: wrong kind")

        platform = doc.get("platform")
        require(isinstance(platform, dict), f"{path}: missing platform object")
        require(isinstance(platform.get("id"), str) and platform["id"], f"{path}: missing platform id")

        claim_count += validate_claims(path, doc, sources, claim_ids)

    print(
        f"validated {len(sources)} source(s), "
        f"{len(device_files)} device reference(s), "
        f"{len(platform_files)} platform reference(s), "
        f"{claim_count} claim(s)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
