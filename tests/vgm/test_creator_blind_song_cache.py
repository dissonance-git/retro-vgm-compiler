from __future__ import annotations

import importlib.util
import json
import pathlib
import struct
import sys
import tempfile
import unittest
from unittest import mock

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
        0x61, 0x32, 0x00,
        0x52, 0x28, 0x70,
        0x61, 0x0A, 0x00,
        0x52, 0x28, 0x00,
        0x61, 0x28, 0x00,
        0x50, 0x90,
        0x52, 0xA0, 0x20,
        0x52, 0x28, 0xF0,
        0x66,
    ])
    return bytes(raw)


def same_tick_rearticulation_vgm() -> bytes:
    raw = bytearray(0x40)
    raw[:4] = b"Vgm "
    struct.pack_into("<I", raw, 8, 0x150)
    struct.pack_into("<I", raw, 0x34, 0)
    raw += bytes([
        0x52, 0xA0, 0x00,
        0x52, 0xA4, 0x22,
        0x52, 0x28, 0xF0,
        0x61, 0x64, 0x00,
        0x52, 0x28, 0x00,
        0x52, 0xA0, 0x20,
        0x52, 0x28, 0xF0,
        0x66,
    ])
    return bytes(raw)


class CreatorBlindSongCacheTests(unittest.TestCase):
    def test_capsule_is_creator_blind_and_preserves_reusable_events(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = pathlib.Path(temp_dir) / "track.vgm"
            source.write_bytes(synthetic_vgm())
            capsule = cache.extract_capsule(source, corpus_id="synthetic")

        self.assertTrue(capsule["label_policy"].startswith("Creator, composer"))
        self.assertNotIn("creator", capsule)
        self.assertEqual(capsule["ym2612"]["ordinary_full_key_ons"], 2)
        self.assertEqual(capsule["ym2612"]["events"]["tick"], [0, 100])
        self.assertEqual(capsule["ym2612"]["events"]["key_gate_event_index"], [0, 3])
        self.assertEqual(capsule["ym2612"]["channels"][0]["interval_tokens"], ["1"])
        self.assertEqual(capsule["psg"]["values"], [0x90])
        self.assertEqual(capsule["ym2612"]["events"]["algorithm"], [5, 5])
        self.assertEqual(capsule["ym2612"]["events"]["pan"], [3, 3])
        self.assertEqual(
            capsule["ym2612"]["key_gate_events"],
            {
                "tick": [0, 50, 60, 100],
                "channel": [0, 0, 0, 0],
                "operator_mask": [0xF0, 0x70, 0x00, 0xF0],
            },
        )
        self.assertEqual(capsule["realization_counters"]["fm_key_gate_writes"], 4)
        self.assertNotIn("sha256", json.dumps(capsule).lower())

    def test_partial_and_off_gate_writes_do_not_become_onsets(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = pathlib.Path(temp_dir) / "track.vgm"
            source.write_bytes(synthetic_vgm())
            capsule = cache.extract_capsule(source, corpus_id="synthetic")

        self.assertEqual(capsule["ym2612"]["ordinary_full_key_ons"], 2)
        self.assertEqual(capsule["ym2612"]["events"]["tick"], [0, 100])
        self.assertEqual(capsule["ym2612"]["key_gate_events"]["operator_mask"], [0xF0, 0x70, 0x00, 0xF0])

    def test_same_tick_off_then_on_order_is_not_lost(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = pathlib.Path(temp_dir) / "track.vgm"
            source.write_bytes(same_tick_rearticulation_vgm())
            capsule = cache.extract_capsule(source, corpus_id="synthetic")

        self.assertEqual(capsule["ym2612"]["key_gate_events"]["tick"], [0, 100, 100])
        self.assertEqual(capsule["ym2612"]["key_gate_events"]["operator_mask"], [0xF0, 0x00, 0xF0])
        self.assertEqual(capsule["ym2612"]["events"]["tick"], [0, 100])
        self.assertEqual(capsule["ym2612"]["events"]["key_gate_event_index"], [0, 2])

    def test_second_build_reuses_capsule_without_parsing_source_again(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = pathlib.Path(temp_dir)
            source = temp / "track.vgm"
            source.write_bytes(synthetic_vgm())
            root = temp / "cache"
            destination, changed, _ = cache.build_one(
                source, corpus_id="synthetic", cache_root=root
            )
            self.assertTrue(changed)
            self.assertTrue(destination.is_file())

            with mock.patch.object(
                cache,
                "extract_capsule",
                side_effect=AssertionError("source should not be reparsed"),
            ):
                _, changed, capsule = cache.build_one(
                    source, corpus_id="synthetic", cache_root=root
                )

            self.assertFalse(changed)
            self.assertEqual(capsule["ym2612"]["ordinary_full_key_ons"], 2)

    def test_creator_selection_is_separate_and_role_scoped(self):
        records = [
            {"fixture_path": "a.vgm", "creator": "A", "role": "composer", "status": "exact"},
            {"fixture_path": "b.vgm", "creator": "A", "role": "arranger_programmer", "status": "exact"},
            {"fixture_path": "c.vgm", "creator": "A", "role": "composer", "status": "conflict"},
            {"fixture_path": "d.vgm", "creator": "B", "role": "composer", "status": "exact"},
        ]
        selected = cache.selected_credits(
            records, creator="A", role="composer", admitted_statuses={"exact"}
        )
        self.assertEqual([item["fixture_path"] for item in selected], ["a.vgm"])

    def test_source_size_change_invalidates_cache_without_hashing(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = pathlib.Path(temp_dir)
            source = temp / "track.vgm"
            source.write_bytes(synthetic_vgm())
            root = temp / "cache"
            destination, changed, _ = cache.build_one(
                source, corpus_id="synthetic", cache_root=root
            )
            self.assertTrue(changed)

            data = json.loads(destination.read_text())
            data["source"]["size_bytes"] += 1
            destination.write_text(json.dumps(data))
            self.assertFalse(cache._cache_current(destination, source))

    def test_previous_cache_generation_is_not_reused_after_gate_schema_upgrade(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = pathlib.Path(temp_dir)
            source = temp / "track.vgm"
            source.write_bytes(synthetic_vgm())
            root = temp / "cache"
            destination = cache.cache_path_for(source, "synthetic", root)
            destination.parent.mkdir(parents=True)
            destination.write_text(
                json.dumps({
                    "schema_version": 2,
                    "extractor": {"name": cache.EXTRACTOR_NAME, "version": 2},
                    "source": {"size_bytes": source.stat().st_size},
                }),
                encoding="utf-8",
            )

            self.assertFalse(cache._cache_current(destination, source))


if __name__ == "__main__":
    unittest.main()
