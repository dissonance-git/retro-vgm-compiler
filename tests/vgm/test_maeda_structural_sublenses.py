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


def track(interval, bigram, contour, gap):
    return {
        "musical_trajectory": {
            "interval_histogram_semitones": {interval: 4},
            "interval_bigram_histogram": {bigram: 3},
            "contour_histogram": {contour: 4},
            "normalized_onset_gap_histogram": {gap: 4},
        }
    }


class MaedaStructuralSublensesTest(unittest.TestCase):
    def test_pitch_and_rhythm_can_disagree_without_collapsing(self):
        query = track("2", "2,2", "up", "1.00")
        same_pitch = track("2", "2,2", "up", "2.00")
        same_rhythm = track("-3", "-3,-3", "down", "1.00")

        self.assertEqual(evaluator.structural_pitch_similarity(query, same_pitch), 1.0)
        self.assertEqual(evaluator.structural_rhythm_similarity(query, same_pitch), 0.0)
        self.assertEqual(evaluator.structural_pitch_similarity(query, same_rhythm), 0.0)
        self.assertEqual(evaluator.structural_rhythm_similarity(query, same_rhythm), 1.0)

    def test_empty_rhythm_evidence_returns_zero_not_false_identity(self):
        query = track("2", "2,2", "up", "1.00")
        empty = track("2", "2,2", "up", "unused")
        empty["musical_trajectory"]["normalized_onset_gap_histogram"] = {}

        self.assertEqual(evaluator.structural_rhythm_similarity(query, empty), 0.0)
        self.assertEqual(evaluator.structural_pitch_similarity(query, empty), 1.0)


if __name__ == "__main__":
    unittest.main()
