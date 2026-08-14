from __future__ import annotations

import unittest

from components.psf.spu_pitch import (
    PLAYSTATION_SPU_VOICE_COUNT,
    SPU_PITCH_UNITY,
    spu_pitch_step_candidates,
    spu_raw_pitch_step,
    spu_raw_pmon_candidates,
)


class SpuPitchTests(unittest.TestCase):
    def test_playstation_spu_has_24_physical_voices(self) -> None:
        self.assertEqual(PLAYSTATION_SPU_VOICE_COUNT, 24)

    def test_unity_pitch_is_1000_without_pmon(self) -> None:
        self.assertEqual(spu_raw_pitch_step(SPU_PITCH_UNITY), 0x1000)

    def test_zero_modulator_output_preserves_pitch(self) -> None:
        self.assertEqual(spu_raw_pitch_step(0x1000, 0), 0x1000)

    def test_positive_modulator_can_nearly_double_pitch_step(self) -> None:
        self.assertEqual(spu_raw_pitch_step(0x1000, 0x7FFF), 0x1FFF)

    def test_negative_full_scale_modulator_can_reduce_step_to_zero(self) -> None:
        self.assertEqual(spu_raw_pitch_step(0x1000, -0x8000), 0)

    def test_high_bit_signed_candidate_preserves_documented_glitch(self) -> None:
        self.assertEqual(spu_raw_pitch_step(0x8000, 0), 0x8000)

    def test_signed_and_unsigned_pmon_agree_below_high_bit_region(self) -> None:
        for pitch in (0x0000, 0x0001, 0x1000, 0x3FFF, 0x7FFF):
            candidates = spu_raw_pmon_candidates(pitch, 0x7FFF)
            self.assertFalse(candidates.disagree)
            self.assertEqual(candidates.signed_carrier, candidates.unsigned_carrier_fpga)

    def test_high_bit_pmon_arithmetic_disagreement_remains_explicit(self) -> None:
        candidates = spu_raw_pmon_candidates(0x8000, 0x7FFF)
        self.assertTrue(candidates.disagree)
        self.assertEqual(candidates.signed_carrier, 0x0001)
        self.assertEqual(candidates.unsigned_carrier_fpga, 0xFFFF)

    def test_maximum_step_disagreement_remains_explicit(self) -> None:
        candidates = spu_pitch_step_candidates(0x4000)
        self.assertEqual(candidates.raw_signed_carrier, 0x4000)
        self.assertEqual(candidates.raw_unsigned_carrier_fpga, 0x4000)
        self.assertEqual(candidates.signed_implementation_3fff, 0x3FFF)
        self.assertEqual(candidates.signed_documented_4000, 0x4000)
        self.assertEqual(candidates.unsigned_fpga_3fff, 0x3FFF)
        self.assertFalse(candidates.arithmetic_disagreement)
        self.assertTrue(candidates.clamp_disagreement)

    def test_high_bit_candidate_disagreement_survives_before_clamp(self) -> None:
        candidates = spu_pitch_step_candidates(0x8000, 0x7FFF)
        self.assertEqual(candidates.raw_signed_carrier, 0x0001)
        self.assertEqual(candidates.raw_unsigned_carrier_fpga, 0xFFFF)
        self.assertEqual(candidates.signed_implementation_3fff, 0x0001)
        self.assertEqual(candidates.signed_documented_4000, 0x0001)
        self.assertEqual(candidates.unsigned_fpga_3fff, 0x3FFF)
        self.assertTrue(candidates.arithmetic_disagreement)

    def test_invalid_coordinates_fail_closed(self) -> None:
        with self.assertRaises(ValueError):
            spu_raw_pitch_step(0x10000)
        with self.assertRaises(ValueError):
            spu_raw_pitch_step(0x1000, 0x8000)
        with self.assertRaises(ValueError):
            spu_raw_pmon_candidates(0x1000, -0x8001)


if __name__ == "__main__":
    unittest.main()
