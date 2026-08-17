import importlib.util
import pathlib
import random
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "maeda_calibration_null.py"

spec = importlib.util.spec_from_file_location("maeda_calibration_null", TOOL_PATH)
null_model = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = null_model
assert spec.loader is not None
spec.loader.exec_module(null_model)


def feature(soundtrack_id, file_name, positive):
    interval = "2" if positive else "-3"
    bigram = "2,2" if positive else "-3,-3"
    contour = "up" if positive else "down"
    gap = "1.00" if positive else "2.00"
    patch = "maeda-ish" if positive else "other-ish"
    algorithm = "1" if positive else "6"
    return {
        "soundtrack_id": soundtrack_id,
        "file": file_name,
        "musical_trajectory": {
            "interval_histogram_semitones": {interval: 4},
            "interval_bigram_histogram": {bigram: 3},
            "contour_histogram": {contour: 4},
            "normalized_onset_gap_histogram": {gap: 4},
        },
        "realization": {
            "core_patch_usage": {patch: 4},
            "algorithm_histogram": {algorithm: 4},
            "feedback_histogram": {algorithm: 4},
            "pan_histogram": {"3": 4},
        },
    }


class MaedaCalibrationNullTest(unittest.TestCase):
    def test_partition_shuffle_preserves_counts_and_is_seeded(self):
        pool = {"a", "b", "c", "d", "e"}
        first = null_model._permute_partition(pool, 2, random.Random(7))
        second = null_model._permute_partition(pool, 2, random.Random(7))

        self.assertEqual(first, second)
        self.assertEqual(len(first[0]), 2)
        self.assertEqual(len(first[1]), 3)
        self.assertEqual(first[0] | first[1], pool)
        self.assertFalse(first[0] & first[1])

    def test_empirical_p_uses_plus_one_correction(self):
        summary = null_model._null_summary(0.5, [0.1, 0.5, 0.9])
        self.assertEqual(summary["permutations"], 3)
        self.assertEqual(summary["empirical_p_greater_or_equal"], 0.75)

    def test_small_complete_panel_is_reproducible_and_keeps_worlds_separate(self):
        policy = {
            "candidate": "Tatsuyuki Maeda",
            "golden_axe_iii_track_resolved_world": {
                "corpus_id": "ga",
                "tracks": [
                    {"fixture_path": "ga/P1.vgz", "composer": "Tatsuyuki Maeda"},
                    {"fixture_path": "ga/P2.vgz", "composer": "Tatsuyuki Maeda"},
                    {"fixture_path": "ga/N1.vgz", "composer": "Other"},
                    {"fixture_path": "ga/N2.vgz", "composer": "Other"},
                    {
                        "fixture_path": "ga/Conflict.vgz",
                        "credit_state": "conflict",
                        "use": "quarantined_conflict",
                    },
                ],
            },
            "sonic_3d_blast_exact_track_world": {
                "corpus_id": "s3d",
                "partition_complete_for_candidate": True,
                "corpus_fixture_count": 4,
                "maeda_fixtures": ["s3d/P1.vgm", "s3d/P2.vgm"],
            },
            "whole_soundtrack_worlds": [
                {
                    "corpus_id": "jleague",
                    "platform_id": "mega-drive",
                    "fixtures": ["jleague/P1.vgz", "jleague/P2.vgz"],
                }
            ],
            "sonic3_target_policy": {
                "target_environment": "tests/corpus/sonic-3-knuckles"
            },
        }
        audit = {
            "model": null_model.evaluator.BLIND_AUDIT_MODEL,
            "label_policy": "No composer/artist metadata or candidate labels are read.",
            "soundtracks": ["ga", "s3d", "jleague"],
            "tracks": [
                feature("ga", "P1.vgz", True),
                feature("ga", "P2.vgz", True),
                feature("ga", "N1.vgz", False),
                feature("ga", "N2.vgz", False),
                feature("s3d", "P1.vgm", True),
                feature("s3d", "P2.vgm", True),
                feature("s3d", "N1.vgm", False),
                feature("s3d", "N2.vgm", False),
                feature("jleague", "P1.vgz", True),
                feature("jleague", "P2.vgz", True),
            ],
        }

        first = null_model.permutation_null(
            audit, policy, k=1, permutations=12, seed=17
        )
        second = null_model.permutation_null(
            audit, policy, k=1, permutations=12, seed=17
        )

        self.assertEqual(first, second)
        self.assertEqual(first["permutations"], 12)
        self.assertIn("ga::Conflict.vgz", first["quarantined_controls"])
        self.assertEqual(
            set(first["views"]),
            {"structural", "structural_pitch", "structural_rhythm", "realization"},
        )
        worlds = first["views"]["structural"][
            "genesis_cross_soundtrack_by_query_world"
        ]
        self.assertEqual(set(worlds), {"ga", "s3d", "jleague"})
        for world in worlds.values():
            p = world["precision_lift_over_chance"][
                "empirical_p_greater_or_equal"
            ]
            self.assertGreater(p, 0.0)
            self.assertLessEqual(p, 1.0)


if __name__ == "__main__":
    unittest.main()
