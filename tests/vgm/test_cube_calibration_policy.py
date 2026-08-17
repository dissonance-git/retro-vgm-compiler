from __future__ import annotations

import collections
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PROJECT = ROOT / "research/projects/sonic3"
POLICY = PROJECT / "cube-calibration-policy.json"
ADMISSIONS = PROJECT / "attribution-control-admissions.jsonl"


def read_policy() -> dict:
    return json.loads(POLICY.read_text(encoding="utf-8"))


def read_admissions() -> list[dict]:
    return [
        json.loads(line)
        for line in ADMISSIONS.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]


class CubeCalibrationPolicyTests(unittest.TestCase):
    def test_current_grounding_state_matches_canonical_admissions(self):
        policy = read_policy()
        admissions = [
            record
            for record in read_admissions()
            if record.get("role") == "composer"
            and record.get("status") in {"exact", "derived"}
            and record.get("candidate") in {"Miyoko Takaoka", "Masanori Hikichi"}
        ]

        counts = collections.Counter(record["candidate"] for record in admissions)
        self.assertEqual(counts, {"Miyoko Takaoka": 11, "Masanori Hikichi": 4})
        self.assertEqual(len(admissions), policy["current_grounding_state"]["total_grounded_composer_cues"])

        by_candidate = collections.defaultdict(set)
        for record in admissions:
            corpus_id = pathlib.PurePosixPath(record["fixture_path"]).parts[2]
            by_candidate[record["candidate"]].add(corpus_id)

        takaoka = policy["current_grounding_state"]["miyoko_takaoka"]
        hikichi = policy["current_grounding_state"]["masanori_hikichi"]
        self.assertEqual(takaoka["grounded_cue_count"], counts["Miyoko Takaoka"])
        self.assertEqual(set(takaoka["grounded_soundtracks"]), by_candidate["Miyoko Takaoka"])
        self.assertEqual(takaoka["strict_cross_soundtrack_status"], "testable")
        self.assertEqual(hikichi["grounded_cue_count"], counts["Masanori Hikichi"])
        self.assertEqual(set(hikichi["grounded_soundtracks"]), by_candidate["Masanori Hikichi"])
        self.assertEqual(hikichi["strict_cross_soundtrack_status"], "underdetermined")

    def test_gleylancer_stays_prospective_until_immutable_bytes_are_admitted(self):
        policy = read_policy()
        admissions = read_admissions()
        worlds = policy["evidence_worlds"]["prospective_exact_control_worlds"]
        gleylancer = next(world for world in worlds if world["work"] == "Gleylancer")

        self.assertEqual(gleylancer["candidate"], "Masanori Hikichi")
        self.assertEqual(gleylancer["status"], "evidence_ready_bytes_missing")
        self.assertEqual(gleylancer["proposed_corpus_id"], "gleylancer-genesis-vgm")
        self.assertIn("immutable VGM/VGZ bytes", gleylancer["admission_gate"])
        self.assertFalse(
            any("gleylancer" in str(record.get("fixture_path", "")).lower() for record in admissions)
        )

        joins = {join["commercial_title"]: join for join in gleylancer["proposed_cue_joins"]}
        self.assertEqual(joins["STAGE 1"]["composer"], "Masanori Hikichi")
        self.assertEqual(joins["STAGE 2"]["composer"], "Noriyuki Iwadare")
        self.assertEqual(joins["STAGE 2"]["usage_after_bytes"], "same_game_non_hikichi_composer_decoy")

        projected = gleylancer["projected_effect_if_two_hikichi_cues_admitted"]
        self.assertEqual(projected["masanori_hikichi_grounded_cue_count"], 6)
        self.assertEqual(set(projected["masanori_hikichi_grounded_soundtracks"]), {"gleylancer-genesis-vgm", "terranigma-spc"})
        self.assertEqual(projected["strict_cross_soundtrack_status"], "testable")
        self.assertEqual(projected["total_takaoka_hikichi_grounded_composer_cues"], 17)

    def test_galaxy_force_port_credit_remains_non_composer_confound(self):
        policy = read_policy()
        admissions = read_admissions()
        confounds = policy["evidence_worlds"]["arrangement_or_implementation_only"]
        galaxy_force = next(world for world in confounds if world["work"] == "Galaxy Force II (Mega Drive)")

        self.assertEqual(galaxy_force["candidate"], "Masanori Hikichi")
        self.assertIn("exclude from Hikichi composer supervision", galaxy_force["usage"])
        self.assertFalse(
            any("galaxy force" in str(record.get("fixture_path", "")).lower() for record in admissions)
        )


if __name__ == "__main__":
    unittest.main()
