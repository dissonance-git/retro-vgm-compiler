import collections
import importlib.util
import pathlib
import random
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "sonic3d_role_specificity_null.py"

spec = importlib.util.spec_from_file_location("sonic3d_role_specificity_null", TOOL_PATH)
null_model = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = null_model
assert spec.loader is not None
spec.loader.exec_module(null_model)


class Sonic3DRoleSpecificityNullTest(unittest.TestCase):
    def test_shuffle_is_seeded_and_preserves_exact_label_multiset(self):
        labels = {
            "a": "Maeda",
            "b": "Maeda",
            "c": "Senoue",
            "d": "Setsumaru",
        }
        first = null_model._shuffle_labels(labels, random.Random(17))
        second = null_model._shuffle_labels(labels, random.Random(17))

        self.assertEqual(first, second)
        self.assertEqual(collections.Counter(first.values()), collections.Counter(labels.values()))
        self.assertEqual(set(first), set(labels))

    def test_empirical_p_uses_plus_one_correction(self):
        summary = null_model._null_summary(0.5, [0.1, 0.5, 0.9])
        self.assertEqual(summary["permutations"], 3)
        self.assertEqual(summary["empirical_p_greater_or_equal"], 0.75)

    def test_null_is_reproducible_and_keeps_role_lanes_and_families_fixed(self):
        names = ["m1", "m2", "m3", "j1", "j2", "j3", "x1", "o1"]
        composer = {
            "m1": "Tatsuyuki Maeda",
            "m2": "Tatsuyuki Maeda",
            "m3": "Tatsuyuki Maeda",
            "j1": "Jun Senoue",
            "j2": "Jun Senoue",
            "j3": "Jun Senoue",
            "x1": "Masaru Setsumaru",
            "o1": "Seirou Okamoto",
        }
        arranger = {
            "m1": "Tatsuyuki Maeda",
            "m2": "Tatsuyuki Maeda",
            "m3": "Jun Senoue",
            "j1": "Jun Senoue",
            "j2": "Jun Senoue",
            "j3": "Masaru Setsumaru",
            "x1": "Masaru Setsumaru",
            "o1": "Masaru Setsumaru",
        }
        composition_latent = {
            "m1": 0.00,
            "m2": 0.05,
            "m3": 0.10,
            "j1": 1.00,
            "j2": 1.05,
            "j3": 1.10,
            "x1": 2.00,
            "o1": 3.00,
        }
        arrangement_latent = {
            "m1": 0.00,
            "m2": 0.05,
            "m3": 1.00,
            "j1": 1.05,
            "j2": 1.10,
            "j3": 2.00,
            "x1": 2.05,
            "o1": 2.10,
        }

        policy = {
            "sonic_3d_blast_exact_track_world": {
                "corpus_id": "s3d",
                "corpus_fixture_count": 8,
                "track_credits": [
                    {
                        "fixture_path": f"s3d/{name}.vgm",
                        "composer": composer[name],
                        "arranger_programmer": arranger[name],
                        "mapping_state": (
                            "derived_audio_correspondence"
                            if name == "m3"
                            else "direct_title"
                        ),
                    }
                    for name in names
                ],
                "role_specificity_policy": {
                    "composer": {
                        "learnable_classes": {
                            "Tatsuyuki Maeda": 3,
                            "Jun Senoue": 3,
                        },
                        "singleton_sentinels": {
                            "Masaru Setsumaru": 1,
                            "Seirou Okamoto": 1,
                        },
                    },
                    "arranger_programmer": {
                        "learnable_classes": {
                            "Tatsuyuki Maeda": 2,
                            "Jun Senoue": 3,
                            "Masaru Setsumaru": 3,
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
            "candidate_exclusion_rule": (
                "For every query, exclude all candidate fixtures with the same family_id."
            ),
            "tracks": [
                {"fixture_path": f"s3d/{name}.vgm", "family_id": f"family-{name}"}
                for name in names
            ],
        }
        audit = {
            "model": null_model.role_eval.maeda.BLIND_AUDIT_MODEL,
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

        maeda = null_model.role_eval.maeda
        old_structural = maeda.base.structural_similarity
        old_pitch = maeda.structural_pitch_similarity
        old_rhythm = maeda.structural_rhythm_similarity
        old_realization = maeda.base.realization_similarity
        composition_score = lambda left, right: 1.0 - abs(
            left["composition_latent"] - right["composition_latent"]
        )
        arrangement_score = lambda left, right: 1.0 - abs(
            left["arrangement_latent"] - right["arrangement_latent"]
        )
        maeda.base.structural_similarity = composition_score
        maeda.structural_pitch_similarity = composition_score
        maeda.structural_rhythm_similarity = composition_score
        maeda.base.realization_similarity = arrangement_score
        try:
            first = null_model.permutation_null(
                audit, policy, family_policy, permutations=20, seed=23
            )
            second = null_model.permutation_null(
                audit, policy, family_policy, permutations=20, seed=23
            )
            sensitivity = null_model.permutation_null(
                audit,
                policy,
                family_policy,
                permutations=20,
                seed=23,
                excluded_mapping_states={"derived_audio_correspondence"},
            )
        finally:
            maeda.base.structural_similarity = old_structural
            maeda.structural_pitch_similarity = old_pitch
            maeda.structural_rhythm_similarity = old_rhythm
            maeda.base.realization_similarity = old_realization

        self.assertEqual(first, second)
        self.assertEqual(first["permutations"], 20)
        self.assertEqual(
            set(first["views"]),
            {"structural", "structural_pitch", "structural_rhythm", "realization"},
        )

        structural = first["views"]["structural"]
        realization = first["views"]["realization"]
        self.assertEqual(structural["lane"], "composition")
        self.assertEqual(realization["lane"], "arrangement_programming")
        self.assertTrue(structural["same_family_candidates_excluded"])
        self.assertEqual(
            structural["observed_label_counts"],
            {
                "Jun Senoue": 3,
                "Masaru Setsumaru": 1,
                "Seirou Okamoto": 1,
                "Tatsuyuki Maeda": 3,
            },
        )
        self.assertEqual(
            realization["observed_label_counts"],
            {
                "Jun Senoue": 3,
                "Masaru Setsumaru": 3,
                "Tatsuyuki Maeda": 2,
            },
        )
        self.assertEqual(
            structural["learnable_classes"],
            ["Jun Senoue", "Tatsuyuki Maeda"],
        )
        self.assertEqual(
            structural["sentinel_classes"],
            ["Masaru Setsumaru", "Seirou Okamoto"],
        )
        for view in first["views"].values():
            for statistic in view["statistics"].values():
                self.assertEqual(statistic["permutations"], 20)
                self.assertGreater(statistic["empirical_p_greater_or_equal"], 0.0)
                self.assertLessEqual(statistic["empirical_p_greater_or_equal"], 1.0)

        self.assertEqual(
            sensitivity["excluded_mapping_states"],
            ["derived_audio_correspondence"],
        )
        self.assertEqual(sensitivity["excluded_tracks"], ["s3d::m3.vgm"])
        self.assertEqual(sensitivity["included_track_count"], 7)
        self.assertEqual(
            sensitivity["views"]["structural"]["observed_label_counts"],
            {
                "Jun Senoue": 3,
                "Masaru Setsumaru": 1,
                "Seirou Okamoto": 1,
                "Tatsuyuki Maeda": 2,
            },
        )
        self.assertEqual(
            sensitivity["views"]["realization"]["observed_label_counts"],
            {
                "Jun Senoue": 2,
                "Masaru Setsumaru": 3,
                "Tatsuyuki Maeda": 2,
            },
        )
        for view in sensitivity["views"].values():
            self.assertTrue(view["same_family_candidates_excluded"])
            for statistic in view["statistics"].values():
                self.assertEqual(statistic["permutations"], 20)


if __name__ == "__main__":
    unittest.main()
