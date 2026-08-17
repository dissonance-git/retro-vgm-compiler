from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "freeze_forensic_sidecars.py"
SPEC = importlib.util.spec_from_file_location("freeze_forensic_sidecars", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
freeze = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = freeze
SPEC.loader.exec_module(freeze)


def profile(rhythm, intervals=None, contour=None, semantics="performed_pitch_ratio", confidence=1.0):
    return {
        "profile_index": 0,
        "gesture_count": len(rhythm) + 1,
        "normalized_inter_onset_intervals": rhythm,
        "interval_octaves": intervals,
        "pitch_contour": contour,
        "interval_semantics": semantics if intervals is not None else "",
        "pitch_range_octaves": 0.5 if intervals is not None else None,
        "evidence_status": "derived",
        "evidence_confidence": confidence,
    }


def sidecar(profiles, *, commit="abc", dropped=0, overflowed=0, breaks=0):
    return {
        "model": freeze.EXPECTED_MODEL,
        "claim_boundary": "blind",
        "provenance": {
            "retro_vgm_compiler_commit": commit,
            "snes_spc_repository": "blarggs-audio-libraries/snes_spc",
            "snes_spc_commit": "ec8ee2b",
            "instrumentation_patch_contract": "phase-accurate-v1",
            "device_tick_rate": 1024000,
        },
        "capture": {
            "ram_write_count": 7,
            "ram_writes_spc700_cpu": 4,
            "ram_writes_dsp_echo": 3,
            "ram_writes_ipl_rom_overlay": 0,
            "window_count": 2,
            "stored_event_count": 12,
            "dropped_event_count": dropped,
            "overflowed_window_count": overflowed,
        },
        "replay": {
            "windows_replayed": 2,
            "ram_writes_applied": 7,
            "records_materialized": 12,
            "continuity_breaks": breaks,
            "samples_materialized": 2,
            "samples_reused": 3,
            "samples_deferred": 0,
            "final_ram_write_serial": 7,
        },
        "features": {
            "voice_episode_count": 8,
            "eligible_episode_count": 8,
            "candidate_transition_count": 5,
            "strong_transition_count": 5,
            "rejected_transition_count": 0,
            "continuity_barrier_count": 0,
            "emitted_part_count": len(profiles),
            "part_profile_count": len(profiles),
            "part_profiles": profiles,
        },
    }


class FreezeForensicSidecarsTest(unittest.TestCase):
    def write(self, directory: pathlib.Path, name: str, value) -> pathlib.Path:
        path = directory / name
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_profile_math_matches_cpp_contract_and_rhythm_only_ceiling(self):
        exact_a = profile([0.5, 1.0, 1.5], [0.25, -0.25, 0.0], [1, -1, 0])
        exact_b = profile([0.5, 1.0, 1.5], [0.25, -0.25, 0.0], [1, -1, 0])
        result = freeze.compare_profiles(exact_a, exact_b)
        self.assertTrue(result["pitch_comparable"])
        self.assertAlmostEqual(result["identity_confidence"], 1.0)

        rhythm_a = profile([0.5, 1.0, 1.5])
        rhythm_b = profile([0.5, 1.0, 1.5])
        result = freeze.compare_profiles(rhythm_a, rhythm_b)
        self.assertFalse(result["pitch_comparable"])
        self.assertAlmostEqual(result["combined_similarity"], 1.0)
        self.assertAlmostEqual(result["identity_confidence"], 0.55)

    def test_unmatched_parts_are_zero_weight_not_discarded(self):
        common = profile([1.0, 1.0], [0.0, 0.0], [0, 0])
        unrelated = profile([0.2, 1.8], [0.5, 0.5], [1, 1])
        result = freeze.compare_profile_sets([common], [common, unrelated])
        self.assertEqual(result["matched_pair_count"], 1)
        self.assertAlmostEqual(result["matched_coverage"], 0.5)
        self.assertAlmostEqual(result["similarity"], 0.5)

    def test_load_rejects_label_leakage_and_incomplete_capture(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            clean = sidecar([profile([1.0, 1.0])])
            clean["composer"] = "poison"
            with self.assertRaises(ValueError):
                freeze.load_sidecar("cue-001", self.write(root, "poison.json", clean))

            broken = sidecar([profile([1.0, 1.0])], dropped=1)
            with self.assertRaises(ValueError):
                freeze.load_sidecar("cue-001", self.write(root, "broken.json", broken))

    def test_zero_profile_capture_is_cache_valid_but_not_freeze_admissible(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            path = self.write(root, "zero.json", sidecar([]))
            cached = freeze.load_sidecar("cue-001", path, require_profiles=False)
            self.assertEqual(cached.profiles, [])
            self.assertEqual(cached.diagnostics["part_profile_count"], 0)
            with self.assertRaises(ValueError):
                freeze.load_sidecar("cue-001", path)

    def test_freeze_requires_identical_runtime_provenance(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            first = freeze.load_sidecar(
                "cue-001", self.write(root, "a.json", sidecar([profile([1.0, 1.0])], commit="a")))
            second = freeze.load_sidecar(
                "cue-002", self.write(root, "b.json", sidecar([profile([1.0, 1.0])], commit="b")))
            with self.assertRaises(ValueError):
                freeze.freeze_corpus([first, second])

    def test_frozen_output_contains_only_opaque_ids_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            a = freeze.load_sidecar(
                "cue-001", self.write(root, "named-game-a.json", sidecar([profile([1.0, 1.0])])))
            b = freeze.load_sidecar(
                "cue-002", self.write(root, "named-game-b.json", sidecar([profile([1.0, 1.0])])))
            result = freeze.freeze_corpus([b, a])
            self.assertEqual([cue["cue_id"] for cue in result["cues"]], ["cue-001", "cue-002"])
            text = json.dumps(result)
            self.assertNotIn("named-game-a", text)
            self.assertNotIn("named-game-b", text)
            self.assertAlmostEqual(result["similarity_matrix"]["cue-001"]["cue-002"], 0.55)


if __name__ == "__main__":
    unittest.main()
