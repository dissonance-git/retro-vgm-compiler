import importlib.util
import pathlib
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "maeda_calibration_eval.py"

spec = importlib.util.spec_from_file_location("maeda_calibration_eval", TOOL_PATH)
evaluator = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = evaluator
assert spec.loader is not None
spec.loader.exec_module(evaluator)


class MaedaCalibrationEvalTest(unittest.TestCase):
    def test_precision_at_k_can_require_cross_soundtrack_retrieval(self):
        tracks = {
            "a::p1": {"soundtrack_id": "a", "latent": 0.00},
            "a::n1": {"soundtrack_id": "a", "latent": 0.90},
            "b::p2": {"soundtrack_id": "b", "latent": 0.05},
            "b::n2": {"soundtrack_id": "b", "latent": 0.95},
        }
        score = lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        result = evaluator._precision_at_k(
            tracks,
            {"a::p1", "b::p2"},
            {"a::n1", "b::n2"},
            score,
            1,
            cross_soundtrack_only=True,
        )

        self.assertEqual(result["positive_queries"], 2)
        self.assertEqual(result["precision_at_k"], 1.0)
        self.assertEqual(result["chance_precision_at_k"], 0.5)
        self.assertEqual(result["precision_lift_over_chance"], 0.5)
        self.assertEqual(result["mean_reciprocal_rank"], 1.0)
        for query in result["queries"]:
            self.assertEqual(query["chance_positive_fraction"], 0.5)
            self.assertEqual(query["precision_lift_over_chance"], 0.5)
            self.assertTrue(query["top"][0]["is_positive"])

    def test_blind_contract_and_heldout_target_are_required(self):
        policy = {
            "sonic3_target_policy": {
                "target_environment": "tests/corpus/sonic-3-knuckles"
            }
        }
        bad_model = {
            "model": "creator-labelled audit",
            "label_policy": "No composer/artist metadata or candidate labels are read.",
            "soundtracks": [],
            "tracks": [],
        }
        with self.assertRaisesRegex(ValueError, "creator-blind cross-soundtrack audit"):
            evaluator._validate_blind_audit(bad_model, policy)

        leaked_target = {
            "model": evaluator.BLIND_AUDIT_MODEL,
            "label_policy": "No composer/artist metadata or candidate labels are read.",
            "soundtracks": ["sonic-3-knuckles"],
            "tracks": [],
        }
        with self.assertRaisesRegex(ValueError, "held-out Sonic 3 target"):
            evaluator._validate_blind_audit(leaked_target, policy)

    def test_incomplete_sonic_3d_panel_is_rejected(self):
        policy = {
            "sonic_3d_blast_exact_track_world": {
                "corpus_id": "sonic-3d-blast-genesis-vgm",
                "partition_complete_for_candidate": True,
                "corpus_fixture_count": 3,
                "maeda_fixtures": [
                    "tests/corpus/sonic-3d-blast-genesis-vgm/Maeda.vgm"
                ],
            }
        }
        tracks = {
            "sonic-3d-blast-genesis-vgm::Maeda.vgm": {
                "soundtrack_id": "sonic-3d-blast-genesis-vgm"
            },
            "sonic-3d-blast-genesis-vgm::Other.vgm": {
                "soundtrack_id": "sonic-3d-blast-genesis-vgm"
            },
        }

        with self.assertRaisesRegex(ValueError, "frozen audit is incomplete"):
            evaluator._sonic_3d_partition(policy, tracks)

    def test_evaluate_quarantines_conflict_and_separates_platform_stress(self):
        policy = {
            "candidate": "Tatsuyuki Maeda",
            "golden_axe_iii_track_resolved_world": {
                "corpus_id": "golden-axe-iii-genesis-vgz",
                "tracks": [
                    {
                        "fixture_path": "tests/corpus/golden-axe-iii-genesis-vgz/Maeda.vgz",
                        "credit_state": "derived",
                        "composer": "Tatsuyuki Maeda",
                    },
                    {
                        "fixture_path": "tests/corpus/golden-axe-iii-genesis-vgz/Other.vgz",
                        "credit_state": "derived",
                        "composer": "Someone Else",
                    },
                    {
                        "fixture_path": "tests/corpus/golden-axe-iii-genesis-vgz/Conflict.vgz",
                        "credit_state": "conflict",
                        "use": "quarantined_conflict",
                    },
                ],
            },
            "sonic_3d_blast_exact_track_world": {
                "corpus_id": "sonic-3d-blast-genesis-vgm",
                "partition_complete_for_candidate": True,
                "maeda_fixtures": [
                    "tests/corpus/sonic-3d-blast-genesis-vgm/Maeda.vgm"
                ],
            },
            "whole_soundtrack_worlds": [
                {
                    "corpus_id": "j-league-pro-striker-2-vgz",
                    "platform_id": "mega-drive",
                    "fixtures": [
                        "tests/corpus/j-league-pro-striker-2-vgz/Maeda.vgz"
                    ],
                },
                {
                    "corpus_id": "super-columns-vgm",
                    "platform_id": "game-gear",
                    "fixtures": ["tests/corpus/super-columns-vgm/Maeda.vgm"],
                },
            ],
            "sonic3_target_policy": {
                "target_environment": "tests/corpus/sonic-3-knuckles"
            },
        }
        audit = {
            "model": evaluator.BLIND_AUDIT_MODEL,
            "label_policy": "No composer/artist metadata or candidate labels are read.",
            "soundtracks": [
                "golden-axe-iii-genesis-vgz",
                "j-league-pro-striker-2-vgz",
                "sonic-3d-blast-genesis-vgm",
            ],
            "tracks": [
                {
                    "soundtrack_id": "golden-axe-iii-genesis-vgz",
                    "file": "Maeda.vgz",
                    "latent": 0.00,
                    "realization_latent": 0.00,
                },
                {
                    "soundtrack_id": "golden-axe-iii-genesis-vgz",
                    "file": "Other.vgz",
                    "latent": 0.90,
                    "realization_latent": 0.90,
                },
                {
                    "soundtrack_id": "sonic-3d-blast-genesis-vgm",
                    "file": "Maeda.vgm",
                    "latent": 0.05,
                    "realization_latent": 0.10,
                },
                {
                    "soundtrack_id": "sonic-3d-blast-genesis-vgm",
                    "file": "Other.vgm",
                    "latent": 0.95,
                    "realization_latent": 0.85,
                },
                {
                    "soundtrack_id": "j-league-pro-striker-2-vgz",
                    "file": "Maeda.vgz",
                    "latent": 0.10,
                    "realization_latent": 0.20,
                },
            ],
        }

        original_structural = evaluator.base.structural_similarity
        original_pitch = evaluator.structural_pitch_similarity
        original_rhythm = evaluator.structural_rhythm_similarity
        original_realization = evaluator.base.realization_similarity
        synthetic_structural = (
            lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        )
        evaluator.base.structural_similarity = synthetic_structural
        evaluator.structural_pitch_similarity = synthetic_structural
        evaluator.structural_rhythm_similarity = synthetic_structural
        evaluator.base.realization_similarity = (
            lambda left, right: 1.0
            - abs(left["realization_latent"] - right["realization_latent"])
        )
        try:
            result = evaluator.evaluate(audit, policy, k=1)
        finally:
            evaluator.base.structural_similarity = original_structural
            evaluator.structural_pitch_similarity = original_pitch
            evaluator.structural_rhythm_similarity = original_rhythm
            evaluator.base.realization_similarity = original_realization

        self.assertIn("validated as creator-blind", result["label_policy"])
        self.assertIn(
            "golden-axe-iii-genesis-vgz::Conflict.vgz",
            result["quarantined_controls"],
        )
        self.assertEqual(len(result["unsupported_cross_platform_worlds"]), 1)
        self.assertEqual(
            result["unsupported_cross_platform_worlds"][0]["corpus_id"],
            "super-columns-vgm",
        )
        self.assertEqual(
            result["views"]["structural"]["genesis_cross_soundtrack"]["precision_at_k"],
            1.0,
        )
        self.assertGreater(
            result["views"]["structural"]["genesis_cross_soundtrack"]["precision_lift_over_chance"],
            0.0,
        )
        self.assertEqual(
            result["views"]["realization"]["genesis_cross_soundtrack"]["precision_at_k"],
            1.0,
        )
        self.assertGreater(
            result["views"]["realization"]["genesis_cross_soundtrack"]["precision_lift_over_chance"],
            0.0,
        )
        self.assertIn("does not assign Sonic 3 authorship", result["claim_boundary"])

    def test_repository_policy_declares_complete_sonic_3d_partition(self):
        import json

        policy_path = REPO_ROOT / "research" / "projects" / "sonic3" / "maeda-calibration-policy.json"
        policy = json.loads(policy_path.read_text(encoding="utf-8"))
        world = policy["sonic_3d_blast_exact_track_world"]

        self.assertTrue(world["partition_complete_for_candidate"])
        self.assertEqual(world["corpus_fixture_count"], 24)
        self.assertEqual(len(world["maeda_fixtures"]), 10)
        self.assertEqual(
            next(
                item for item in policy["whole_soundtrack_worlds"]
                if item["corpus_id"] == "super-columns-vgm"
            )["evaluation_use"],
            "future_cross_platform_stress",
        )


if __name__ == "__main__":
    unittest.main()
