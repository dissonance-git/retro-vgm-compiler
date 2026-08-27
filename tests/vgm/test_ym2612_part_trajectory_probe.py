from __future__ import annotations

import gzip
import importlib.util
from pathlib import Path
from types import SimpleNamespace
import sys
import tempfile
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
    def test_vgm_payload_compression_is_detected_from_bytes_not_suffix(self) -> None:
        payload = b"Vgm " + bytes(64)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            compressed_named_vgm = root / "compressed.vgm"
            compressed_named_vgm.write_bytes(gzip.compress(payload))
            self.assertEqual(probe.motif_probe._read_vgm_payload(compressed_named_vgm), payload)

            raw_named_vgz = root / "raw.vgz"
            raw_named_vgz.write_bytes(payload)
            self.assertEqual(probe.motif_probe._read_vgm_payload(raw_named_vgz), payload)

    def test_same_tick_projection_collapses_only_identical_observations(self) -> None:
        onsets = [
            event(100, 0, 100.0, "patch-a"),
            event(100, 0, 100.0, "patch-a"),
            event(200, 0, 112.5, "patch-a"),
            event(200, 0, 125.0, "patch-a"),
            event(300, 1, 150.0, "patch-b"),
        ]
        normalized, stats = probe.normalize_same_tick_observations(onsets)

        self.assertEqual(len(normalized), 2)
        self.assertEqual(stats["raw_source_onsets"], 5)
        self.assertEqual(stats["analysis_observations"], 2)
        self.assertEqual(stats["same_tick_duplicate_collapses"], 1)
        self.assertEqual(stats["same_tick_ambiguous_exclusions"], 2)
        self.assertEqual(
            [(item.channel, item.tick) for item in normalized],
            [(0, 100), (1, 300)],
        )

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

    def test_shared_support_can_compose_without_becoming_identity_conflict(self) -> None:
        onsets = [
            event(0, 0, 100.0, "patch-a"),
            event(100, 0, 112.5, "patch-a"),
            event(200, 0, 125.0, "patch-a"),
            event(900, 0, 200.0, "patch-a"),
            event(1000, 0, 225.0, "patch-a"),
            event(1100, 0, 250.0, "patch-a"),
            event(1800, 0, 300.0, "patch-a"),
            event(1900, 0, 337.5, "patch-a"),
            event(2000, 0, 375.0, "patch-a"),
        ]
        result = probe.analyze_onsets(onsets, source_name="synthetic-chain.vgm")

        self.assertEqual(result["trajectory_candidate_count"], 2)
        self.assertEqual(result["trajectory_support_relation_count"], 1)
        self.assertEqual(result["composable_link_chain_candidate_count"], 1)
        self.assertEqual(result["unresolved_support_reuse_count"], 0)
        relation = result["trajectory_support_relations"][0]
        self.assertEqual(relation["relation_kind"], "composable_link_chain_candidate")
        self.assertEqual(relation["shared_support_ticks"], [900, 1000, 1100])
        self.assertFalse(relation["persistent_part_identity_established"])
        self.assertFalse(relation["identity_conflict_established"])
        self.assertEqual(
            result["shared_model_promotion"]["persistent_part_identity"],
            "blocked",
        )

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
        self.assertGreater(first["source_onset_observation_count"], 0)
        self.assertGreater(first["observation_count"], 0)
        self.assertGreaterEqual(first["same_tick_duplicate_collapses"], 0)
        self.assertGreaterEqual(first["same_tick_ambiguous_exclusions"], 0)
        self.assertEqual(first["shared_model_promotion"]["persistent_part_identity"], "blocked")
        self.assertEqual(first["shared_model_promotion"]["cross_part_phrase_boundary"], "blocked")
        self.assertEqual(first["shared_model_promotion"]["phrase_role"], "blocked")

        print(
            "ANGEL_ISLAND_YM2612_TRAJECTORY_PRESSURE",
            {
                "source_onsets": first["source_onset_observation_count"],
                "analysis_observations": first["observation_count"],
                "same_tick_duplicate_collapses": first["same_tick_duplicate_collapses"],
                "same_tick_ambiguous_exclusions": first["same_tick_ambiguous_exclusions"],
                "trajectory_candidates": first["trajectory_candidate_count"],
                "cross_trajectory_boundaries": first[
                    "cross_trajectory_boundary_candidate_count"
                ],
            },
        )

    def test_real_corpus_pressure_spans_multiple_ym2612_works(self) -> None:
        fixtures = [
            ROOT / "tests" / "corpus" / "sonic-3-knuckles" / "01 - Angel Island Zone Act 1.vgz",
            ROOT / "tests" / "corpus" / "aa-harimanada-vgz" / "02 - Fierce God of Flame.vgz",
            ROOT / "tests" / "corpus" / "battle-golfer-yui-vgz" / "03 - Smile On.vgz",
            ROOT / "tests" / "corpus" / "toki-vgm" / "02 - Caves.vgm",
        ]
        reports = []
        for fixture in fixtures:
            self.assertTrue(fixture.is_file(), fixture)
            report = probe.audit_file(fixture)
            self.assertEqual(report["chip_scope"], "YM2612")
            self.assertFalse(report["platform_identity_consulted"])
            self.assertGreater(report["source_onset_observation_count"], 0)
            self.assertGreater(report["observation_count"], 0)
            self.assertEqual(
                report["shared_model_promotion"]["persistent_part_identity"],
                "blocked",
            )
            self.assertEqual(
                report["shared_model_promotion"]["cross_part_phrase_boundary"],
                "blocked",
            )
            self.assertEqual(report["shared_model_promotion"]["phrase_role"], "blocked")
            reports.append({
                "fixture": fixture.name,
                "source_onsets": report["source_onset_observation_count"],
                "analysis_observations": report["observation_count"],
                "trajectory_candidates": report["trajectory_candidate_count"],
                "support_relations": report["trajectory_support_relation_count"],
                "composable_chains": report["composable_link_chain_candidate_count"],
                "unresolved_support_reuse": report["unresolved_support_reuse_count"],
                "cross_trajectory_boundaries": report[
                    "cross_trajectory_boundary_candidate_count"
                ],
            })

        self.assertEqual(len(reports), 4)
        print("YM2612_CROSS_WORK_TRAJECTORY_PRESSURE", reports)


if __name__ == "__main__":
    unittest.main()
