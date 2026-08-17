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


class MaedaLeaveWorldOutTest(unittest.TestCase):
    def test_cross_soundtrack_queries_are_exposed_per_held_out_world(self):
        tracks = {
            "world-a::p1": {"soundtrack_id": "world-a", "latent": 0.00},
            "world-a::n1": {"soundtrack_id": "world-a", "latent": 0.90},
            "world-b::p2": {"soundtrack_id": "world-b", "latent": 0.05},
            "world-b::n2": {"soundtrack_id": "world-b", "latent": 0.95},
        }
        score = lambda left, right: 1.0 - abs(left["latent"] - right["latent"])
        metric = evaluator._precision_at_k(
            tracks,
            {"world-a::p1", "world-b::p2"},
            {"world-a::n1", "world-b::n2"},
            score,
            1,
            cross_soundtrack_only=True,
        )
        worlds = evaluator._summarize_query_worlds(metric)

        self.assertEqual(set(worlds), {"world-a", "world-b"})
        for world in worlds.values():
            self.assertEqual(world["query_count"], 1)
            self.assertEqual(world["precision_at_k"], 1.0)
            self.assertEqual(world["chance_precision_at_k"], 0.5)
            self.assertEqual(world["precision_lift_over_chance"], 0.5)
            self.assertEqual(world["mean_reciprocal_rank"], 1.0)
            self.assertTrue(world["same_soundtrack_candidates_excluded"])


if __name__ == "__main__":
    unittest.main()
