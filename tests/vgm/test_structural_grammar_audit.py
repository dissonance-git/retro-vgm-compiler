from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "structural_grammar_audit.py"
SPEC = importlib.util.spec_from_file_location("structural_grammar_audit", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = AUDIT
SPEC.loader.exec_module(AUDIT)


class StructuralGrammarAuditTest(unittest.TestCase):
    def _write(self, path: Path, observations: list[dict[str, object]], **extra: object) -> None:
        payload: dict[str, object] = {
            "schema": AUDIT.SCHEMA,
            "observations": observations,
        }
        payload.update(extra)
        path.write_text(json.dumps(payload), encoding="utf-8")

    def _observation(
        self,
        soundtrack: str,
        work: str,
        confidence: float,
        *,
        rule: str = "bass_harmony:moving_bass_under_retained_upper_material;retained_upper=2",
    ) -> dict[str, object]:
        return {
            "soundtrack_id": soundtrack,
            "work_family_id": work,
            "representation": "synthesis_runtime",
            "rule_key": rule,
            "dimension": "bass_harmony",
            "role_scope": "composer",
            "confidence": confidence,
            "source": f"blind-{soundtrack}-{work}",
            "detail": "creator-free structural observation",
        }

    def test_cross_soundtrack_rule_uses_weaker_independent_support(self) -> None:
        observations = [
            self._observation("soundtrack-a", "work-a", 0.88),
            self._observation("soundtrack-a", "work-b", 0.84),
            self._observation("soundtrack-b", "work-c", 0.79),
        ]
        result = AUDIT.audit_observations(observations)
        self.assertEqual(result["portable_rule_count"], 1)
        rule = result["rules"][0]
        self.assertTrue(rule["cross_work_grounded"])
        self.assertTrue(rule["cross_soundtrack_grounded"])
        self.assertTrue(rule["eligible_for_candidate_aggregation"])
        self.assertEqual(rule["independent_soundtrack_count"], 2)
        self.assertAlmostEqual(rule["portable_support_ceiling"], 0.79)

    def test_weak_cross_soundtrack_echoes_do_not_become_portable(self) -> None:
        observations = [
            self._observation("soundtrack-a", "work-a", 0.55, rule="rhythm-only-echo"),
            self._observation("soundtrack-b", "work-b", 0.55, rule="rhythm-only-echo"),
        ]
        result = AUDIT.audit_observations(observations)
        self.assertEqual(result["portable_rule_count"], 0)
        rule = result["rules"][0]
        self.assertEqual(rule["grounding_observation_count"], 0)
        self.assertFalse(rule["cross_soundtrack_grounded"])
        self.assertFalse(rule["eligible_for_candidate_aggregation"])

    def test_identity_fields_are_rejected_before_audit(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "observations.json"
            self._write(
                path,
                [self._observation("soundtrack-a", "work-a", 0.90)],
                composer="forbidden-answer",
            )
            with self.assertRaisesRegex(ValueError, "identity-bearing field"):
                AUDIT.load_observations([path])

    def test_valid_files_merge_without_identity_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            first = Path(directory) / "a.json"
            second = Path(directory) / "b.json"
            self._write(first, [self._observation("soundtrack-a", "work-a", 0.90)])
            self._write(second, [self._observation("soundtrack-b", "work-b", 0.82)])
            observations = AUDIT.load_observations([first, second])
            self.assertEqual(len(observations), 2)
            result = AUDIT.audit_observations(observations)
            self.assertEqual(result["portable_rule_count"], 1)


if __name__ == "__main__":
    unittest.main()
