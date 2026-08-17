from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPC_TOOLS = ROOT / "tools" / "spc"
if str(SPC_TOOLS) not in sys.path:
    sys.path.insert(0, str(SPC_TOOLS))

import creator_blind_spc_cache as cache


def profile() -> dict[str, object]:
    return {
        "profile_index": 0,
        "gesture_count": 3,
        "normalized_inter_onset_intervals": [1.0, 1.0],
        "interval_octaves": [0.25, -0.25],
        "pitch_contour": [1, -1],
        "interval_semantics": "performed_pitch_ratio",
        "pitch_range_octaves": 0.5,
        "evidence_status": "derived",
        "evidence_confidence": 0.9,
    }


def sidecar(
    source_size: int,
    seconds: int,
    *,
    poison: bool = False,
    profiles: bool = True,
) -> dict[str, object]:
    part_profiles = [profile()] if profiles else []
    value: dict[str, object] = {
        "model": cache.EXPECTED_MODEL,
        "claim_boundary": "blind",
        "provenance": {
            "retro_vgm_compiler_commit": "test",
            "snes_spc_repository": "blarggs-audio-libraries/snes_spc",
            "snes_spc_commit": "test-snes-spc",
            "instrumentation_patch_contract": "test-contract",
            "device_tick_rate": 1024000,
        },
        "controlled_execution": {
            "source_bytes": source_size,
            "requested_seconds": seconds,
            "requested_device_clocks": seconds * 1024000,
        },
        "capture": {
            "ram_write_count": 4,
            "ram_writes_spc700_cpu": 4,
            "ram_writes_dsp_echo": 0,
            "ram_writes_ipl_rom_overlay": 0,
            "window_count": 1,
            "stored_event_count": 5,
            "dropped_event_count": 0,
            "overflowed_window_count": 0,
        },
        "replay": {
            "windows_replayed": 1,
            "ram_writes_applied": 4,
            "records_materialized": 5,
            "continuity_breaks": 0,
            "samples_materialized": 1,
            "samples_reused": 1,
            "samples_deferred": 0,
            "final_ram_write_serial": 4,
        },
        "features": {
            "voice_episode_count": 3,
            "eligible_episode_count": 3,
            "candidate_transition_count": 2,
            "strong_transition_count": 2,
            "rejected_transition_count": 0,
            "continuity_barrier_count": 0,
            "emitted_part_count": len(part_profiles),
            "part_profile_count": len(part_profiles),
            "part_profiles": part_profiles,
        },
    }
    if poison:
        value["composer"] = "label leak"
    return value


class CreatorBlindSpcCacheTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temporary.name)
        self.source = self.root / "01 - Cue.spc"
        self.source.write_bytes(b"SNES-SPC700 Sound File Data" + bytes(512))
        self.extractor = self.root / "spc_forensic_features"
        self.extractor.write_bytes(b"placeholder")
        self.cache_root = self.root / "cache"

    def tearDown(self):
        self.temporary.cleanup()

    def fake_run(self, args, check):
        self.assertTrue(check)
        output = pathlib.Path(args[2])
        seconds = int(args[3])
        output.write_text(
            json.dumps(sidecar(self.source.stat().st_size, seconds)),
            encoding="utf-8",
        )

    def test_build_reuses_valid_sidecar_without_executing_source_again(self):
        with mock.patch.object(cache.subprocess, "run", side_effect=self.fake_run) as run:
            destination, changed = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=5,
            )
            self.assertTrue(changed)
            self.assertEqual(run.call_count, 1)

        with mock.patch.object(
            cache.subprocess,
            "run",
            side_effect=AssertionError("valid cache must not rerun SPC execution"),
        ):
            same, changed = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=5,
            )
        self.assertEqual(same, destination)
        self.assertFalse(changed)

    def test_capture_durations_coexist_without_source_hash(self):
        with mock.patch.object(cache.subprocess, "run", side_effect=self.fake_run):
            five, _ = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=5,
            )
            ten, _ = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=10,
            )
        self.assertNotEqual(five, ten)
        self.assertIn("5s", five.parts)
        self.assertIn("10s", ten.parts)
        self.assertTrue(cache.cache_current(five, self.source, seconds=5))
        self.assertTrue(cache.cache_current(ten, self.source, seconds=10))

    def test_source_size_change_invalidates_without_source_hash(self):
        with mock.patch.object(cache.subprocess, "run", side_effect=self.fake_run):
            destination, _ = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=5,
            )
        self.source.write_bytes(self.source.read_bytes() + b"x")
        self.assertFalse(cache.cache_current(destination, self.source, seconds=5))

    def test_zero_profile_negative_result_is_cached(self):
        def zero_run(args, check):
            self.assertTrue(check)
            pathlib.Path(args[2]).write_text(
                json.dumps(
                    sidecar(
                        self.source.stat().st_size,
                        int(args[3]),
                        profiles=False,
                    )
                ),
                encoding="utf-8",
            )

        with mock.patch.object(cache.subprocess, "run", side_effect=zero_run):
            destination, changed = cache.build_one(
                self.source,
                corpus_id="test-spc",
                extractor=self.extractor,
                cache_root=self.cache_root,
                seconds=5,
            )
        self.assertTrue(changed)
        self.assertTrue(cache.cache_current(destination, self.source, seconds=5))

    def test_label_bearing_forensic_output_is_rejected(self):
        def poison_run(args, check):
            self.assertTrue(check)
            pathlib.Path(args[2]).write_text(
                json.dumps(sidecar(self.source.stat().st_size, int(args[3]), poison=True)),
                encoding="utf-8",
            )

        with mock.patch.object(cache.subprocess, "run", side_effect=poison_run):
            with self.assertRaises(ValueError):
                cache.build_one(
                    self.source,
                    corpus_id="test-spc",
                    extractor=self.extractor,
                    cache_root=self.cache_root,
                    seconds=5,
                )
        destination = cache.destination_for(
            self.source,
            corpus_id="test-spc",
            cache_root=self.cache_root,
            seconds=5,
        )
        self.assertFalse(destination.exists())

    def test_destination_is_song_centered_not_creator_or_panel_centered(self):
        destination = cache.destination_for(
            self.source,
            corpus_id="terranigma-spc",
            cache_root=self.cache_root,
            seconds=5,
        )
        self.assertIn("terranigma-spc", destination.parts)
        self.assertIn("5s", destination.parts)
        self.assertNotIn("composer", destination.as_posix().lower())
        self.assertNotIn("cue-", destination.as_posix().lower())


if __name__ == "__main__":
    unittest.main()
