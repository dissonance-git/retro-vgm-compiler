from __future__ import annotations

import importlib.util
import pathlib
import random
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "freeze_forensic_sidecars.py"
SPEC = importlib.util.spec_from_file_location("freeze_forensic_sidecars", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
freeze = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = freeze
SPEC.loader.exec_module(freeze)


def profile(*, rhythm, intervals, contour, confidence=1.0, basis="native"):
    return {
        "normalized_inter_onset_intervals": list(rhythm),
        "interval_octaves": list(intervals),
        "pitch_contour": list(contour),
        "pitch_basis": basis,
        "interval_semantics": "log2_frequency_ratio_octaves",
        "evidence_confidence": confidence,
    }


class MotifAssignmentDiagnosticStabilityTest(unittest.TestCase):
    def test_tied_optimal_assignments_have_stable_diagnostics_under_permutation(self):
        query = [
            profile(rhythm=[1.0], intervals=[0.0], contour=[0]),
            profile(rhythm=[1.0], intervals=[0.0], contour=[0]),
        ]
        control = [
            profile(rhythm=[1.0], intervals=[0.0], contour=[0]),
            {
                "normalized_inter_onset_intervals": [1.0],
                "interval_octaves": None,
                "pitch_contour": None,
                "pitch_basis": "",
                "interval_semantics": "",
                "evidence_confidence": 1.0,
            },
        ]

        baseline = freeze.compare_profile_sets(query, control)
        baseline_summary = (
            baseline["similarity"],
            baseline["matched_pair_count"],
            baseline["pitch_comparable_pair_count"],
        )

        rng = random.Random(0xC0BE)
        for _ in range(2000):
            q = list(query)
            c = list(control)
            rng.shuffle(q)
            rng.shuffle(c)
            result = freeze.compare_profile_sets(q, c)
            self.assertEqual(
                (
                    result["similarity"],
                    result["matched_pair_count"],
                    result["pitch_comparable_pair_count"],
                ),
                baseline_summary,
            )


if __name__ == "__main__":
    unittest.main()
