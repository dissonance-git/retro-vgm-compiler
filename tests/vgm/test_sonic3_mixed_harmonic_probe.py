from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "sonic3_mixed_harmonic_probe.py"


def load_probe():
    spec = importlib.util.spec_from_file_location("sonic3_mixed_harmonic_probe", TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {TOOL}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


probe = load_probe()


class Sonic3MixedHarmonicProbeTest(unittest.TestCase):
    def test_mixed_stream_preserves_psg_role_ambiguity(self) -> None:
        result = probe.audit_bytes(
            probe._synthetic_mixed_vgm(),
            source_name="synthetic-mixed.vgm",
        )

        self.assertGreater(result["active_fm_full_key_voice_ticks"], 0)
        self.assertGreater(result["active_psg_tone_voice_ticks"], 0)
        self.assertGreater(result["surface_device_mix_ticks"]["mixed_fm_psg"], 0)

        role = result["psg_role_evidence"]
        self.assertGreater(role["same_pitch_fm_doubling_candidate_voice_ticks"], 0)
        self.assertIn("does not establish bass_foundation", role["mixed_context_bass_policy"])

        noise = result["psg_noise_surface"]
        self.assertGreater(noise["active_ticks"], 0)
        self.assertFalse(noise["pitch_class_available"])
        self.assertFalse(noise["hi_hat_established"])
        self.assertEqual(noise["candidate_role_family"], "percussion_or_texture")

        promotion = result["shared_model_promotion"]
        self.assertEqual(promotion["psg_part_role"], "candidate_only")
        self.assertEqual(promotion["psg_noise_percussion_identity"], "candidate_only")
        self.assertEqual(promotion["key_class"], "blocked")

    def test_psg_doubling_does_not_duplicate_pitch_class_presence_weight(self) -> None:
        result = probe.audit_bytes(
            probe._synthetic_mixed_vgm(with_noise=False),
            source_name="synthetic-mixed-no-noise.vgm",
        )
        # C/F/G/C still produces the same three basic major-triad classes. A
        # shadowed PSG pitch must not become an extra copy of its pitch class in
        # the surface collection merely because a second chip emits it.
        triads = result["surface_triad_duration_ticks"]
        self.assertIn("0:major", triads)
        self.assertIn("5:major", triads)
        self.assertIn("7:major", triads)
        self.assertGreater(
            result["psg_role_evidence"]["same_pitch_fm_doubling_candidate_voice_ticks"],
            0,
        )

    def test_low_psg_tone_cannot_define_mixed_context_inversion_by_register_alone(self) -> None:
        projected = [
            {
                "pitch_class": 0,
                "performed_hz": 261.63,
                "device_family": "YM2612",
                "surface_bass_eligible": True,
            },
            {
                "pitch_class": 4,
                "performed_hz": 329.63,
                "device_family": "YM2612",
                "surface_bass_eligible": True,
            },
            {
                "pitch_class": 7,
                "performed_hz": 392.00,
                "device_family": "YM2612",
                "surface_bass_eligible": True,
            },
            {
                # Same pitch class as G, but far below the FM C. Hardware
                # register position alone must not turn this PSG source into
                # the harmonic bass in a mixed FM+PSG arrangement.
                "pitch_class": 7,
                "performed_hz": 98.00,
                "device_family": "SN76489",
                "surface_bass_eligible": False,
            },
        ]
        triad = probe._mixed_triad_state(projected)
        self.assertIsNotNone(triad)
        assert triad is not None
        self.assertEqual(triad["label"], "0:major")
        self.assertEqual(triad["inversion"], "root_position")
        self.assertEqual(triad["bass_source"], "YM2612")

    def test_noncoincident_psg_pitch_remains_available_as_independent_part_candidate(self) -> None:
        fm_pitch = {
            "resolved": True,
            "performed_hz": 261.63,
            "device_family": "YM2612",
        }
        psg_pitch = {
            "resolved": True,
            "performed_hz": 659.26,
            "device_family": "SN76489",
        }
        self.assertFalse(probe.psg.pitch_coincident(fm_pitch, psg_pitch))
        # Non-coincidence does not establish independence either. It merely keeps
        # the PSG source available for persistent-part/counterpoint analysis.
        self.assertNotEqual(fm_pitch["device_family"], psg_pitch["device_family"])

    def test_self_test_keeps_hihat_unresolved(self) -> None:
        result = probe._synthetic_self_test()
        self.assertFalse(result["psg_noise_surface"]["hi_hat_established"])
        self.assertEqual(result["promotion_status"]["key_class"], "blocked")


if __name__ == "__main__":
    unittest.main()
