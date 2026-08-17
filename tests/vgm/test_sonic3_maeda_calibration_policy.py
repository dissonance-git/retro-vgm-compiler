from __future__ import annotations

import collections
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PANEL = ROOT / "research" / "projects" / "sonic3" / "maeda-calibration-policy.json"


class Sonic3MaedaCalibrationPolicyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.panel = json.loads(PANEL.read_text(encoding="utf-8"))

    def test_golden_axe_iii_is_track_resolved_with_one_quarantined_conflict(self) -> None:
        tracks = self.panel["golden_axe_iii_track_resolved_world"]["tracks"]
        self.assertEqual(len(tracks), 21)
        composers = collections.Counter(
            item["composer"] for item in tracks if "composer" in item
        )
        self.assertEqual(composers["Tatsuyuki Maeda"], 10)
        self.assertEqual(composers["Naofumi Hataya"], 4)
        self.assertEqual(composers["Haruyo Oguro"], 5)
        self.assertEqual(composers["Tomonori Sawada"], 1)

        conflicts = [item for item in tracks if item["credit_state"] == "conflict"]
        self.assertEqual(len(conflicts), 1)
        self.assertTrue(conflicts[0]["fixture_path"].endswith("11 - The Scorching Sand.vgz"))
        self.assertEqual(conflicts[0]["use"], "quarantined_conflict")

        for item in tracks:
            self.assertTrue((ROOT / item["fixture_path"]).is_file(), item["fixture_path"])

    def test_sonic_3d_blast_supplies_ten_exact_maeda_track_controls(self) -> None:
        world = self.panel["sonic_3d_blast_exact_track_world"]
        self.assertEqual(world["role_scope"], ["composer", "arranger_programmer"])
        self.assertEqual(len(world["maeda_fixtures"]), 10)
        for fixture in world["maeda_fixtures"]:
            self.assertTrue((ROOT / fixture).is_file(), fixture)

    def test_sonic_3d_blast_role_map_covers_all_24_committed_fixtures(self) -> None:
        world = self.panel["sonic_3d_blast_exact_track_world"]
        credits = world["track_credits"]
        self.assertEqual(world["corpus_fixture_count"], 24)
        self.assertTrue(world["partition_complete_for_candidate"])
        self.assertEqual(len(credits), 24)

        fixture_paths = [item["fixture_path"] for item in credits]
        self.assertEqual(len(set(fixture_paths)), 24)
        for fixture in fixture_paths:
            self.assertTrue((ROOT / fixture).is_file(), fixture)

        composers = collections.Counter(item["composer"] for item in credits)
        arrangers = collections.Counter(item["arranger_programmer"] for item in credits)
        self.assertEqual(
            composers,
            collections.Counter(
                {
                    "Tatsuyuki Maeda": 10,
                    "Jun Senoue": 12,
                    "Masaru Setsumaru": 1,
                    "Seirou Okamoto": 1,
                }
            ),
        )
        self.assertEqual(
            arrangers,
            collections.Counter(
                {
                    "Tatsuyuki Maeda": 10,
                    "Jun Senoue": 10,
                    "Masaru Setsumaru": 4,
                }
            ),
        )

        maeda_from_roles = {
            item["fixture_path"]
            for item in credits
            if item["composer"] == "Tatsuyuki Maeda"
        }
        self.assertEqual(maeda_from_roles, set(world["maeda_fixtures"]))

    def test_sonic_3d_renamed_tail_mappings_stay_explicit_and_revisable(self) -> None:
        credits = self.panel["sonic_3d_blast_exact_track_world"]["track_credits"]
        derived = {
            pathlib.Path(item["fixture_path"]).name: item
            for item in credits
            if item["mapping_state"] == "derived_audio_correspondence"
        }
        self.assertEqual(
            set(derived),
            {"19 - Special Stage 2.vgm", "22 - Robotnik 3.vgm"},
        )
        self.assertEqual(derived["19 - Special Stage 2.vgm"]["rom_work"], "Sonic 3 Bonus Stage")
        self.assertEqual(derived["19 - Special Stage 2.vgm"]["composer"], "Jun Senoue")
        self.assertEqual(derived["19 - Special Stage 2.vgm"]["arranger_programmer"], "Jun Senoue")
        self.assertEqual(derived["22 - Robotnik 3.vgm"]["rom_work"], "unused 1D")
        self.assertEqual(derived["22 - Robotnik 3.vgm"]["composer"], "Jun Senoue")
        self.assertEqual(
            derived["22 - Robotnik 3.vgm"]["arranger_programmer"],
            "Masaru Setsumaru",
        )
        for item in derived.values():
            self.assertIn("revisable", item["mapping_note"])

    def test_sonic_3d_specificity_policy_does_not_invent_singleton_clusters(self) -> None:
        specificity = self.panel["sonic_3d_blast_exact_track_world"]["role_specificity_policy"]
        self.assertEqual(
            specificity["composer"]["learnable_classes"],
            {"Tatsuyuki Maeda": 10, "Jun Senoue": 12},
        )
        self.assertEqual(
            specificity["composer"]["singleton_sentinels"],
            {"Masaru Setsumaru": 1, "Seirou Okamoto": 1},
        )
        self.assertEqual(
            specificity["arranger_programmer"]["learnable_classes"],
            {"Tatsuyuki Maeda": 10, "Jun Senoue": 10, "Masaru Setsumaru": 4},
        )
        self.assertIn("never fake learnable clusters", specificity["composer"]["rule"])
        self.assertIn("cannot promote a composition claim", specificity["arranger_programmer"]["rule"])

    def test_whole_soundtrack_controls_cross_game_and_platform(self) -> None:
        worlds = {item["corpus_id"]: item for item in self.panel["whole_soundtrack_worlds"]}
        self.assertEqual(len(worlds["j-league-pro-striker-2-vgz"]["fixtures"]), 6)
        self.assertEqual(len(worlds["super-columns-vgm"]["fixtures"]), 12)
        self.assertEqual(worlds["j-league-pro-striker-2-vgz"]["platform_id"], "mega-drive")
        self.assertEqual(worlds["super-columns-vgm"]["platform_id"], "game-gear")
        for world in worlds.values():
            for fixture in world["fixtures"]:
                self.assertTrue((ROOT / fixture).is_file(), fixture)

    def test_sonic_3_target_stays_creator_blind(self) -> None:
        policy = self.panel["sonic3_target_policy"]
        self.assertEqual(policy["target_environment"], "tests/corpus/sonic-3-knuckles")
        self.assertIn("Do not label", policy["rule"])
        self.assertIn("never a historical assignment", policy["promotion"])


if __name__ == "__main__":
    unittest.main()
