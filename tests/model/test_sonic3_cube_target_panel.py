from __future__ import annotations

import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PANEL = ROOT / "research" / "projects" / "sonic3" / "sonic3-cube-target-panel.json"


class Sonic3CubeTargetPanelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.panel = json.loads(PANEL.read_text(encoding="utf-8"))

    def test_exact_positive_anchors(self):
        anchors = {
            (item["candidate"], item["fixture_path"], item["role"], item["status"])
            for item in self.panel["positive_anchors"]
        }
        self.assertEqual(
            anchors,
            {
                (
                    "Miyoko Takaoka",
                    "tests/corpus/sonic-3-knuckles/05 - Marble Garden Zone Act 1.vgz",
                    "composer",
                    "strong",
                ),
                (
                    "Masanori Hikichi",
                    "tests/corpus/sonic-3-knuckles/27 - Boss Theme.vgz",
                    "composer",
                    "strong",
                ),
            },
        )

    def test_six_primary_unknown_work_families_remain_unlabeled(self):
        targets = self.panel["primary_unknown_targets"]
        self.assertEqual(
            {item["work_family_id"] for item in targets},
            {
                "mushroom-hill",
                "flying-battery",
                "sandopolis",
                "sky-sanctuary",
                "death-egg",
                "doomsday",
            },
        )
        for item in targets:
            self.assertNotIn("candidate", item)
            self.assertNotIn("composer", item)
            for fixture in item["fixtures"]:
                self.assertTrue((ROOT / fixture).is_file(), fixture)

    def test_hikichi_leads_are_notes_not_ground_truth(self):
        by_family = {
            item["work_family_id"]: item
            for item in self.panel["primary_unknown_targets"]
        }
        self.assertIn("Hikichi", by_family["mushroom-hill"]["candidate_note"])
        self.assertIn("Hikichi", by_family["doomsday"]["candidate_note"])
        self.assertNotIn("status", by_family["mushroom-hill"])
        self.assertNotIn("status", by_family["doomsday"])

    def test_setsamaru_and_nagao_are_role_confounds_only(self):
        roles = {
            (item["candidate"], item["role"])
            for item in self.panel["arrangement_confound_controls"]
        }
        self.assertEqual(
            roles,
            {
                ("Masayuki Nagao", "arranger_programmer"),
                ("Masaru Setsumaru", "arranger_programmer"),
            },
        )
        for item in self.panel["arrangement_confound_controls"]:
            self.assertNotEqual(item["role"], "composer")

    def test_known_sega_decoys_are_not_cube_labels(self):
        controls = self.panel["known_non_cube_composer_controls"]
        self.assertEqual(
            {item["candidate"] for item in controls},
            {"Jun Senoue", "Yoshiaki Kashima"},
        )
        self.assertTrue(all(item["role"] == "composer" for item in controls))
        self.assertTrue(all(item["status"] == "strong" for item in controls))

    def test_version_controls_include_replacement_and_same_work_cases(self):
        families = {item["family"] for item in self.panel["version_confound_controls"]}
        self.assertEqual(
            families,
            {"carnival-night", "icecap", "launch-base", "mushroom-hill"},
        )
        for item in self.panel["version_confound_controls"]:
            self.assertTrue((ROOT / item["final"]).is_file(), item["final"])
            self.assertTrue((ROOT / item["prototype"]).is_file(), item["prototype"])


if __name__ == "__main__":
    unittest.main()
