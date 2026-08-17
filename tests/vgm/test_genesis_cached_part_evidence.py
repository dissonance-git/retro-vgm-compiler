from __future__ import annotations

import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools" / "genesis_cached_part_evidence.py"
spec = importlib.util.spec_from_file_location("genesis_cached_part_evidence", MODULE_PATH)
assert spec and spec.loader
cached = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = cached
spec.loader.exec_module(cached)


def capsule(*, events, gates, duration=2500):
    return {
        "schema_version": 3,
        "extractor": {"name": "creator-blind-genesis-song-capsule", "version": 3},
        "label_policy": "Creator labels excluded.",
        "source": {"path": "tests/corpus/hidden/opaque.vgz", "file": "opaque.vgz", "corpus_id": "hidden"},
        "timing": {"duration_vgm_samples": duration},
        "ym2612": {
            "events": {
                "tick": [row[0] for row in events],
                "channel": [row[1] for row in events],
                "fnum": [row[2] for row in events],
                "block": [row[3] for row in events],
                "patch_full_id": [row[4] for row in events],
                "key_gate_event_index": [row[5] for row in events],
            },
            "key_gate_events": {
                "tick": [row[0] for row in gates],
                "channel": [row[1] for row in gates],
                "operator_mask": [row[2] for row in gates],
            },
        },
    }


class GenesisCachedPartEvidenceTests(unittest.TestCase):
    def test_same_tick_off_then_on_reconstructs_two_bounded_episodes(self):
        data = capsule(
            events=[
                (0, 0, 0x300, 3, 0, 0),
                (100, 0, 0x360, 3, 0, 2),
            ],
            gates=[
                (0, 0, 0xF0),
                (100, 0, 0x00),
                (100, 0, 0xF0),
            ],
            duration=200,
        )
        episodes = cached.reconstruct_episodes(data)
        self.assertEqual([(item.start_tick, item.end_tick) for item in episodes], [(0, 100), (100, 200)])
        self.assertEqual([item.onset_gate_event_index for item in episodes], [0, 2])
        self.assertEqual([item.end_gate_event_index for item in episodes], [1, None])

    def test_cache_hypotheses_match_cpp_strong_and_overlap_controls(self):
        # Mirrors tests/vgm/genesis_part_evidence_test.cpp:
        # first = [0,1000] ch0, overlap = [800,1600] ch1,
        # second = [1100,2000] ch0, all with the same program fingerprint.
        data = capsule(
            events=[
                (0, 0, 0x300, 3, 0, 0),
                (800, 1, 0x320, 3, 0, 1),
                (1100, 0, 0x360, 3, 0, 3),
            ],
            gates=[
                (0, 0, 0xF0),
                (800, 1, 0xF0),
                (1000, 0, 0x00),
                (1100, 0, 0xF0),
                (1600, 1, 0x00),
                (2000, 0, 0x00),
            ],
        )
        episodes = cached.reconstruct_episodes(data)
        first, overlapping, second = episodes

        strong = cached.infer_continuity(
            first,
            second,
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
        )
        self.assertAlmostEqual(strong["proposed_confidence"], 0.94)
        self.assertAlmostEqual(strong["confidence"], 0.94)
        self.assertTrue(strong["identity_bearing_support"])
        self.assertTrue(strong["cross_domain_grounded"])
        strong_kinds = {(item["kind"], item["polarity"]) for item in strong["evidence"]}
        self.assertIn(("physical_slot_continuity", "supports"), strong_kinds)
        self.assertIn(("instrument_program_identity", "supports"), strong_kinds)
        self.assertIn(("temporal_adjacency", "supports"), strong_kinds)
        self.assertIn(("pitch_trajectory_continuity", "supports"), strong_kinds)

        conflict = cached.infer_continuity(
            first,
            overlapping,
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
        )
        self.assertTrue(conflict["strong_conflict_present"])
        self.assertAlmostEqual(conflict["confidence"], cached.STRONG_CONFLICT_CEILING)
        conflict_kinds = {(item["kind"], item["polarity"]) for item in conflict["evidence"]}
        self.assertIn(("simultaneous_conflict", "counters"), conflict_kinds)

    def test_changed_program_cannot_be_promoted_by_slot_timing_and_pitch(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        second = cached.Episode(1, 1100, 2000, 0, 0x360, 3, 1, 2, 3)
        hypothesis = cached.infer_continuity(
            first,
            second,
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
        )
        self.assertFalse(hypothesis["identity_bearing_support"])
        self.assertLessEqual(hypothesis["confidence"], cached.NO_IDENTITY_CEILING)
        kinds = {(item["kind"], item["polarity"]) for item in hypothesis["evidence"]}
        self.assertIn(("identity_discontinuity", "counters"), kinds)

    def test_projection_does_not_copy_source_identity_into_output(self):
        data = capsule(
            events=[
                (0, 0, 0x300, 3, 0, 0),
                (100, 0, 0x360, 3, 0, 2),
            ],
            gates=[
                (0, 0, 0xF0),
                (50, 0, 0x00),
                (100, 0, 0xF0),
                (150, 0, 0x00),
            ],
            duration=200,
        )
        projected = cached.project(data, max_gap_ticks=100, max_pitch_interval_octaves=1.5)
        text = str(projected)
        self.assertNotIn("opaque.vgz", text)
        self.assertNotIn("tests/corpus", text)
        self.assertNotIn("hidden", text)


if __name__ == "__main__":
    unittest.main()
