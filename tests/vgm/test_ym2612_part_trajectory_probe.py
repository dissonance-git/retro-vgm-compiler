from __future__ import annotations

import importlib.util
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "ym2612_part_trajectory_probe.py"


def load_probe():
    spec = importlib.util.spec_from_file_location("ym2612_part_trajectory_probe", TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {TOOL}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


probe = load_probe()


def event(tick: int, channel: int, pitch: float, patch: str):
    return SimpleNamespace(
        tick=tick,
        channel=channel,
        frequency_measure=pitch,
        patch_core=patch,
    )


class Ym2612PartTrajectoryProbeTest(unittest.TestCase):
    def test_repeated_motif_across_gap_creates_bounded_trajectory_candidate(self) -> None:
        onsets = [
            event(0, 0, 100.0, "patch-a"),
            event(100, 0, 112.5, "patch-a"),
            event(200, 0, 125.0, "patch-a"),
            event(900, 0, 200.0, "patch-a"),
            event(1100, 0, 225.0, "patch-a"),
            event(1300, 0, 250.0, "patch-a"),
        ]
        result = probe.analyze_onsets(onsets, source_name="synthetic.vgm")

        self.assertEqual(result["trajectory_candidate_count"], 1)
        candidate = result["trajectory_candidates"][0]
        self.assertEqual(candidate["channel"], 0)
        self.assertEqual(candidate["boundary_tick"], 900)
        self.assertTrue(candidate["evidence"]["instrument_program_identity"])
        self.assertTrue(candidate["evidence"]["repeated_pitch_rhythm_motif"])
        self.assertFalse(candidate["persistent_part_promoted"])
        self.assertEqual(result["shared_model_promotion"]["persistent_part_identity"], "blocked")

    def test_same_channel_without_program_identity_cannot_bridge_gap(self) -> None:
        onsets = [
            event(0, 0, 100.0, "patch-a"),
            event(100, 0, 112.5, "patch-a"),
            event(200, 0, 125.0, "patch-a"),
            event(900, 0, 200.0, "patch-b"),
            event(1100, 0, 225.0, "patch-b"),
            event(1300, 0, 250.0, "patch-b"),
        ]
        result = probe.analyze_onsets(onsets, source_name="synthetic.vgm")
        self.assertEqual(result["trajectory_candidate_count"], 0)

    def test_same_patch_without_repeated_motif_cannot_bridge_gap(self) -> None:
        onsets = [
            event(0, 0, 100.0, "patch-a"),
            event(100, 0, 112.5, "patch-a"),
            event(200, 0, 125.0, "patch-a"),
            event(900, 0, 200.0, "patch-a"),
            event(1100, 0, 180.0, "patch-a"),
            event(1300, 0, 260.0, "patch-a"),
        ]
        result = probe.analyze_onsets(onsets, source_name="synthetic.vgm")
        self.assertEqual(result["trajectory_candidate_count"], 0)

    def test_distinct_channels_can_form_aligned_boundary_target_without_promotion(self) -> None:
        onsets = []
        for channel, scale, patch in ((0, 1.0, "patch-a"), (1, 1.5, "patch-b")):
            onsets.extend([
                event(0, channel, 100.0 * scale, patch),
                event(100, channel, 112.5 * scale, patch),
                event(200, channel, 125.0 * scale, patch),
                event(900, channel, 200.0 * scale, patch),
                event(1100, channel, 225.0 * scale, patch),
                event(1300, channel, 250.0 * scale, patch),
            ])
        result = probe.analyze_onsets(onsets, source_name="synthetic.vgm")

        self.assertEqual(result["trajectory_candidate_count"], 2)
        self.assertEqual(result["cross_trajectory_boundary_candidate_count"], 1)
        boundary = result["cross_trajectory_boundary_candidates"][0]
        self.assertEqual(boundary["representative_tick"], 900)
        self.assertEqual(boundary["supporting_channels"], [0, 1])
        self.assertFalse(boundary["cross_part_phrase_boundary_promoted"])
        self.assertEqual(result["shared_model_promotion"]["phrase_role"], "blocked")

    def test_real_corpus_probe_is_deterministic_and_preserves_promotion_firewall(self) -> None:
        fixture = (
            ROOT
            / "tests"
            / "corpus"
            / "sonic-3-knuckles"
            / "01 - Angel Island Zone Act 1.vgz"
        )
        self.assertTrue(fixture.is_file())

        first = probe.audit_file(fixture)
        second = probe.audit_file(fixture)
        self.assertEqual(first, second)
        self.assertEqual(first["chip_scope"], "YM2612")
        self.assertFalse(first["platform_identity_consulted"])
        self.assertGreater(first["observation_count"], 0)
        self.assertEqual(first["shared_model_promotion"]["persistent_part_identity"], "blocked")
        self.assertEqual(first["shared_model_promotion"]["cross_part_phrase_boundary"], "blocked")
        self.assertEqual(first["shared_model_promotion"]["phrase_role"], "blocked")

        print(
            "ANGEL_ISLAND_YM2612_TRAJECTORY_PRESSURE",
            {
                "observations": first["observation_count"],
                "trajectory_candidates": first["trajectory_candidate_count"],
                "cross_trajectory_boundaries": first[
                    "cross_trajectory_boundary_candidate_count"
                ],
            },
        )


if __name__ == "__main__":
    unittest.main()
