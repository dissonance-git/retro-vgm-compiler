import importlib.util
import pathlib
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "sonic3d_role_specificity_eval.py"

spec = importlib.util.spec_from_file_location("sonic3d_role_specificity_eval", TOOL_PATH)
role_eval = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = role_eval
assert spec.loader is not None
spec.loader.exec_module(role_eval)


class Sonic3DRoleSpecificityEvalTest(unittest.TestCase):
    def test_leave_one_out_role_retrieval_reports_balanced_accuracy(self):
        tracks = {
            "s3d::m1": {"latent": 0.00},
            "s3d::m2": {"latent": 0.05},
            "s3d::j1": {"latent": 1.00},
            "s3d::j2": {"latent": 1.05},
            "s3d::singleton": {"latent": 3.00},
        }
        labels = {
            "s3d::m1": "Maeda",
            "s3d::m2": "Maeda",
            "s3d::j1": "Senoue",
            "s3d::j2": "Senoue",
            "s3d::singleton": "Setsumaru",
        }
        score = lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        result = role_eval._evaluate_role(
            tracks,
            labels,
            {"Maeda", "Senoue"},
            score,
            sentinel_classes={"Setsumaru"},
        )

        self.assertEqual(result["query_count"], 4)
        self.assertEqual(result["top1_accuracy"], 1.0)
        self.assertEqual(result["balanced_top1_accuracy"], 1.0)
        self.assertEqual(result["mean_reciprocal_rank"], 1.0)
        self.assertEqual(result["sentinel_top1_intrusions"], 0)
        self.assertEqual(result["per_class_recall"], {"Maeda": 1.0, "Senoue": 1.0})

    def test_singleton_can_hurt_score_without_becoming_a_learnable_class(self):
        tracks = {
            "s3d::m1": {"latent": 0.00},
            "s3d::m2": {"latent": 0.50},
            "s3d::j1": {"latent": 2.00},
            "s3d::j2": {"latent": 2.05},
            "s3d::singleton": {"latent": 0.01},
        }
        labels = {
            "s3d::m1": "Maeda",
            "s3d::m2": "Maeda",
            "s3d::j1": "Senoue",
            "s3d::j2": "Senoue",
            "s3d::singleton": "Setsumaru",
        }
        score = lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        result = role_eval._evaluate_role(
            tracks,
            labels,
            {"Maeda", "Senoue"},
            score,
            sentinel_classes={"Setsumaru"},
        )

        self.assertEqual(result["learnable_classes"], ["Maeda", "Senoue"])
        self.assertEqual(result["sentinel_classes"], ["Setsumaru"])
        self.assertGreater(result["sentinel_top1_intrusions"], 0)
        self.assertLess(result["balanced_top1_accuracy"], 1.0)

    def test_full_evaluator_keeps_composition_and_arrangement_geometries_separate(self):
        names = ["m1", "m2", "j1", "j2", "x1", "o1"]
        composer = {
            "m1": "Tatsuyuki Maeda",
            "m2": "Tatsuyuki Maeda",
            "j1": "Jun Senoue",
            "j2": "Jun Senoue",
            "x1": "Masaru Setsumaru",
            "o1": "Seirou Okamoto",
        }
        arranger = {
            "m1": "Tatsuyuki Maeda",
            "m2": "Tatsuyuki Maeda",
            "j1": "Jun Senoue",
            "j2": "Jun Senoue",
            "x1": "Masaru Setsumaru",
            "o1": "Masaru Setsumaru",
        }
        composition_latent = {"m1": 0.00, "m2": 0.05, "j1": 1.00, "j2": 1.05, "x1": 2.0, "o1": 3.0}
        arrangement_latent = {"m1": 0.00, "m2": 0.05, "j1": 1.00, "j2": 1.05, "x1": 2.00, "o1": 2.05}

        policy = {
            "sonic_3d_blast_exact_track_world": {
                "corpus_id": "s3d",
                "corpus_fixture_count": 6,
                "track_credits": [
                    {
                        "fixture_path": f"s3d/{name}.vgm",
                        "composer": composer[name],
                        "arranger_programmer": arranger[name],
                        "mapping_state": "direct_title",
                    }
                    for name in names
                ],
                "role_specificity_policy": {
                    "composer": {
                        "learnable_classes": {
                            "Tatsuyuki Maeda": 2,
                            "Jun Senoue": 2,
                        },
                        "singleton_sentinels": {
                            "Masaru Setsumaru": 1,
                            "Seirou Okamoto": 1,
                        },
                    },
                    "arranger_programmer": {
                        "learnable_classes": {
                            "Tatsuyuki Maeda": 2,
                            "Jun Senoue": 2,
                            "Masaru Setsumaru": 2,
                        }
                    },
                },
            },
            "sonic3_target_policy": {
                "target_environment": "tests/corpus/sonic-3-knuckles"
            },
        }
        audit = {
            "model": role_eval.maeda.BLIND_AUDIT_MODEL,
            "label_policy": "No composer/artist metadata or candidate labels are read.",
            "soundtracks": ["s3d"],
            "tracks": [
                {
                    "soundtrack_id": "s3d",
                    "file": f"{name}.vgm",
                    "composition_latent": composition_latent[name],
                    "arrangement_latent": arrangement_latent[name],
                }
                for name in names
            ],
        }

        old_structural = role_eval.maeda.base.structural_similarity
        old_pitch = role_eval.maeda.structural_pitch_similarity
        old_rhythm = role_eval.maeda.structural_rhythm_similarity
        old_realization = role_eval.maeda.base.realization_similarity
        role_eval.maeda.base.structural_similarity = (
            lambda left, right: 1.0
            - abs(left["composition_latent"] - right["composition_latent"])
        )
        role_eval.maeda.structural_pitch_similarity = role_eval.maeda.base.structural_similarity
        role_eval.maeda.structural_rhythm_similarity = role_eval.maeda.base.structural_similarity
        role_eval.maeda.base.realization_similarity = (
            lambda left, right: 1.0
            - abs(left["arrangement_latent"] - right["arrangement_latent"])
        )
        try:
            result = role_eval.evaluate(audit, policy)
        finally:
            role_eval.maeda.base.structural_similarity = old_structural
            role_eval.maeda.structural_pitch_similarity = old_pitch
            role_eval.maeda.structural_rhythm_similarity = old_rhythm
            role_eval.maeda.base.realization_similarity = old_realization

        composition = result["composition"]["structural"]
        realization = result["arrangement_programming"]["realization"]
        self.assertEqual(composition["query_count"], 4)
        self.assertEqual(composition["balanced_top1_accuracy"], 1.0)
        self.assertEqual(realization["query_count"], 6)
        self.assertEqual(realization["balanced_top1_accuracy"], 1.0)
        self.assertEqual(
            set(realization["learnable_classes"]),
            {"Tatsuyuki Maeda", "Jun Senoue", "Masaru Setsumaru"},
        )
        self.assertIn("cannot support a Sonic 3 composition credit", result["claim_boundary"])


if __name__ == "__main__":
    unittest.main()
