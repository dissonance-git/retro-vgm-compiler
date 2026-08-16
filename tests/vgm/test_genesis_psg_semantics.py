from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "genesis_psg_semantics.py"


def load_module():
    spec = importlib.util.spec_from_file_location("genesis_psg_semantics", TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {TOOL}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


psg = load_module()


class GenesisPsgSemanticsTest(unittest.TestCase):
    def test_latch_data_tone_period_and_pitch_projection(self) -> None:
        state = psg.SN76489SurfaceState()
        clock = 3_579_545

        # Tone 0 period 0x100, then unmute it.
        state.write(0x80 | 0x00)
        state.write(0x10)
        state.write(0x90 | 0x02)

        self.assertEqual(state.tone_periods[0], 0x100)
        self.assertTrue(state.tone_active(0))
        pitch = state.tone_pitch(
            0,
            clock,
            tolerance_cents=50.0,
            ym2612_music_present=True,
        )
        self.assertTrue(pitch["resolved"])
        self.assertEqual(pitch["device_family"], "SN76489")
        self.assertFalse(pitch["surface_bass_eligible"])
        self.assertEqual(
            pitch["bass_role_prior"],
            "requires_strong_independent_evidence_in_ym2612_plus_psg_context",
        )

    def test_psg_tone_is_not_prevented_from_being_independent_music(self) -> None:
        state = psg.SN76489SurfaceState()
        clock = 3_579_545
        # A4-ish tone. The helper exposes pitch while refusing to assign musical
        # role merely from the chip family.
        period = round(clock / (32.0 * 440.0))
        state.write(0x80 | (period & 0x0F))
        state.write((period >> 4) & 0x3F)
        state.write(0x90)
        pitch = state.tone_pitch(0, clock, ym2612_music_present=True)
        self.assertTrue(pitch["resolved"])
        self.assertEqual(pitch["pitch_class"], 9)
        self.assertNotIn("melodic_foreground", pitch)
        self.assertNotIn("doubling_support", pitch)

    def test_noise_is_percussion_or_texture_not_harmonic_pitch_or_fixed_hihat(self) -> None:
        state = psg.SN76489SurfaceState()
        state.write(0xE7)  # noise control
        state.write(0xF2)  # unmute noise
        surface = state.noise_surface(ym2612_music_present=True)
        self.assertTrue(surface["active"])
        self.assertEqual(surface["candidate_role_family"], "percussion_or_texture")
        self.assertFalse(surface["pitch_class_available"])
        self.assertFalse(surface["hi_hat_established"])
        self.assertFalse(surface["bass_foundation_candidate"])

    def test_same_pitch_is_only_a_doubling_candidate(self) -> None:
        first = {
            "resolved": True,
            "performed_hz": 440.0,
            "device_family": "YM2612",
        }
        second = {
            "resolved": True,
            "performed_hz": 441.0,
            "device_family": "SN76489",
        }
        self.assertTrue(psg.pitch_coincident(first, second, cents=5.0))
        # The helper does not collapse them into one part or assert a role.
        self.assertNotEqual(first["device_family"], second["device_family"])

    def test_psg_only_context_does_not_apply_mixed_fm_bass_prior(self) -> None:
        state = psg.SN76489SurfaceState()
        clock = 3_579_545
        period = round(clock / (32.0 * 110.0))
        state.write(0x80 | (period & 0x0F))
        state.write((period >> 4) & 0x3F)
        state.write(0x90)
        pitch = state.tone_pitch(0, clock, ym2612_music_present=False)
        self.assertTrue(pitch["resolved"])
        self.assertTrue(pitch["surface_bass_eligible"])
        self.assertEqual(pitch["bass_role_prior"], "no_mixed_fm_context_prior")


if __name__ == "__main__":
    unittest.main()
