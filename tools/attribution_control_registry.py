#!/usr/bin/env python3
"""Build an evidence-safe attribution-control registry.

External artist tags are useful for locating candidate controls in the permanent
corpus, but they are not authorship proof. This tool therefore keeps two inputs
strictly separate:

1. locator metadata, such as the Helix/foobar external-tag inventory;
2. explicit admissions backed by independent role-specific evidence.

A corpus object becomes an attribution control only when a separate admission
names the exact fixture, creator, role, evidence status, confidence, and source.
"""

from __future__ import annotations

import argparse
import json
import pathlib
from dataclasses import asdict, dataclass
from typing import Iterable


VALID_ROLES = {
    "composer",
    "arranger_programmer",
    "driver_toolchain",
    "patch_sample_designer",
}
VALID_STATUSES = {"exact", "derived", "hypothesis"}


@dataclass(frozen=True)
class Locator:
    candidate: str
    fixture_path: str
    corpus_id: str
    platform: str
    external_artist: str
    attribution_limit: str
    authority: str


@dataclass(frozen=True)
class Admission:
    candidate: str
    fixture_path: str
    role: str
    status: str
    confidence: float
    source: str
    work_family_id: str = ""
    implementation_family_id: str = ""
    detail: str = ""


def _read_jsonl(path: pathlib.Path) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    with path.open("r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, start=1):
            stripped = line.strip()
            if not stripped:
                continue
            try:
                value = json.loads(stripped)
            except json.JSONDecodeError as exc:
                raise ValueError(f"{path}:{line_number}: invalid JSON: {exc}") from exc
            if not isinstance(value, dict):
                raise ValueError(f"{path}:{line_number}: JSONL record must be an object")
            records.append(value)
    return records


def _require_text(record: dict[str, object], field: str, context: str) -> str:
    value = record.get(field)
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{context}: requires non-empty {field}")
    return value.strip()


def locators_from_external_tags(
    records: Iterable[dict[str, object]],
    people: set[str] | None = None,
) -> list[Locator]:
    locators: list[Locator] = []
    seen: set[tuple[str, str]] = set()

    for index, record in enumerate(records, start=1):
        context = f"external-tag record {index}"
        matches = record.get("target_control_person_match", [])
        if not isinstance(matches, list):
            raise ValueError(f"{context}: target_control_person_match must be a list")

        fixture_path = _require_text(record, "fixture_path", context)
        corpus_id = _require_text(record, "corpus_id", context)
        attribution_limit = _require_text(record, "attribution_limit", context)
        authority = _require_text(record, "authority", context)
        platform = str(record.get("external_tag_platform", ""))
        external_artist = str(record.get("external_tag_artist", ""))

        for raw_candidate in matches:
            if not isinstance(raw_candidate, str) or not raw_candidate.strip():
                continue
            candidate = raw_candidate.strip()
            if people is not None and candidate not in people:
                continue
            key = (candidate, fixture_path)
            if key in seen:
                continue
            seen.add(key)
            locators.append(
                Locator(
                    candidate=candidate,
                    fixture_path=fixture_path,
                    corpus_id=corpus_id,
                    platform=platform,
                    external_artist=external_artist,
                    attribution_limit=attribution_limit,
                    authority=authority,
                )
            )

    return sorted(locators, key=lambda item: (item.candidate, item.corpus_id, item.fixture_path))


def admissions_from_records(records: Iterable[dict[str, object]]) -> list[Admission]:
    admissions: list[Admission] = []
    seen: set[tuple[str, str, str]] = set()

    for index, record in enumerate(records, start=1):
        context = f"admission record {index}"
        candidate = _require_text(record, "candidate", context)
        fixture_path = _require_text(record, "fixture_path", context)
        role = _require_text(record, "role", context)
        status = _require_text(record, "status", context)
        source = _require_text(record, "source", context)

        if role not in VALID_ROLES:
            raise ValueError(f"{context}: unsupported role {role!r}")
        if status not in VALID_STATUSES:
            raise ValueError(f"{context}: unsupported evidence status {status!r}")

        confidence_value = record.get("confidence")
        if not isinstance(confidence_value, (int, float)):
            raise ValueError(f"{context}: confidence must be numeric")
        confidence = float(confidence_value)
        if confidence < 0.0 or confidence > 1.0:
            raise ValueError(f"{context}: confidence must be in [0, 1]")

        key = (candidate, fixture_path, role)
        if key in seen:
            raise ValueError(
                f"{context}: duplicate admission for candidate/fixture/role {key!r}"
            )
        seen.add(key)

        admissions.append(
            Admission(
                candidate=candidate,
                fixture_path=fixture_path,
                role=role,
                status=status,
                confidence=confidence,
                source=source,
                work_family_id=str(record.get("work_family_id", "")),
                implementation_family_id=str(
                    record.get("implementation_family_id", "")
                ),
                detail=str(record.get("detail", "")),
            )
        )

    return sorted(admissions, key=lambda item: (item.candidate, item.fixture_path, item.role))


def build_registry(
    locators: Iterable[Locator],
    admissions: Iterable[Admission],
) -> dict[str, object]:
    locator_list = list(locators)
    admission_list = list(admissions)
    locator_index = {(item.candidate, item.fixture_path): item for item in locator_list}

    admitted_keys: set[tuple[str, str]] = set()
    controls: list[dict[str, object]] = []
    for admission in admission_list:
        locator_key = (admission.candidate, admission.fixture_path)
        locator = locator_index.get(locator_key)
        if locator is None:
            raise ValueError(
                "admission is not backed by locator inventory: "
                f"candidate={admission.candidate!r}, fixture={admission.fixture_path!r}"
            )
        admitted_keys.add(locator_key)
        controls.append(
            {
                "candidate": admission.candidate,
                "fixture_path": admission.fixture_path,
                "corpus_id": locator.corpus_id,
                "platform": locator.platform,
                "role": admission.role,
                "status": admission.status,
                "confidence": admission.confidence,
                "source": admission.source,
                "work_family_id": admission.work_family_id,
                "implementation_family_id": admission.implementation_family_id,
                "detail": admission.detail,
                "locator_authority": locator.authority,
                "locator_attribution_limit": locator.attribution_limit,
            }
        )

    locator_only = [
        asdict(locator)
        for locator in locator_list
        if (locator.candidate, locator.fixture_path) not in admitted_keys
    ]

    return {
        "model": "evidence-safe attribution control registry",
        "claim_boundary": (
            "External artist tags are locator metadata only. No fixture becomes an "
            "attribution control until a separate exact-fixture admission supplies "
            "role-specific evidence status, confidence, and source."
        ),
        "locator_count": len(locator_list),
        "admitted_control_count": len(controls),
        "locator_only_count": len(locator_only),
        "controls": sorted(
            controls,
            key=lambda item: (
                str(item["candidate"]),
                str(item["corpus_id"]),
                str(item["fixture_path"]),
                str(item["role"]),
            ),
        ),
        "locator_only": locator_only,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("external_tags", type=pathlib.Path)
    parser.add_argument("--admissions", type=pathlib.Path)
    parser.add_argument("--person", action="append", default=[])
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    people = set(args.person) if args.person else None
    locators = locators_from_external_tags(_read_jsonl(args.external_tags), people)
    admissions = (
        admissions_from_records(_read_jsonl(args.admissions))
        if args.admissions
        else []
    )
    registry = build_registry(locators, admissions)
    text = json.dumps(registry, indent=2, sort_keys=True)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
