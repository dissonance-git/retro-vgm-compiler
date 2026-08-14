from __future__ import annotations

import unittest

from components.psf.chrono_cross_allocator import (
    CHRONO_LOGICAL_CHANNEL_COUNT,
    PLAYSTATION_SPU_VOICE_COUNT,
    PROTECTED_ENVELOPE_SENTINEL,
    VOICE_INVALID_INDEX,
    allocate_dynamic_voice,
    apply_prevent_rekey_on_resume,
    channel_mask_to_voice_mask,
    effective_envelope_cache,
    find_free_voice,
    initial_sfx_voice_indices,
    steal_quietest_voice,
)


class ChronoCrossAllocatorTests(unittest.TestCase):
    def test_driver_exposes_32_logical_channels_above_24_physical_voices(self) -> None:
        self.assertEqual(CHRONO_LOGICAL_CHANNEL_COUNT, 32)
        self.assertEqual(PLAYSTATION_SPU_VOICE_COUNT, 24)
        self.assertGreater(CHRONO_LOGICAL_CHANNEL_COUNT, PLAYSTATION_SPU_VOICE_COUNT)

    def test_initial_sfx_channels_are_preassigned_to_upper_12_physical_voices(self) -> None:
        self.assertEqual(initial_sfx_voice_indices(), tuple(range(12, 24)))

    def test_fe13_masks_saved_active_notes_without_disabling_other_channels(self) -> None:
        active = 0b111101
        prevent = 0b001100
        self.assertEqual(apply_prevent_rekey_on_resume(active, prevent), 0b110001)

    def test_logical_mode_mask_projects_through_dynamic_physical_assignment(self) -> None:
        assignments = [VOICE_INVALID_INDEX] * CHRONO_LOGICAL_CHANNEL_COUNT
        assignments[5] = 17
        assignments[27] = 3
        mask = (1 << 5) | (1 << 27)
        self.assertEqual(channel_mask_to_voice_mask(assignments, mask), (1 << 17) | (1 << 3))

    def test_active_sfx_voice_is_protected_from_free_scan(self) -> None:
        measured = [1] * PLAYSTATION_SPU_VOICE_COUNT
        measured[12] = 0
        measured[13] = 0
        cache = effective_envelope_cache(measured, active_sfx_voice_mask=(1 << 12))
        self.assertEqual(cache[12], PROTECTED_ENVELOPE_SENTINEL)
        self.assertEqual(find_free_voice(cache, allocator_floor=12), 13)

    def test_idle_preassigned_sfx_voice_can_reenter_pool_when_not_protected(self) -> None:
        measured = [1] * PLAYSTATION_SPU_VOICE_COUNT
        measured[12] = 0
        cache = effective_envelope_cache(measured)
        self.assertEqual(find_free_voice(cache, allocator_floor=12), 12)

    def test_allocator_floor_excludes_lower_free_voice(self) -> None:
        cache = [1] * PLAYSTATION_SPU_VOICE_COUNT
        cache[3] = 0
        cache[15] = 0
        self.assertEqual(find_free_voice(cache, allocator_floor=12), 15)

    def test_force_full_scan_bypasses_allocator_floor_for_rekey_path(self) -> None:
        cache = [1] * PLAYSTATION_SPU_VOICE_COUNT
        cache[3] = 0
        cache[15] = 0
        self.assertEqual(find_free_voice(cache, allocator_floor=12, force_full_scan=True), 3)

    def test_quietest_steal_uses_first_strict_minimum(self) -> None:
        cache = [100] * PLAYSTATION_SPU_VOICE_COUNT
        cache[7] = 4
        cache[8] = 4
        cache[9] = 5
        self.assertEqual(steal_quietest_voice(cache), 7)

    def test_protected_sentinel_is_not_stealable(self) -> None:
        cache = [PROTECTED_ENVELOPE_SENTINEL] * PLAYSTATION_SPU_VOICE_COUNT
        cache[8] = 100
        self.assertEqual(steal_quietest_voice(cache), 8)

    def test_all_protected_voices_produce_explicit_exhaustion(self) -> None:
        cache = [PROTECTED_ENVELOPE_SENTINEL] * PLAYSTATION_SPU_VOICE_COUNT
        decision = allocate_dynamic_voice(cache)
        self.assertEqual(decision.voice_index, VOICE_INVALID_INDEX)
        self.assertFalse(decision.stole_voice)
        self.assertTrue(decision.exhausted)

    def test_no_free_voice_steals_quietest_eligible_voice(self) -> None:
        cache = [100] * PLAYSTATION_SPU_VOICE_COUNT
        cache[18] = 2
        decision = allocate_dynamic_voice(cache, allocator_floor=12)
        self.assertEqual(decision.voice_index, 18)
        self.assertTrue(decision.stole_voice)
        self.assertFalse(decision.exhausted)

    def test_free_voice_wins_before_steal_path(self) -> None:
        cache = [100] * PLAYSTATION_SPU_VOICE_COUNT
        cache[14] = 1
        cache[19] = 0
        decision = allocate_dynamic_voice(cache, allocator_floor=12)
        self.assertEqual(decision.voice_index, 19)
        self.assertFalse(decision.stole_voice)
        self.assertFalse(decision.exhausted)

    def test_invalid_snapshot_shapes_fail_closed(self) -> None:
        with self.assertRaises(ValueError):
            channel_mask_to_voice_mask([VOICE_INVALID_INDEX] * 31, 1)
        with self.assertRaises(ValueError):
            effective_envelope_cache([0] * 23)
        with self.assertRaises(ValueError):
            find_free_voice([0] * 24, allocator_floor=25)


if __name__ == "__main__":
    unittest.main()
