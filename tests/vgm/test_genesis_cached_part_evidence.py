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

    def test_conservative_strands_join_only_successive_grounded_links(self):
        episodes = [
            cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1),
            cached.Episode(1, 1100, 2000, 0, 0x330, 3, 0, 2, 3),
            cached.Episode(2, 2100, 3000, 0, 0x360, 3, 0, 4, 5),
        ]
        hypotheses = []
        for first, second in ((episodes[0], episodes[1]), (episodes[1], episodes[2]), (episodes[0], episodes[2])):
            hypotheses.append(cached.infer_continuity(
                first,
                second,
                max_gap_ticks=500,
                max_pitch_interval_octaves=1.5,
            ))

        result = cached.assemble_strand_hypotheses(episodes, hypotheses)
        self.assertEqual(len(result["strands"]), 1)
        self.assertEqual(result["strands"][0]["episode_ids"], [0, 1, 2])
        self.assertAlmostEqual(result["strands"][0]["confidence"], 0.94)
        self.assertEqual(result["unresolved"], [])
        self.assertEqual(
            [(link["first_episode_id"], link["second_episode_id"]) for link in result["strands"][0]["links"]],
            [(0, 1), (1, 2)],
        )

    def test_overlap_and_changed_program_do_not_enter_strands(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        overlap = cached.Episode(1, 800, 1600, 1, 0x320, 3, 0, 2, 3)
        changed = cached.Episode(2, 1100, 2000, 0, 0x360, 3, 1, 4, 5)
        hypotheses = [
            cached.infer_continuity(first, overlap, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
            cached.infer_continuity(first, changed, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
        ]
        result = cached.assemble_strand_hypotheses([first, overlap, changed], hypotheses)
        self.assertEqual(result["strands"], [])
        self.assertEqual(result["unresolved"], [])

    def test_slot_only_edge_never_forms_a_strand(self):
        episodes = [
            cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1),
            cached.Episode(1, 1100, 2000, 0, 0x900, 6, 1, 2, 3),
        ]
        slot_only = {
            "first_episode_id": 0,
            "second_episode_id": 1,
            "confidence": cached.SLOT_ONLY_CEILING,
            "identity_bearing_support": False,
            "cross_domain_grounded": False,
            "strong_conflict_present": False,
            "gap_ticks": 100,
            "evidence": [{
                "kind": "physical_slot_continuity",
                "polarity": "supports",
                "confidence": 0.55,
            }],
        }
        result = cached.assemble_strand_hypotheses(episodes, [slot_only])
        self.assertEqual(result["strands"], [])

    def test_equal_time_successor_fork_stays_unresolved(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        branch_a = cached.Episode(1, 1100, 1800, 0, 0x330, 3, 0, 2, 4)
        branch_b = cached.Episode(2, 1100, 1900, 1, 0x340, 3, 0, 3, 5)
        hypotheses = [
            cached.infer_continuity(first, branch_a, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
            cached.infer_continuity(first, branch_b, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
        ]
        result = cached.assemble_strand_hypotheses([first, branch_a, branch_b], hypotheses)
        self.assertEqual(result["strands"], [])
        self.assertEqual(result["unresolved"], [{
            "kind": "ambiguous_successor",
            "episode_id": 0,
            "candidate_episode_ids": [1, 2],
        }])

    def test_competing_predecessors_stay_unresolved(self):
        first_a = cached.Episode(0, 0, 900, 0, 0x300, 3, 0, 0, 2)
        first_b = cached.Episode(1, 100, 900, 1, 0x300, 3, 0, 1, 3)
        target = cached.Episode(2, 1000, 1800, 0, 0x330, 3, 0, 4, 5)
        hypotheses = [
            cached.infer_continuity(first_a, target, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
            cached.infer_continuity(first_b, target, max_gap_ticks=500, max_pitch_interval_octaves=1.5),
        ]
        result = cached.assemble_strand_hypotheses([first_a, first_b, target], hypotheses)
        self.assertEqual(result["strands"], [])
        self.assertEqual(result["unresolved"], [{
            "kind": "ambiguous_predecessor",
            "episode_id": 2,
            "candidate_episode_ids": [0, 1],
        }])

    def test_strand_projection_remains_creator_and_source_blind(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        second = cached.Episode(1, 1100, 2000, 0, 0x330, 3, 0, 2, 3)
        hypothesis = cached.infer_continuity(first, second, max_gap_ticks=500, max_pitch_interval_octaves=1.5)
        result = cached.assemble_strand_hypotheses([first, second], [hypothesis])
        payload_text = str({"strands": result["strands"], "unresolved": result["unresolved"]}).lower()
        self.assertNotIn("composer", payload_text)
        self.assertNotIn("creator", payload_text)
        self.assertNotIn("source", payload_text)
        self.assertNotIn(".vgm", payload_text)
        self.assertNotIn("tests/corpus", payload_text)
        self.assertEqual(
            set(result["strands"][0]),
            {"episode_ids", "start_tick", "end_tick", "confidence", "links"},
        )

    def test_strand_threshold_must_exceed_single_domain_ceiling(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        second = cached.Episode(1, 1100, 2000, 0, 0x330, 3, 0, 2, 3)
        with self.assertRaisesRegex(ValueError, "single-domain ceiling"):
            cached.assemble_strand_hypotheses(
                [first, second],
                [],
                min_confidence=cached.SINGLE_DOMAIN_CEILING,
            )

    def test_strand_assembly_rejects_forged_identity_summary(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        second = cached.Episode(1, 1100, 2000, 0, 0x330, 3, 0, 2, 3)
        hypothesis = cached.infer_continuity(
            first, second, max_gap_ticks=500, max_pitch_interval_octaves=1.5
        )
        hypothesis["evidence"] = [
            item
            for item in hypothesis["evidence"]
            if item["kind"] != "instrument_program_identity"
        ]
        with self.assertRaisesRegex(ValueError, "identity-bearing summary"):
            cached.assemble_strand_hypotheses([first, second], [hypothesis])

    def test_strand_assembly_rejects_forged_conflict_summary(self):
        first = cached.Episode(0, 0, 1000, 0, 0x300, 3, 0, 0, 1)
        second = cached.Episode(1, 1100, 2000, 0, 0x330, 3, 0, 2, 3)
        hypothesis = cached.infer_continuity(
            first, second, max_gap_ticks=500, max_pitch_interval_octaves=1.5
        )
        hypothesis["evidence"].append({
            "kind": "simultaneous_conflict",
            "polarity": "counters",
            "confidence": 0.88,
        })
        with self.assertRaisesRegex(ValueError, "conflict summary"):
            cached.assemble_strand_hypotheses([first, second], [hypothesis])


if __name__ == "__main__":
    unittest.main()
