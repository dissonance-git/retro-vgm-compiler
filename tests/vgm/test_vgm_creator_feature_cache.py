from __future__ import annotations

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import vgm_creator_cached_similarity as cached_similarity  # noqa: E402
import vgm_creator_feature_cache as feature_cache  # noqa: E402


def _part(channel: int, intervals: dict[str, int], bigrams: dict[str, int]) -> dict[str, object]:
    return {
        "channel": channel,
        "key_ons": 10,
        "interval_histogram_semitones": intervals,
        "interval_bigram_histogram": bigrams,
    }


def _capsule(soundtrack: str, filename: str, part: dict[str, object]) -> dict[str, object]:
    motion = feature_cache._motion_part(part)
    return {
        "schema_version": feature_cache.SCHEMA_VERSION,
        "model": feature_cache.MODEL,
        "label_policy": "No composer/artist metadata is stored in this capsule.",
        "source": {"soundtrack_id": soundtrack, "file": filename, "source_path": filename},
        "views": {
            "canonical_events": {
                "duration_vgm_samples": 0,
                "ordinary_full_fm_key_on_count": 0,
                "ordinary_full_fm_key_ons": [],
            },
            "gen1": {
                "musical_trajectory": {
                    "interval_histogram_semitones": dict(part["interval_histogram_semitones"]),
                    "interval_bigram_histogram": dict(part["interval_bigram_histogram"]),
                    "normalized_onset_gap_histogram": {},
                    "contour_histogram": {},
                },
                "realization": {
                    "core_patch_usage": {},
                    "algorithm_histogram": {},
                    "feedback_histogram": {},
                    "pan_histogram": {},
                },
            },
            "gen2_parts": {"parts": [part]},
            "gen3_motion_parts": {"parts": [motion]},
        },
    }


def test_motion_view_removes_zero_interval_and_every_bigram_touching_zero() -> None:
    source = _part(
        2,
        {"0": 8, "2": 3, "-1": 2},
        {"0,0": 4, "0,2": 2, "2,0": 2, "2,-1": 3},
    )
    motion = feature_cache._motion_part(source)

    assert motion["interval_histogram_semitones"] == {"2": 3, "-1": 2}
    assert motion["interval_bigram_histogram"] == {"2,-1": 3}
    assert motion["channel"] == 2


def test_cached_similarity_reads_capsules_without_source_music(tmp_path: pathlib.Path) -> None:
    left = _capsule("a", "01.vgz", _part(0, {"2": 5}, {"2,2": 4}))
    right = _capsule("b", "02.vgz", _part(5, {"2": 7}, {"2,2": 6}))

    left_path = tmp_path / "left.json"
    right_path = tmp_path / "right.json"
    left_path.write_text(json.dumps(left), encoding="utf-8")
    right_path.write_text(json.dumps(right), encoding="utf-8")

    loaded = [cached_similarity._load(left_path), cached_similarity._load(right_path)]
    matrix = cached_similarity.build_matrix(loaded, "gen3-motion-parts")

    assert matrix["track_ids"] == ["a::01.vgz", "b::02.vgz"]
    assert matrix["similarity_matrix"] == [[1.0, 1.0], [1.0, 1.0]]
    assert "no VGM/VGZ" in matrix["source_policy"]


def test_capsule_contract_is_creator_blind_and_event_first() -> None:
    assert feature_cache.SCHEMA_VERSION >= 2
    assert "composer" in feature_cache.__doc__.lower()
    assert "canonical" in feature_cache.__doc__.lower()
    assert "creator-blind" in feature_cache.MODEL


def test_existing_cache_is_reused_without_refresh(tmp_path: pathlib.Path, monkeypatch) -> None:
    source = tmp_path / "01.vgz"
    source.write_bytes(b"not parsed because cache already exists")
    out = tmp_path / "cache"
    out.mkdir()
    cached_file = out / "001-01.json"
    cached_file.write_text('{"sentinel": true}\n', encoding="utf-8")

    def fail_extract(*_args, **_kwargs):
        raise AssertionError("existing capsule should not be reparsed")

    monkeypatch.setattr(feature_cache, "extract_capsule", fail_extract)
    manifest = feature_cache.cache_corpus(
        [source], soundtrack_id="test", output_dir=out, refresh=False
    )

    assert manifest["tracks"][0]["state"] == "cached"
    assert json.loads(cached_file.read_text(encoding="utf-8")) == {"sentinel": True}
