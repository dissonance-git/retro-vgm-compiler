from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock


ROOT = pathlib.Path(__file__).resolve().parents[2]
CAPTURE_TOOL = ROOT / "tools" / "spc" / "capture_blind_panel.py"
EVALUATE_TOOL = ROOT / "tools" / "spc" / "evaluate_cube_calibration.py"


def load_module(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


capture = load_module("capture_blind_panel", CAPTURE_TOOL)
evaluate = load_module("evaluate_cube_calibration", EVALUATE_TOOL)


class CubeBlindPanelTest(unittest.TestCase):
    def write_json(self, root: pathlib.Path, name: str, value) -> pathlib.Path:
        path = root / name
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_blind_panel_rejects_creator_bearing_keys(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            panel = {
                "cues": [
                    {
                        "cue_id": "cue-001",
                        "fixture_path": "tests/corpus/a/01.spc",
                        "composer": "poison",
                    },
                    {"cue_id": "cue-002", "fixture_path": "tests/corpus/b/01.spc"},
                ]
            }
            with self.assertRaises(ValueError):
                capture.load_panel(self.write_json(root, "panel.json", panel))

    def test_blind_panel_requires_opaque_unique_spc_fixtures(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            bad_id = {
                "cues": [
                    {"cue_id": "takaoka-001", "fixture_path": "tests/corpus/a/01.spc"},
                    {"cue_id": "cue-002", "fixture_path": "tests/corpus/b/01.spc"},
                ]
            }
            with self.assertRaises(ValueError):
                capture.load_panel(self.write_json(root, "bad-id.json", bad_id))

            bad_suffix = {
                "cues": [
                    {"cue_id": "cue-001", "fixture_path": "tests/corpus/a/01.mp3"},
                    {"cue_id": "cue-002", "fixture_path": "tests/corpus/b/01.spc"},
                ]
            }
            with self.assertRaises(ValueError):
                capture.load_panel(self.write_json(root, "bad-suffix.json", bad_suffix))

    def test_panel_ids_are_downstream_of_song_cache_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = pathlib.Path(temp)
            fixture_rel = pathlib.PurePosixPath("tests/corpus/world-spc/01 - Cue.spc")
            fixture = repo / pathlib.Path(*fixture_rel.parts)
            fixture.parent.mkdir(parents=True)
            fixture.write_bytes(b"spc")

            extractor = repo / "spc_forensic_features"
            extractor.write_bytes(b"extractor")
            freeze_tool = repo / "freeze.py"
            freeze_tool.write_text("# placeholder\n", encoding="utf-8")
            cache_root = repo / "research/cache/spc-song-capsules"
            cached = cache_root / "world-spc/5s/01 - Cue.spc.json"
            cached.parent.mkdir(parents=True)
            cached.write_text('{"model":"cached"}', encoding="utf-8")

            with mock.patch.object(
                capture.spc_cache,
                "build_one",
                return_value=(cached, False),
            ) as build_one, mock.patch.object(capture.subprocess, "run") as run:
                for cue_id in ("cue-001", "cue-999"):
                    output = repo / f"panel-{cue_id}"
                    capture.capture_panel(
                        [capture.PanelCue(cue_id=cue_id, fixture_path=fixture_rel)],
                        repo_root=repo,
                        extractor=extractor,
                        output_dir=output,
                        seconds=5,
                        freeze_tool=freeze_tool,
                        freeze_output=repo / f"freeze-{cue_id}.json",
                        cache_root=cache_root,
                    )
                    self.assertEqual(
                        (output / f"{cue_id}.json").read_text(encoding="utf-8"),
                        cached.read_text(encoding="utf-8"),
                    )

            self.assertEqual(build_one.call_count, 2)
            destinations = [call.args[0] for call in build_one.call_args_list]
            self.assertEqual(destinations, [fixture.resolve(), fixture.resolve()])
            self.assertEqual(run.call_count, 2)

    def synthetic_fixture(self):
        fixture_by_cue = {
            "cue-001": "tests/corpus/world-a/01.spc",
            "cue-002": "tests/corpus/world-b/01.spc",
            "cue-003": "tests/corpus/world-b/02.spc",
            "cue-004": "tests/corpus/world-c/01.spc",
        }
        matrix = {
            "cue-001": {"cue-001": 1.0, "cue-002": 0.90, "cue-003": 0.20, "cue-004": 0.80},
            "cue-002": {"cue-001": 0.90, "cue-002": 1.0, "cue-003": 0.30, "cue-004": 0.70},
            "cue-003": {"cue-001": 0.20, "cue-002": 0.30, "cue-003": 1.0, "cue-004": 0.25},
            "cue-004": {"cue-001": 0.80, "cue-002": 0.70, "cue-003": 0.25, "cue-004": 1.0},
        }
        controls = [
            evaluate.GroundedControl(
                cue_id="cue-001",
                fixture_path=fixture_by_cue["cue-001"],
                candidate="A",
                soundtrack_id="world-a",
                work_family_id="a1",
                confidence=1.0,
                status="exact",
            ),
            evaluate.GroundedControl(
                cue_id="cue-002",
                fixture_path=fixture_by_cue["cue-002"],
                candidate="A",
                soundtrack_id="world-b",
                work_family_id="a2",
                confidence=1.0,
                status="exact",
            ),
            evaluate.GroundedControl(
                cue_id="cue-003",
                fixture_path=fixture_by_cue["cue-003"],
                candidate="B",
                soundtrack_id="world-b",
                work_family_id="b1",
                confidence=1.0,
                status="exact",
            ),
        ]
        policy = {
            "evaluation_only_clues": [],
            "shared_composition_holdouts": [],
            "disputed_mapping_holdouts": [],
            "third_party_decoys": [],
        }
        return fixture_by_cue, matrix, controls, policy

    def test_strict_holdout_fails_closed_when_candidate_has_no_other_soundtrack(self):
        fixture_by_cue, matrix, controls, policy = self.synthetic_fixture()
        result = evaluate.evaluate(
            matrix=matrix,
            frozen_sha256="abc",
            fixture_by_cue=fixture_by_cue,
            controls=controls,
            policy=policy,
            minimum_margin=0.08,
            false_positive_threshold=0.65,
        )
        by_cue = {
            item["cue_id"]: item
            for item in result["grounded_evaluation"]["cues"]
        }

        world_a = by_cue["cue-001"]["strict_leave_soundtrack_out"]
        self.assertTrue(world_a["complete_candidate_coverage"])
        self.assertTrue(world_a["decisive"])
        self.assertEqual(world_a["winner"], "A")
        self.assertTrue(world_a["correct_if_decisive"])

        world_b_b = by_cue["cue-003"]["strict_leave_soundtrack_out"]
        self.assertFalse(world_b_b["complete_candidate_coverage"])
        self.assertFalse(world_b_b["decisive"])
        self.assertIsNone(world_b_b["winner"])
        self.assertIsNone(world_b_b["candidate_scores"]["B"])

        limitations = " ".join(result["limitations"])
        self.assertIn("B has grounded composer controls in only 1 soundtrack world", limitations)

    def test_unlabeled_world_reports_affinity_without_correctness_claim(self):
        fixture_by_cue, matrix, controls, policy = self.synthetic_fixture()
        result = evaluate.evaluate(
            matrix=matrix,
            frozen_sha256="abc",
            fixture_by_cue=fixture_by_cue,
            controls=controls,
            policy=policy,
            minimum_margin=0.08,
            false_positive_threshold=0.65,
        )
        validation = result["validation_worlds"]["world-c"]
        self.assertEqual(validation["cue_count"], 1)
        cue = validation["cues"][0]
        self.assertEqual(cue["winner"], "A")
        self.assertNotIn("correct_if_decisive", cue)
        self.assertIn("not an authorship result", cue["claim_boundary"])


if __name__ == "__main__":
    unittest.main()
