from __future__ import annotations

import collections
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PROJECT = ROOT / "research" / "projects" / "sonic3"
PREREG = PROJECT / "maeda-calibration-preregistration.json"
POLICY = PROJECT / "maeda-calibration-policy.json"
FAMILIES = PROJECT / "sonic3d-role-family-policy.json"
RUNBOOK = PROJECT / "maeda-calibration-runbook.md"


class MaedaPreregistrationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.prereg = json.loads(PREREG.read_text(encoding="utf-8"))
        cls.policy = json.loads(POLICY.read_text(encoding="utf-8"))
        cls.families = json.loads(FAMILIES.read_text(encoding="utf-8"))
        cls.runbook = RUNBOOK.read_text(encoding="utf-8")

    def test_method_is_explicitly_pre_data_and_sonic3_is_held_out(self) -> None:
        self.assertEqual(self.prereg["status"], "pre-data_method_frozen")
        self.assertEqual(self.prereg["candidate"], "Tatsuyuki Maeda")
        self.assertEqual(
            self.prereg["held_out_target"],
            self.policy["sonic3_target_policy"]["target_environment"],
        )
        self.assertEqual(
            self.prereg["result_status"],
            "No real calibration result is declared by this preregistration artifact.",
        )
        self.assertEqual(
            self.prereg["method_tree_commit"],
            "47fa9fbc3ef0f5b1c2a5e91a7ab22efb7088d43c",
        )

    def test_primary_parameters_cannot_silently_drift(self) -> None:
        params = self.prereg["primary_parameters"]
        self.assertEqual(params["retrieval_k"], 3)
        self.assertEqual(params["permutations"], 5000)
        self.assertEqual(params["seed"], 20260816)
        self.assertEqual(params["primary_alpha"], 0.05)
        self.assertEqual(params["role_null_permutation_unit"], "family-block")
        self.assertEqual(params["role_null_secondary_sensitivity_unit"], "track")
        self.assertIn("--permutation-unit family-block", self.runbook)
        self.assertIn("Track-wise significance cannot rescue", self.runbook)

    def test_golden_axe_counts_match_documentary_policy(self) -> None:
        tracks = self.policy["golden_axe_iii_track_resolved_world"]["tracks"]
        composers = collections.Counter(
            item["composer"] for item in tracks if "composer" in item
        )
        conflicts = [item for item in tracks if item["credit_state"] == "conflict"]
        frozen = self.prereg["control_worlds"]["golden_axe_iii"]

        self.assertEqual(frozen["maeda"], composers["Tatsuyuki Maeda"])
        self.assertEqual(frozen["hataya"], composers["Naofumi Hataya"])
        self.assertEqual(frozen["oguro"], composers["Haruyo Oguro"])
        self.assertEqual(frozen["uncontested_sawada"], composers["Tomonori Sawada"])
        self.assertEqual(frozen["quarantined_conflicts"], len(conflicts))
        self.assertEqual(conflicts[0]["use"], "quarantined_conflict")

    def test_sonic3d_complete_role_counts_match_policy(self) -> None:
        world = self.policy["sonic_3d_blast_exact_track_world"]
        credits = world["track_credits"]
        composers = collections.Counter(item["composer"] for item in credits)
        arrangers = collections.Counter(item["arranger_programmer"] for item in credits)
        frozen = self.prereg["control_worlds"]["sonic_3d_blast_complete"]

        self.assertEqual(frozen["track_count"], len(credits))
        self.assertEqual(frozen["composition"], dict(composers))
        self.assertEqual(frozen["arrangement_programming"], dict(arrangers))
        specificity = world["role_specificity_policy"]
        self.assertEqual(
            set(frozen["composition_learnable_classes"]),
            set(specificity["composer"]["learnable_classes"]),
        )
        self.assertEqual(
            set(frozen["composition_singleton_sentinels"]),
            set(specificity["composer"]["singleton_sentinels"]),
        )

    def test_direct_mapping_replication_counts_are_derived_from_selected_rows(self) -> None:
        credits = self.policy["sonic_3d_blast_exact_track_world"]["track_credits"]
        selected = [
            item
            for item in credits
            if item["mapping_state"] != "derived_audio_correspondence"
        ]
        composers = collections.Counter(item["composer"] for item in selected)
        arrangers = collections.Counter(item["arranger_programmer"] for item in selected)
        frozen = self.prereg["control_worlds"][
            "sonic_3d_blast_direct_mapping_replication"
        ]

        self.assertEqual(frozen["excluded_mapping_states"], ["derived_audio_correspondence"])
        self.assertEqual(frozen["track_count"], len(selected))
        self.assertEqual(frozen["track_count"], 22)
        self.assertEqual(frozen["composition"], dict(composers))
        self.assertEqual(frozen["arrangement_programming"], dict(arrangers))

    def test_family_policy_is_complete_and_primary_null_preserves_it(self) -> None:
        world = self.policy["sonic_3d_blast_exact_track_world"]
        family_rows = self.families["tracks"]
        self.assertEqual(len(family_rows), world["corpus_fixture_count"])
        self.assertEqual(
            {row["fixture_path"] for row in family_rows},
            {row["fixture_path"] for row in world["track_credits"]},
        )
        self.assertIn(
            "same track count",
            self.prereg["primary_null_policy"]["sonic3d_role_specificity"],
        )
        self.assertIn(
            "cannot replace or rescue",
            self.prereg["primary_null_policy"]["track_level_role_shuffle"],
        )

    def test_method_paths_exist(self) -> None:
        for relative in self.prereg["method_paths"]:
            self.assertTrue((ROOT / relative).is_file(), relative)


if __name__ == "__main__":
    unittest.main()
