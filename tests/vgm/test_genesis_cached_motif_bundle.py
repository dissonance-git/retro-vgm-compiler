from __future__ import annotations

import importlib.util
import json
import math
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
GENESIS_TOOL = ROOT / "tools" / "genesis_cached_part_evidence.py"
FREEZE_TOOL = ROOT / "tools" / "spc" / "freeze_forensic_sidecars.py"


def load_module(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


genesis = load_module("genesis_cached_part_evidence_bundle_test", GENESIS_TOOL)
freeze = load_module("freeze_forensic_sidecars_bundle_test", FREEZE_TOOL)


def capsule() -> dict:
    return {
        "schema_version": 3,
        "extractor": {"name": "creator-blind-genesis-song-capsule", "version": 3},
        "label_policy": "Creator labels excluded.",
        "source": {
            "path": "tests/corpus/poison-world/Secret Composer - Secret Track.vgz",
            "file": "Secret Composer - Secret Track.vgz",
            "corpus_id": "poison-world",
        },
        "timing": {"duration_vgm_samples": 3200},
        "ym2612": {
            "events": {
                "tick": [0, 1100, 2100],
                "channel": [0, 0, 0],
                "fnum": [0x300, 0x360, 0x330],
                "block": [3, 3, 3],
                "patch_full_id": [17, 17, 17],
                "key_gate_event_index": [0, 2, 4],
            },
            "key_gate_events": {
                "tick": [0, 1000, 1100, 2000, 2100, 3000],
                "channel": [0, 0, 0, 0, 0, 0],
                "operator_mask": [0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00],
            },
        },
    }


class GenesisCachedMotifBundleTest(unittest.TestCase):
    def test_projected_bundle_preserves_geometry_confidence_and_label_firewall(self):
        projection = genesis.project(
            capsule(),
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
            strand_min_confidence=0.75,
        )
        bundle = genesis.make_motif_profile_bundle(projection)

        self.assertEqual(bundle["model"], genesis.PROFILE_BUNDLE_MODEL)
        self.assertEqual(bundle["representation"], genesis.GENESIS_MOTIF_REPRESENTATION)
        self.assertEqual(bundle["diagnostics"]["part_profile_count"], 1)
        motif = bundle["part_profiles"][0]
        self.assertEqual(motif["gesture_count"], 3)
        self.assertEqual(motif["evidence_status"], "hypothesis")
        self.assertAlmostEqual(motif["evidence_confidence"], 0.94)
        self.assertEqual(motif["pitch_basis"], "genesis_ym2612_relative_frequency_code")
        self.assertEqual(motif["interval_semantics"], "log2_frequency_ratio_octaves")

        self.assertAlmostEqual(motif["normalized_inter_onset_intervals"][0], 1100 / 1050)
        self.assertAlmostEqual(motif["normalized_inter_onset_intervals"][1], 1000 / 1050)
        expected_intervals = [
            math.log2(0x360 / 0x300),
            math.log2(0x330 / 0x360),
        ]
        self.assertAlmostEqual(motif["interval_octaves"][0], expected_intervals[0])
        self.assertAlmostEqual(motif["interval_octaves"][1], expected_intervals[1])
        self.assertEqual(motif["pitch_contour"], [1, -1])

        text = json.dumps(bundle).lower()
        self.assertNotIn("secret composer", text)
        self.assertNotIn("secret track", text)
        self.assertNotIn("poison-world", text)
        self.assertNotIn("tests/corpus", text)
        for forbidden_key in freeze.FORBIDDEN_KEYS:
            self.assertNotIn(f'"{forbidden_key}"', text)

    def test_generic_freezer_accepts_genesis_bundle_without_reinterpretation(self):
        projection = genesis.project(
            capsule(),
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
            strand_min_confidence=0.75,
        )
        bundle = genesis.make_motif_profile_bundle(projection)
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / "genesis-bundle.json"
            path.write_text(json.dumps(bundle), encoding="utf-8")
            loaded = freeze.load_profile_bundle("cue-001", path)
        self.assertEqual(loaded.representation, genesis.GENESIS_MOTIF_REPRESENTATION)
        self.assertEqual(loaded.profiles, bundle["part_profiles"])

    def test_two_episode_strand_remains_below_motif_profile_boundary(self):
        episodes = [
            genesis.Episode(0, 0, 1000, 0, 0x300, 3, 17, 0, 1),
            genesis.Episode(1, 1100, 2000, 0, 0x360, 3, 17, 2, 3),
        ]
        link = genesis.infer_continuity(
            episodes[0],
            episodes[1],
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
        )
        strand_projection = genesis.assemble_strand_hypotheses(episodes, [link])
        projection = {
            "episodes": [episode.__dict__ for episode in episodes],
            "strand_projection": strand_projection,
        }
        with self.assertRaisesRegex(ValueError, "at least three episodes"):
            genesis.make_motif_profile_bundle(projection)

    def test_motif_bundle_requires_the_conservative_strand_gate(self):
        projection = genesis.project(
            capsule(),
            max_gap_ticks=500,
            max_pitch_interval_octaves=1.5,
        )
        with self.assertRaisesRegex(ValueError, "existing strand projection"):
            genesis.make_motif_profile_bundle(projection)


if __name__ == "__main__":
    unittest.main()
