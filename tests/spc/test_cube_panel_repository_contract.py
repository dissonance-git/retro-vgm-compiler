from __future__ import annotations

import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PANEL = ROOT / "research" / "projects" / "sonic3" / "spc-cube-blind-panel.json"
ADMISSIONS = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"
POLICY = ROOT / "research" / "projects" / "sonic3" / "cube-calibration-policy.json"


class CubePanelRepositoryContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.panel = json.loads(PANEL.read_text(encoding="utf-8"))
        cls.policy = json.loads(POLICY.read_text(encoding="utf-8"))
        cls.admissions = [
            json.loads(line)
            for line in ADMISSIONS.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        cls.fixture_by_cue = {
            entry["cue_id"]: entry["fixture_path"]
            for entry in cls.panel["cues"]
        }
        cls.panel_fixtures = set(cls.fixture_by_cue.values())

    def test_panel_is_31_unique_existing_spc_fixtures(self):
        self.assertEqual(len(self.fixture_by_cue), 31)
        self.assertEqual(len(self.panel_fixtures), 31)
        for cue_id, fixture in self.fixture_by_cue.items():
            self.assertRegex(cue_id, r"^cue-\d{3}$")
            self.assertTrue(fixture.endswith(".spc"), fixture)
            self.assertTrue((ROOT / fixture).is_file(), fixture)

    def test_panel_contains_all_15_grounded_composer_admissions(self):
        grounded = {
            entry["fixture_path"]
            for entry in self.admissions
            if entry.get("role") == "composer" and entry.get("status") in {"exact", "derived"}
        }
        self.assertEqual(len(grounded), 15)
        self.assertTrue(grounded <= self.panel_fixtures)

    def test_ancient_magic_uncertain_hikichi_clues_are_holdouts_not_grounding(self):
        weak = {
            entry["fixture_path"]
            for entry in self.policy["evaluation_only_clues"]
        }
        self.assertEqual(
            weak,
            {
                "tests/corpus/ancient-magic-spc/01 - Title.spc",
                "tests/corpus/ancient-magic-spc/03 - Menu.spc",
                "tests/corpus/ancient-magic-spc/10 - Boss Battle.spc",
            },
        )
        grounded = {entry["fixture_path"] for entry in self.admissions}
        self.assertTrue(weak <= self.panel_fixtures)
        self.assertTrue(weak.isdisjoint(grounded))

    def test_all_documented_stress_holdouts_are_in_panel(self):
        lanes = (
            "evaluation_only_clues",
            "shared_composition_holdouts",
            "disputed_mapping_holdouts",
            "third_party_decoys",
        )
        stress = {
            entry["fixture_path"]
            for lane in lanes
            for entry in self.policy[lane]
        }
        self.assertEqual(len(stress), 8)
        self.assertTrue(stress <= self.panel_fixtures)

    def test_team_level_validation_worlds_are_present_but_not_grounded(self):
        expected = {
            "america-oudan-ultra-quiz-spc": 4,
            "battle-master-spc": 4,
        }
        counts = {key: 0 for key in expected}
        for fixture in self.panel_fixtures:
            parts = pathlib.PurePosixPath(fixture).parts
            corpus_id = parts[2]
            if corpus_id in counts:
                counts[corpus_id] += 1
        self.assertEqual(counts, expected)

        grounded = {entry["fixture_path"] for entry in self.admissions}
        for fixture in self.panel_fixtures:
            if any(f"tests/corpus/{corpus}/" in fixture for corpus in expected):
                self.assertNotIn(fixture, grounded)


if __name__ == "__main__":
    unittest.main()
