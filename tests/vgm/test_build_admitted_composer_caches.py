from __future__ import annotations

import collections
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import build_admitted_composer_caches as all_caches


def test_admitted_creators_filters_roles_and_conflicts():
    records = [
        {"creator": "B", "role": "composer", "status": "exact"},
        {"creator": "A", "role": "composer", "status": "derived"},
        {"creator": "C", "role": "composer", "status": "conflict"},
        {"creator": "D", "role": "arranger_programmer", "status": "exact"},
    ]
    assert all_caches.admitted_creators(records) == ["A", "B"]


def test_build_all_routes_only_backend_compatible_fixtures(tmp_path, monkeypatch):
    credits = tmp_path / "credits.jsonl"
    records = [
        {"fixture_path": "a.vgm", "creator": "A", "role": "composer", "status": "exact"},
        {"fixture_path": "b.spc", "creator": "A", "role": "composer", "status": "exact"},
        {"fixture_path": "c.spc", "creator": "B", "role": "composer", "status": "exact"},
        {"fixture_path": "ignored.vgm", "creator": "C", "role": "composer", "status": "conflict"},
    ]
    credits.write_text("\n".join(json.dumps(record) for record in records) + "\n")

    calls = []

    def fake_build_creator(**kwargs):
        filtered = all_caches.read_jsonl(kwargs["credit_index"])
        calls.append((kwargs["creator"], [record["fixture_path"] for record in filtered]))
        return {
            "creator": kwargs["creator"],
            "role": kwargs["role"],
            "selected_tracks": len(filtered),
            "built": len(filtered),
            "reused": 0,
        }

    monkeypatch.setattr(all_caches, "build_creator", fake_build_creator)
    result = all_caches.build_all(
        credit_index=credits,
        role="composer",
        repo_root=tmp_path,
        cache_root=tmp_path / "cache",
        cache_index=tmp_path / "cache" / "index.jsonl",
        refresh=False,
        admitted_statuses={"exact"},
    )

    assert calls == [("A", ["a.vgm"])]
    assert result["creator_count"] == 2
    assert result["role_selected_tracks"] == 3
    assert result["cacheable_tracks"] == 1
    assert result["unsupported_tracks"] == 2
    assert result["selected_tracks"] == 1
    assert result["built"] == 1
    assert result["reused"] == 0

    by_creator = {item["creator"]: item for item in result["results"]}
    assert by_creator["A"]["role_selected_tracks"] == 2
    assert by_creator["A"]["cacheable_tracks"] == 1
    assert by_creator["A"]["unsupported_fixtures"] == ["b.spc"]
    assert by_creator["B"]["role_selected_tracks"] == 1
    assert by_creator["B"]["cacheable_tracks"] == 0
    assert by_creator["B"]["unsupported_fixtures"] == ["c.spc"]


def test_sonic3_credit_index_covers_all_admitted_composer_controls():
    records = all_caches.read_jsonl(
        ROOT / "research" / "projects" / "sonic3" / "role-credit-index.jsonl"
    )
    admitted = [
        record
        for record in records
        if record.get("role") == "composer"
        and record.get("status") in all_caches.DEFAULT_ADMITTED_STATUSES
    ]
    counts = collections.Counter(record["creator"] for record in admitted)
    assert counts == {
        "Tatsuyuki Maeda": 26,
        "Jun Senoue": 12,
        "Haruyo Oguro": 5,
        "Naofumi Hataya": 4,
        "Tomonori Sawada": 2,
        "Masaru Setsumaru": 1,
        "Seirou Okamoto": 1,
        "Miyoko Takaoka": 19,
        "Masanori Hikichi": 9,
    }
    assert sum(counts.values()) == 79

    cacheable = [record for record in admitted if all_caches.cacheable_by_current_backend(record)]
    unsupported = [record for record in admitted if not all_caches.cacheable_by_current_backend(record)]
    assert len(cacheable) == 51
    assert len(unsupported) == 28
    assert {pathlib.PurePosixPath(record["fixture_path"]).suffix for record in unsupported} == {".spc"}

    scorching = [
        record for record in admitted
        if "Scorching Sand" in str(record.get("fixture_path", ""))
    ]
    assert len(scorching) == 1
    assert scorching[0]["creator"] == "Tomonori Sawada"
    assert scorching[0]["status"] == "derived"
    assert "counterevidence" in scorching[0]


def test_cube_controls_are_external_terranigma_only():
    records = all_caches.read_jsonl(
        ROOT / "research" / "projects" / "sonic3" / "role-credit-index.jsonl"
    )
    cube = [
        record for record in records
        if record.get("creator") in {"Miyoko Takaoka", "Masanori Hikichi"}
    ]
    assert len(cube) == 28
    assert {record["corpus_id"] for record in cube} == {"terranigma-spc"}
    assert all(
        record["source_policy"]
        == "research/projects/sonic3/cube-composer-control-policy.json"
        for record in cube
    )
    assert not any("sonic-3-and-knuckles" in record["fixture_path"] for record in cube)
