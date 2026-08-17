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


def test_build_all_dispatches_each_admitted_composer_once(tmp_path, monkeypatch):
    credits = tmp_path / "credits.jsonl"
    records = [
        {"fixture_path": "a.vgm", "creator": "B", "role": "composer", "status": "exact"},
        {"fixture_path": "b.vgm", "creator": "A", "role": "composer", "status": "exact"},
        {"fixture_path": "c.vgm", "creator": "A", "role": "composer", "status": "conflict"},
    ]
    credits.write_text("\n".join(json.dumps(record) for record in records) + "\n")

    calls = []

    def fake_build_creator(**kwargs):
        calls.append(kwargs["creator"])
        return {
            "creator": kwargs["creator"],
            "role": kwargs["role"],
            "selected_tracks": 1,
            "built": 1,
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

    assert calls == ["A", "B"]
    assert result["creator_count"] == 2
    assert result["selected_tracks"] == 2
    assert result["built"] == 2
    assert result["reused"] == 0


def test_sonic3_credit_index_covers_all_admitted_composer_controls():
    records = all_caches.read_jsonl(
        ROOT / "research" / "projects" / "sonic3" / "role-credit-index.jsonl"
    )
    counts = collections.Counter(
        record["creator"]
        for record in records
        if record.get("role") == "composer"
        and record.get("status") in all_caches.DEFAULT_ADMITTED_STATUSES
    )
    assert counts == {
        "Tatsuyuki Maeda": 26,
        "Jun Senoue": 12,
        "Haruyo Oguro": 5,
        "Naofumi Hataya": 4,
        "Tomonori Sawada": 1,
        "Masaru Setsumaru": 1,
        "Seirou Okamoto": 1,
    }
    assert sum(counts.values()) == 50
    assert not any("Scorching Sand" in str(record.get("fixture_path", "")) for record in records)
