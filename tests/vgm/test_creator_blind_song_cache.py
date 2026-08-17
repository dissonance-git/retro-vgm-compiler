from __future__ import annotations

import importlib.util
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools" / "creator_blind_song_cache.py"
spec = importlib.util.spec_from_file_location("creator_blind_song_cache", MODULE_PATH)
assert spec and spec.loader
cache = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = cache
spec.loader.exec_module(cache)


def synthetic_vgm() -> bytes:
    raw = bytearray(0x40)
    raw[:4] = b"Vgm "
    struct.pack_into("<I", raw, 8, 0x150)
    struct.pack_into("<I", raw, 0x34, 0)
    raw += bytes([
        0x52, 0xA0, 0x00,
        0x52, 0xA4, 0x22,
        0x52, 0xB0, 0x05,
        0x52, 0xB4, 0xC3,
        0x52, 0x28, 0xF0,
        0x61, 0x64, 0x00,
        0x50, 0x90,
        0x52, 0xA0, 0x20,
        0x52, 0x28, 0xF0,
        0x66,
    ])
    return bytes(raw)


def test_capsule_is_creator_blind_and_preserves_reusable_events(tmp_path):
    source = tmp_path / "track.vgm"
    source.write_bytes(synthetic_vgm())
    capsule = cache.extract_capsule(source, corpus_id="synthetic")

    assert capsule["label_policy"].startswith("Creator, composer")
    assert "creator" not in capsule
    assert capsule["ym2612"]["ordinary_full_key_ons"] == 2
    assert capsule["ym2612"]["events"]["tick"] == [0, 100]
    assert capsule["ym2612"]["channels"][0]["interval_tokens"] == ["1"]
    assert capsule["psg"]["values"] == [0x90]
    assert capsule["ym2612"]["events"]["algorithm"] == [5, 5]
    assert capsule["ym2612"]["events"]["pan"] == [3, 3]
    assert "sha256" not in json.dumps(capsule).lower()


def test_second_build_reuses_capsule_without_parsing_source_again(tmp_path, monkeypatch):
    source = tmp_path / "track.vgm"
    source.write_bytes(synthetic_vgm())
    root = tmp_path / "cache"
    destination, changed, _ = cache.build_one(source, corpus_id="synthetic", cache_root=root)
    assert changed is True
    assert destination.is_file()

    def explode(*args, **kwargs):
        raise AssertionError("source should not be reparsed")

    monkeypatch.setattr(cache, "extract_capsule", explode)
    _, changed, capsule = cache.build_one(source, corpus_id="synthetic", cache_root=root)
    assert changed is False
    assert capsule["ym2612"]["ordinary_full_key_ons"] == 2


def test_creator_selection_is_separate_and_role_scoped():
    records = [
        {"fixture_path": "a.vgm", "creator": "A", "role": "composer", "status": "exact"},
        {"fixture_path": "b.vgm", "creator": "A", "role": "arranger_programmer", "status": "exact"},
        {"fixture_path": "c.vgm", "creator": "A", "role": "composer", "status": "conflict"},
        {"fixture_path": "d.vgm", "creator": "B", "role": "composer", "status": "exact"},
    ]
    selected = cache.selected_credits(
        records, creator="A", role="composer", admitted_statuses={"exact"}
    )
    assert [item["fixture_path"] for item in selected] == ["a.vgm"]


def test_source_size_change_invalidates_cache_without_hashing(tmp_path):
    source = tmp_path / "track.vgm"
    source.write_bytes(synthetic_vgm())
    root = tmp_path / "cache"
    destination, changed, _ = cache.build_one(source, corpus_id="synthetic", cache_root=root)
    assert changed is True

    data = json.loads(destination.read_text())
    data["source"]["size_bytes"] += 1
    destination.write_text(json.dumps(data))
    assert cache._cache_current(destination, source) is False
