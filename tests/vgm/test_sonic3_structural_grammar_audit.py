from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "sonic3_structural_grammar_audit.py"
SPEC = importlib.util.spec_from_file_location("sonic3_structural_grammar_audit", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
SONIC = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = SONIC
SPEC.loader.exec_module(SONIC)

STRUCTURAL_SCHEMA = "gmi-structural-grammar-observations-v1"


class Sonic3StructuralGrammarAuditTest(unittest.TestCase):
    def _observation(self, soundtrack: str, work: str, confidence: float) -> dict[str, object]:
        return {
            "soundtrack_id": soundtrack,
            "work_family_id": work,
            "representation": "synthesis_runtime",
            "rule_key": "imitation:imitation;lag=1.50",
            "dimension": "counterpoint_voice_leading",
            "role_scope": "composer",
            "confidence": confidence,
            "source": f"blind-{soundtrack}-{work}",
            "detail": "creator-free structural observation",
        }

    def _write(self, path: Path, observations: list[dict[str, object]], **extra: object) -> None:
        payload: dict[str, object] = {
            "schema": STRUCTURAL_SCHEMA,
            "observations": observations,
        }
        payload.update(extra)
        path.write_text(json.dumps(payload), encoding="utf-8")

    def test_selected_target_and_control_can_form_portable_rule(self) -> None:
        selected = sorted(SONIC.selected_sonic3_corpus_ids())
        self.assertIn(SONIC.TARGET_CORPUS_ID, selected)
        control = next(value for value in selected if value != SONIC.TARGET_CORPUS_ID)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "blind.json"
            self._write(
                path,
                [
                    self._observation(SONIC.TARGET_CORPUS_ID, "held-out-sonic-work", 0.84),
                    self._observation(control, "external-control-work", 0.80),
                ],
            )
            result = SONIC.run_sonic3_structural_audit([path])
            self.assertEqual(result["stage"], "blind-structural-grammar-cross-soundtrack")
            self.assertEqual(result["portable_rule_count"], 1)
            self.assertEqual(result["rules"][0]["independent_soundtrack_count"], 2)
            self.assertAlmostEqual(result["rules"][0]["portable_support_ceiling"], 0.80)

    def test_foreign_soundtrack_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "foreign.json"
            self._write(path, [self._observation("not-a-sonic-control", "work", 0.90)])
            with self.assertRaisesRegex(ValueError, "outside the predeclared"):
                SONIC.run_sonic3_structural_audit([path])

    def test_identity_fields_remain_forbidden(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "leaky.json"
            self._write(
                path,
                [self._observation(SONIC.TARGET_CORPUS_ID, "work", 0.90)],
                composer="answer-key-leak",
            )
            with self.assertRaisesRegex(ValueError, "identity-bearing field"):
                SONIC.run_sonic3_structural_audit([path])


if __name__ == "__main__":
    unittest.main()
