from __future__ import annotations

import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
POLICY = ROOT / "research" / "projects" / "sonic3" / "cube-calibration-policy.json"
ADMISSIONS = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"


class GleylancerProspectiveControlGateTest(unittest.TestCase):
    def prospective_world(self) -> dict:
        policy = json.loads(POLICY.read_text(encoding="utf-8"))
        worlds = policy["evidence_worlds"]["prospective_exact_control_worlds"]
        matches = [world for world in worlds if world.get("work") == "Gleylancer"]
        self.assertEqual(len(matches), 1)
        return matches[0]

    def test_structured_future_joins_preserve_two_hikichi_controls_and_iwadare_decoy(self):
        world = self.prospective_world()
        joins = world["proposed_cue_joins"]
        self.assertEqual(
            [
                (join["vgm_pack_ordinal"], join["commercial_title"], join["composer"])
                for join in joins
            ],
            [
                (1, "OPENING", "Masanori Hikichi"),
                (3, "STAGE 1", "Masanori Hikichi"),
                (4, "STAGE 2", "Noriyuki Iwadare"),
            ],
        )
        self.assertEqual(joins[0]["admission_after_bytes"], "derived")
        self.assertEqual(joins[1]["admission_after_bytes"], "exact")
        self.assertEqual(joins[2]["usage_after_bytes"], "same_game_non_hikichi_composer_decoy")

    def test_missing_bytes_state_cannot_leak_into_canonical_admissions(self):
        world = self.prospective_world()
        if world["status"] != "evidence_ready_bytes_missing":
            self.skipTest("Gleylancer acquisition state has advanced; use the new state contract")

        corpus_id = world["proposed_corpus_id"]
        self.assertFalse((ROOT / "tests" / "corpus" / corpus_id).exists())

        admitted = []
        for raw_line in ADMISSIONS.read_text(encoding="utf-8").splitlines():
            if not raw_line.strip():
                continue
            entry = json.loads(raw_line)
            fixture = entry.get("fixture_path")
            if isinstance(fixture, str) and corpus_id in fixture:
                admitted.append(entry)
        self.assertEqual(admitted, [])

        self.assertIn(
            "Do not add these labels to attribution-control-admissions.jsonl until the immutable VGM/VGZ bytes are committed",
            world["admission_gate"],
        )

    def test_projected_hikichi_coverage_matches_strict_transfer_requirement(self):
        world = self.prospective_world()
        projected = world["projected_effect_if_two_hikichi_cues_admitted"]
        self.assertEqual(projected["masanori_hikichi_grounded_cue_count"], 6)
        self.assertEqual(
            set(projected["masanori_hikichi_grounded_soundtracks"]),
            {"terranigma-spc", "gleylancer-genesis-vgm"},
        )
        self.assertEqual(projected["strict_cross_soundtrack_status"], "testable")
        self.assertEqual(projected["total_takaoka_hikichi_grounded_composer_cues"], 17)


if __name__ == "__main__":
    unittest.main()
