from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "sonic3_harmonic_probe.py"


def load_probe():
    spec = importlib.util.spec_from_file_location("sonic3_harmonic_probe", TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {TOOL}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


probe = load_probe()


class Sonic3HarmonicProbeTest(unittest.TestCase):
    def test_synthetic_vgm_recovers_surface_harmony_without_illegal_promotion(self) -> None:
        result = probe.audit_bytes(
            probe._synthetic_vgm(),
            source_name="synthetic-cfgc.vgm",
        )

        self.assertGreater(result["performed_pitch_projection_coverage"], 0.99)
        triads = result["surface_triad_duration_ticks"]
        self.assertIn("0:major", triads)
        self.assertIn("5:major", triads)
        self.assertIn("7:major", triads)

        top = result["top_surface_tonal_candidates"][0]
        self.assertEqual(top["center_pitch_class"], 0)
        self.assertEqual(top["mode"], "ionian")
        self.assertFalse(top["key_class_resolved"])

        shapes = result["surface_function_shapes"]["counts"]
        self.assertGreaterEqual(shapes.get("predominant_to_dominant_shape", 0), 1)
        self.assertGreaterEqual(shapes.get("dominant_to_tonic_shape", 0), 1)
        self.assertFalse(result["surface_function_shapes"]["functional_tendency_resolved"])
        self.assertFalse(result["surface_function_shapes"]["cadence_resolved"])

        promotion = result["shared_model_promotion"]
        self.assertEqual(promotion["tonal_center"], "ranked_surface_candidate_only")
        self.assertEqual(promotion["key_class"], "blocked")
        self.assertEqual(promotion["functional_tendency"], "blocked")
        self.assertEqual(promotion["cadence_class"], "blocked")
        self.assertEqual(promotion["tonicization_or_modulation"], "blocked")

    def test_operator_detune_reduces_performed_pitch_coverage(self) -> None:
        clean = probe.audit_bytes(
            probe._synthetic_vgm(),
            source_name="synthetic-clean.vgm",
        )
        detuned = probe.audit_bytes(
            probe._synthetic_vgm(detune=True),
            source_name="synthetic-detuned.vgm",
        )

        self.assertLess(
            detuned["performed_pitch_projection_coverage"],
            clean["performed_pitch_projection_coverage"],
        )
        self.assertGreater(
            detuned["unresolved_voice_ticks_by_reason"].get("operator_detune_present", 0),
            0,
        )

    def test_transposition_invariant_signature_coordinates_ignore_absolute_root(self) -> None:
        first = {
            "surface_triad_quality_duration_ticks": {"major": 100, "minor": 50},
            "directed_root_motion_histogram": {"5": 3, "7": 1},
            "quality_transition_histogram": {"major>major": 2, "major>minor": 1},
            "top_surface_tonal_candidates": [
                {"mode": "ionian", "ranking_score": 0.8, "center_pitch_class": 0},
            ],
            "surface_function_shapes": {"counts": {"dominant_to_tonic_shape": 2}},
        }
        second = {
            "surface_triad_quality_duration_ticks": {"major": 200, "minor": 100},
            "directed_root_motion_histogram": {"5": 6, "7": 2},
            "quality_transition_histogram": {"major>major": 4, "major>minor": 2},
            "top_surface_tonal_candidates": [
                {"mode": "ionian", "ranking_score": 0.8, "center_pitch_class": 7},
            ],
            "surface_function_shapes": {"counts": {"dominant_to_tonic_shape": 4}},
        }

        self.assertEqual(probe.harmonic_signature(first), probe.harmonic_signature(second))

    def test_real_corpus_surface_harmony_cannot_bootstrap_phrase_syntax(self) -> None:
        fixture = (
            ROOT
            / "tests"
            / "corpus"
            / "sonic-3-knuckles"
            / "01 - Angel Island Zone Act 1.vgz"
        )
        self.assertTrue(fixture.is_file())

        result = probe.audit_file(fixture)

        self.assertGreater(result["duration_ticks"], 0)
        self.assertGreater(result["active_full_key_voice_ticks"], 0)
        self.assertGreaterEqual(result["performed_pitch_projection_coverage"], 0.0)
        self.assertLessEqual(result["performed_pitch_projection_coverage"], 1.0)
        self.assertEqual(result["surface_scope"], "physical-channel FM execution")

        # This is deliberately a negative promotion control on a complete
        # historical cue. Surface harmonic candidates may exist, but physical
        # channel evidence has not earned persistent musical parts, cross-part
        # phrase boundaries, cadence class, or a phrase role.
        promotion = result["shared_model_promotion"]
        self.assertEqual(promotion["tonal_center"], "ranked_surface_candidate_only")
        self.assertEqual(promotion["key_class"], "blocked")
        self.assertEqual(promotion["functional_tendency"], "blocked")
        self.assertEqual(promotion["cadence_class"], "blocked")
        self.assertIn(
            "no_persistent_part_voice_leading",
            promotion["blocked_by"],
        )
        self.assertIn(
            "no_cross_part_phrase_boundary_or_cadential_arrival",
            promotion["blocked_by"],
        )
        self.assertFalse(
            result["surface_function_shapes"]["functional_tendency_resolved"]
        )
        self.assertFalse(result["surface_function_shapes"]["cadence_resolved"])

        for candidate in result["top_surface_tonal_candidates"]:
            self.assertFalse(candidate["key_class_resolved"])

    def test_embedded_self_test(self) -> None:
        result = probe._synthetic_self_test()
        self.assertEqual(result["clean_top_candidate"]["mode"], "ionian")
        self.assertEqual(result["promotion_status"]["key_class"], "blocked")


if __name__ == "__main__":
    unittest.main()
