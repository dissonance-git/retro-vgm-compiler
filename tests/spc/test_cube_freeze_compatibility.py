from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "evaluate_cube_calibration.py"
SPEC = importlib.util.spec_from_file_location("evaluate_cube_calibration", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
evaluate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = evaluate
SPEC.loader.exec_module(evaluate)


class CubeFreezeCompatibilityTest(unittest.TestCase):
    def write_freeze(self, root: pathlib.Path, model: str) -> pathlib.Path:
        value = {
            "model": model,
            "cues": [
                {"cue_id": "cue-001"},
                {"cue_id": "cue-002"},
            ],
            "similarity_matrix": {
                "cue-001": {"cue-001": 1.0, "cue-002": 0.4},
                "cue-002": {"cue-001": 0.4, "cue-002": 1.0},
            },
        }
        path = root / (model.replace(" ", "-") + ".json")
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_legacy_and_representation_neutral_freezes_are_both_readable(self):
        expected = {"cue-001", "cue-002"}
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            for model in (
                evaluate.LEGACY_FREEZE_MODEL,
                evaluate.CURRENT_FREEZE_MODEL,
            ):
                matrix, digest = evaluate.load_frozen(
                    self.write_freeze(root, model),
                    expected,
                )
                self.assertEqual(matrix["cue-001"]["cue-002"], 0.4)
                self.assertEqual(len(digest), 64)

    def test_unknown_freeze_model_fails_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            with self.assertRaisesRegex(ValueError, "supported creator-blind motif"):
                evaluate.load_frozen(
                    self.write_freeze(root, "untrusted future freeze"),
                    {"cue-001", "cue-002"},
                )

    def test_supported_freeze_still_requires_exact_symmetric_panel_matrix(self):
        expected = {"cue-001", "cue-002"}
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            path = self.write_freeze(root, evaluate.CURRENT_FREEZE_MODEL)
            value = json.loads(path.read_text(encoding="utf-8"))
            value["similarity_matrix"]["cue-002"]["cue-001"] = 0.41
            path.write_text(json.dumps(value), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "symmetric"):
                evaluate.load_frozen(path, expected)


if __name__ == "__main__":
    unittest.main()
