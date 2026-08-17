from __future__ import annotations

import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
PANEL_PATH = ROOT / "research" / "projects" / "sonic3" / "spc-cube-blind-panel.json"
ADMISSIONS_PATH = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"
POLICY_PATH = ROOT / "research" / "projects" / "sonic3" / "cube-calibration-policy.json"

FORBIDDEN_CUE_KEYS = {
    "artist",
    "composer",
    "candidate",
    "role",
    "attribution",
    "soundtrack",
    "track_title",
    "external_artist",
}


class CubeBlindPanelCoverageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.panel = json.loads(PANEL_PATH.read_text(encoding="utf-8"))
        cls.policy = json.loads(POLICY_PATH.read_text(encoding="utf-8"))
        cls.admissions = [
            json.loads(line)
            for line in ADMISSIONS_PATH.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]

    def test_cue_ids_are_unique_opaque_and_label_free(self) -> None:
        cues = self.panel["cues"]
        cue_ids = [cue["cue_id"] for cue in cues]
        self.assertEqual(len(cue_ids), len(set(cue_ids)))
        for cue in cues:
            self.assertRegex(cue["cue_id"], r"^cue-\d{3}$")
            self.assertTrue(cue["fixture_path"].startswith("tests/corpus/"))
            self.assertEqual(FORBIDDEN_CUE_KEYS.intersection(cue), set())

    def test_every_grounded_composer_admission_is_in_blind_panel(self) -> None:
        admitted = {
            entry["fixture_path"]
            for entry in self.admissions
            if entry.get("role") == "composer"
        }
        panel = {cue["fixture_path"] for cue in self.panel["cues"]}
        self.assertEqual(len(admitted), 15)
        self.assertTrue(admitted.issubset(panel))

    def test_policy_holdouts_are_present_but_not_admitted(self) -> None:
        panel = {cue["fixture_path"] for cue in self.panel["cues"]}
        admitted = {entry["fixture_path"] for entry in self.admissions}

        holdouts = {
            entry["fixture_path"]
            for section in (
                "shared_composition_holdouts",
                "disputed_mapping_holdouts",
                "evaluation_only_clues",
                "third_party_decoys",
            )
            for entry in self.policy.get(section, [])
            if isinstance(entry.get("fixture_path"), str)
        }
        self.assertTrue(holdouts.issubset(panel))
        self.assertEqual(holdouts.intersection(admitted), set())

    def test_panel_is_broader_than_training_controls(self) -> None:
        panel_paths = {cue["fixture_path"] for cue in self.panel["cues"]}
        admitted_paths = {entry["fixture_path"] for entry in self.admissions}
        self.assertGreater(len(panel_paths), len(admitted_paths))
        self.assertEqual(len(panel_paths), 31)


if __name__ == "__main__":
    unittest.main()
