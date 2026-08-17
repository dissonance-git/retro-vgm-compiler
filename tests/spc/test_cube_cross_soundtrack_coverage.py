from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import unittest
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "evaluate_cube_calibration.py"
ADMISSIONS = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"
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


def current_cube_admissions() -> list[dict]:
    return [
        json.loads(line)
        for line in ADMISSIONS.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]


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

    def test_current_admissions_have_exactly_three_strict_complete_queries(self):
        admissions = [
            entry
            for entry in current_cube_admissions()
            if entry.get("role") == "composer"
            and entry.get("status") in {"exact", "derived"}
            and entry.get("candidate") in {"Miyoko Takaoka", "Masanori Hikichi"}
        ]
        self.assertEqual(len(admissions), 15)

        candidate_worlds: dict[str, set[str]] = defaultdict(set)
        query_rows: list[tuple[str, str, str]] = []
        for entry in admissions:
            soundtrack = pathlib.PurePosixPath(entry["fixture_path"]).parts[2]
            candidate_worlds[entry["candidate"]].add(soundtrack)
            query_rows.append((entry["fixture_path"], entry["candidate"], soundtrack))

        candidates = {"Miyoko Takaoka", "Masanori Hikichi"}
        complete = []
        incomplete = []
        for fixture_path, candidate, query_soundtrack in query_rows:
            has_all_candidates_after_exclusion = all(
                any(world != query_soundtrack for world in candidate_worlds[other_candidate])
                for other_candidate in candidates
            )
            bucket = complete if has_all_candidates_after_exclusion else incomplete
            bucket.append((fixture_path, candidate, query_soundtrack))

        self.assertEqual(len(complete), 3)
        self.assertEqual(len(incomplete), 12)
        self.assertEqual(
            {row[2] for row in complete},
            {"ancient-magic-spc"},
        )
        self.assertEqual(
            {row[1] for row in complete},
            {"Miyoko Takaoka"},
        )
        self.assertEqual(
            {row[2] for row in incomplete},
            {"terranigma-spc"},
        )

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
