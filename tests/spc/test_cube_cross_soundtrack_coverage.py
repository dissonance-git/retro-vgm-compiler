from __future__ import annotations

import importlib.util
import pathlib
import sys
import unittest
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "evaluate_cube_calibration.py"
SPEC = importlib.util.spec_from_file_location("evaluate_cube_cross_soundtrack_test", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
evaluate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = evaluate
SPEC.loader.exec_module(evaluate)


def control(cue_id: str, candidate: str, soundtrack: str):
    return evaluate.GroundedControl(
        cue_id=cue_id,
        fixture_path=f"tests/corpus/{soundtrack}/{cue_id}.spc",
        candidate=candidate,
        soundtrack_id=soundtrack,
        work_family_id=cue_id,
        confidence=1.0,
        status="exact",
    )


class CubeCrossSoundtrackCoverageTest(unittest.TestCase):
    def test_one_world_candidate_remains_incomplete_until_second_world_exists(self):
        controls = [
            control("t-terra", "Takaoka", "terranigma"),
            control("t-ancient", "Takaoka", "ancient-magic"),
            control("h-terra", "Hikichi", "terranigma"),
        ]
        ids = [item.cue_id for item in controls] + ["query-h-terra"]
        matrix = {left: {right: 0.2 for right in ids} for left in ids}
        for cue_id in ids:
            matrix[cue_id][cue_id] = 1.0
        matrix["query-h-terra"]["h-terra"] = matrix["h-terra"]["query-h-terra"] = 0.90
        matrix["query-h-terra"]["t-terra"] = matrix["t-terra"]["query-h-terra"] = 0.30
        matrix["query-h-terra"]["t-ancient"] = matrix["t-ancient"]["query-h-terra"] = 0.25

        by_candidate = defaultdict(list)
        for item in controls:
            by_candidate[item.candidate].append(item)
        candidates = ["Hikichi", "Takaoka"]
        scores = evaluate.candidate_scores(
            "query-h-terra",
            candidates,
            by_candidate,
            matrix,
            exclude_soundtrack="terranigma",
        )
        ranked = evaluate.rank_scores(scores, evaluate.DEFAULT_MINIMUM_MARGIN)
        self.assertIsNone(scores["Hikichi"])
        self.assertAlmostEqual(scores["Takaoka"], 0.25)
        self.assertFalse(ranked["complete_candidate_coverage"])
        self.assertIsNone(ranked["winner"])
        self.assertFalse(ranked["decisive"])

        second_world = control("h-second", "Hikichi", "second-world")
        controls.append(second_world)
        for row in matrix.values():
            row[second_world.cue_id] = 0.2
        matrix[second_world.cue_id] = {cue_id: 0.2 for cue_id in ids + [second_world.cue_id]}
        matrix[second_world.cue_id][second_world.cue_id] = 1.0
        matrix["query-h-terra"][second_world.cue_id] = 0.85
        matrix[second_world.cue_id]["query-h-terra"] = 0.85

        by_candidate = defaultdict(list)
        for item in controls:
            by_candidate[item.candidate].append(item)
        scores = evaluate.candidate_scores(
            "query-h-terra",
            candidates,
            by_candidate,
            matrix,
            exclude_soundtrack="terranigma",
        )
        ranked = evaluate.rank_scores(scores, evaluate.DEFAULT_MINIMUM_MARGIN)
        self.assertAlmostEqual(scores["Hikichi"], 0.85)
        self.assertAlmostEqual(scores["Takaoka"], 0.25)
        self.assertTrue(ranked["complete_candidate_coverage"])
        self.assertEqual(ranked["winner"], "Hikichi")
        self.assertTrue(ranked["decisive"])

    def test_soundtrack_worlds_are_equal_weight_not_raw_cue_count(self):
        controls = [
            control("a-1", "Candidate", "world-a"),
            control("a-2", "Candidate", "world-a"),
            control("a-3", "Candidate", "world-a"),
            control("b-1", "Candidate", "world-b"),
        ]
        ids = [item.cue_id for item in controls] + ["query"]
        matrix = {left: {right: 0.0 for right in ids} for left in ids}
        for cue_id in ids:
            matrix[cue_id][cue_id] = 1.0
        for cue_id in ("a-1", "a-2", "a-3"):
            matrix["query"][cue_id] = matrix[cue_id]["query"] = 0.90
        matrix["query"]["b-1"] = matrix["b-1"]["query"] = 0.10

        score = evaluate.candidate_affinity("query", controls, matrix)
        self.assertAlmostEqual(score, 0.50)


if __name__ == "__main__":
    unittest.main()
