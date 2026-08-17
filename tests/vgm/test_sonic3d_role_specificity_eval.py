import importlib.util
import json
import pathlib
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "sonic3d_role_specificity_eval.py"
FAMILY_POLICY = (
    REPO_ROOT
    / "research"
    / "projects"
    / "sonic3"
    / "sonic3d-role-family-policy.json"
)

spec = importlib.util.spec_from_file_location("sonic3d_role_specificity_eval", TOOL_PATH)
role_eval = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = role_eval
assert spec.loader is not None
spec.loader.exec_module(role_eval)


def unique_families(keys):
    return {key: f"family-{index}" for index, key in enumerate(keys)}


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
            families=unique_families(labels),
            sentinel_classes={"Setsumaru"},
        )

        self.assertEqual(result["query_count"], 4)
        self.assertEqual(result["top1_accuracy"], 1.0)
        self.assertEqual(result["balanced_top1_accuracy"], 1.0)
        self.assertEqual(result["mean_reciprocal_rank"], 1.0)
        self.assertEqual(result["sentinel_top1_intrusions"], 0)
        self.assertTrue(result["same_family_candidates_excluded"])
        self.assertEqual(result["per_class_recall"], {"Maeda": 1.0, "Senoue": 1.0})

    def test_same_family_nearest_neighbor_is_forbidden(self):
        tracks = {
            "s3d::m1": {"latent": 0.00},
            "s3d::m-sibling": {"latent": 0.001},
            "s3d::m-independent": {"latent": 0.10},
            "s3d::j1": {"latent": 1.00},
            "s3d::j2": {"latent": 1.05},
        }
        labels = {
            "s3d::m1": "Maeda",
            "s3d::m-sibling": "Maeda",
            "s3d::m-independent": "Maeda",
            "s3d::j1": "Senoue",
            "s3d::j2": "Senoue",
        }
        families = {
            "s3d::m1": "zone-a",
            "s3d::m-sibling": "zone-a",
            "s3d::m-independent": "zone-b",
            "s3d::j1": "zone-c",
            "s3d::j2": "zone-d",
        }
        score = lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        result = role_eval._evaluate_role(
            tracks,
            labels,
            {"Maeda", "Senoue"},
            score,
            families=families,
        )

        row = next(item for item in result["queries"] if item["query"] == "s3d::m1")
        self.assertEqual(row["top1_track"], "s3d::m-independent")
        self.assertNotEqual(row["top1_family"], row["query_family"])
        self.assertTrue(row["same_family_candidates_excluded"])
        self.assertGreater(result["excluded_same_family_candidate_instances"], 0)

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
            families=unique_families(labels),
            sentinel_classes={"Setsumaru"},
        )

        self.assertEqual(result["learnable_classes"], ["Maeda", "Senoue"])
        self.assertEqual(result["sentinel_classes"], ["Setsumaru"])
        self.assertGreater(result["sentinel_top1_intrusions"], 0)
        self.assertLess(result["balanced_top1_accuracy"], 1.0)

    def test_repository_family_policy_covers_exactly_24_fixtures(self):
        family_policy = json.loads(FAMILY_POLICY.read_text(encoding="utf-8"))
        rows = family_policy["tracks"]
        self.assertEqual(family_policy["corpus_id"], "sonic-3d-blast-genesis-vgm")
        self.assertEqual(len(rows), 24)
        paths = [row["fixture_path"] for row in rows]
        self.assertEqual(len(set(paths)), 24)
        for path in paths:
            self.assertTrue((REPO_ROOT / path).is_file(), path)

        by_family = {}
        for row in rows:
            by_family.setdefault(row["family_id"], []).append(pathlib.Path(row["fixture_path"]).name)
        self.assertEqual(
            set(by_family["boss-2-lineage"]),
            {"21 - Robotnik 2.vgm", "22 - Robotnik 3.vgm"},
        )
        self.assertEqual(
            set(by_family["green-grove"]),
            {"03 - Green Grove Zone Act 1.vgm", "04 - Green Grove Zone Act 2.vgm"},
        )
        self.assertIn("exclude all candidate fixtures with the same family_id", family_policy["candidate_exclusion_rule"])

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
        family_policy = {
            "corpus_id": "s3d",
            "candidate_exclusion_rule": "For every query, exclude all candidate fixtures with the same family_id.",
            "tracks": [
                {"fixture_path": f"s3d/{name}.vgm", "family_id": f"family-{name}"}
                for name in names
            ],
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
            result = role_eval.evaluate(audit, policy, family_policy)
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
        self.assertTrue(composition["same_family_candidates_excluded"])
        self.assertEqual(
            set(realization["learnable_classes"]),
            {"Tatsuyuki Maeda", "Jun Senoue", "Masaru Setsumaru"},
        )
        self.assertIn("cannot support a Sonic 3 composition credit", result["claim_boundary"])


if __name__ == "__main__":
    unittest.main()
