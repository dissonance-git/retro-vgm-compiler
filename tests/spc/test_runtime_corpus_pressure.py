from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "runtime_corpus_pressure.py"


def load_tool():
    spec = importlib.util.spec_from_file_location("spc_runtime_corpus_pressure", TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {TOOL}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


pressure = load_tool()


def sidecar(
    *,
    stored: int = 12,
    dropped: int = 0,
    overflowed: int = 0,
    continuity_breaks: int = 0,
    voice_episodes: int = 6,
    eligible: int = 6,
    candidates: int = 5,
    strong: int = 3,
    rejected: int = 2,
    emitted: int = 1,
    profiles: int = 1,
):
    return {
        "model": "label-blind SPC forensic feature sidecar",
        "claim_boundary": "runtime only",
        "provenance": {"device_tick_rate": 1024000},
        "controlled_execution": {
            "source_bytes": 66048,
            "requested_seconds": 3,
            "requested_device_clocks": 3072000,
        },
        "capture": {
            "ram_write_count": 10,
            "ram_writes_spc700_cpu": 10,
            "ram_writes_dsp_echo": 0,
            "ram_writes_ipl_rom_overlay": 0,
            "window_count": 2,
            "stored_event_count": stored,
            "dropped_event_count": dropped,
            "overflowed_window_count": overflowed,
            "cross_lane_backstep_count": 2,
            "max_cross_lane_backstep_ticks": 7,
        },
        "replay": {
            "windows_replayed": 2,
            "ram_writes_applied": 10,
            "records_materialized": stored,
            "continuity_breaks": continuity_breaks,
            "samples_materialized": 4,
            "samples_reused": 2,
            "samples_deferred": 0,
            "final_ram_write_serial": 10,
        },
        "features": {
            "voice_episode_count": voice_episodes,
            "eligible_episode_count": eligible,
            "candidate_transition_count": candidates,
            "strong_transition_count": strong,
            "rejected_transition_count": rejected,
            "continuity_barrier_count": 0,
            "emitted_part_count": emitted,
            "part_profile_count": profiles,
            "part_profiles": [
                {
                    "profile_index": index,
                    "gesture_count": 3,
                    "normalized_inter_onset_intervals": [1.0, 1.0],
                    "interval_octaves": [0.1, -0.1],
                    "pitch_contour": [1, -1],
                    "interval_semantics": "relative_pitch_ratio",
                    "pitch_range_octaves": 0.2,
                    "evidence_status": "hypothesis",
                    "evidence_confidence": 0.75,
                }
                for index in range(profiles)
            ],
        },
    }


class SpcRuntimeCorpusPressureTest(unittest.TestCase):
    def test_valid_sidecar_preserves_balanced_runtime_and_part_evidence(self) -> None:
        metrics = pressure.validate_sidecar(sidecar())
        self.assertEqual(metrics["cross_lane_backstep_count"], 2)
        self.assertEqual(metrics["max_cross_lane_backstep_ticks"], 7)
        self.assertEqual(metrics["candidate_transition_count"], 5)
        self.assertEqual(metrics["strong_transition_count"], 3)
        self.assertEqual(metrics["rejected_transition_count"], 2)
        self.assertEqual(metrics["part_profile_count"], 1)

    def test_backstep_diagnostics_must_remain_self_consistent(self) -> None:
        payload = sidecar()
        payload["capture"]["cross_lane_backstep_count"] = 0
        with self.assertRaisesRegex(ValueError, "magnitude"):
            pressure.validate_sidecar(payload)

    def test_capture_loss_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "lossless capture"):
            pressure.validate_sidecar(sidecar(dropped=1))

    def test_transition_accounting_must_partition_candidates(self) -> None:
        with self.assertRaisesRegex(ValueError, "partition"):
            pressure.validate_sidecar(sidecar(candidates=6, strong=3, rejected=2))

    def test_short_motif_profile_is_rejected(self) -> None:
        payload = sidecar()
        payload["features"]["part_profiles"][0]["gesture_count"] = 2
        with self.assertRaisesRegex(ValueError, "at least three gestures"):
            pressure.validate_sidecar(payload)

    def test_identity_bearing_feature_keys_are_rejected(self) -> None:
        payload = sidecar()
        payload["features"]["composer"] = "forbidden"
        with self.assertRaisesRegex(ValueError, "identity-bearing"):
            pressure.validate_sidecar(payload)


if __name__ == "__main__":
    unittest.main()
