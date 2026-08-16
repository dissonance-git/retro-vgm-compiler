import unittest

from components.nds.sseq import (
    SSEQ_EXECUTION_CONTRACT,
    SseqEffect,
    SseqStaticRisk,
    authored_path_requires_runtime_confirmation,
    classify_sseq_opcode,
    musical_effects,
)


class NdsSseqSemanticTests(unittest.TestCase):
    def test_execution_contract_preserves_multitrack_program(self) -> None:
        self.assertEqual(SSEQ_EXECUTION_CONTRACT.max_tracks, 16)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.notes_encode_pitch_in_opcode)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.tracks_execute_concurrently)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.has_variables)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.has_conditional_execution)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.has_randomized_arguments)
        self.assertTrue(SSEQ_EXECUTION_CONTRACT.has_calls_and_jumps)

    def test_note_opcodes_expose_pitch_without_claiming_full_event_decode(self) -> None:
        low = classify_sseq_opcode(0x00)
        middle_c = classify_sseq_opcode(0x3C)
        high = classify_sseq_opcode(0x7F)
        self.assertEqual(low.name, "NOTE")
        self.assertEqual(middle_c.encoded_pitch, 60)
        self.assertEqual(high.encoded_pitch, 127)
        self.assertEqual(middle_c.effect, SseqEffect.NOTE)

    def test_track_topology_and_global_controls_are_explicit(self) -> None:
        open_track = classify_sseq_opcode(0x93)
        define_tracks = classify_sseq_opcode(0xFE)
        tempo = classify_sseq_opcode(0xE1)
        end_track = classify_sseq_opcode(0xFF)

        self.assertTrue(open_track.opens_track)
        self.assertEqual(open_track.effect, SseqEffect.TRACK_TOPOLOGY)
        self.assertEqual(define_tracks.name, "DEFINE_TRACKS")
        self.assertTrue(define_tracks.global_scope)
        self.assertEqual(tempo.effect, SseqEffect.TEMPO)
        self.assertTrue(tempo.global_scope)
        self.assertTrue(end_track.ends_track)

    def test_pitch_bend_names_follow_runtime_meaning_not_misleading_alias(self) -> None:
        bend = classify_sseq_opcode(0xC4)
        bend_range = classify_sseq_opcode(0xC5)
        self.assertEqual(bend.name, "PITCH_BEND")
        self.assertEqual(bend_range.name, "PITCH_BEND_RANGE")
        self.assertEqual(bend.effect, SseqEffect.PITCH)
        self.assertEqual(bend_range.effect, SseqEffect.PITCH)

    def test_random_variable_and_if_commands_mark_static_path_limits(self) -> None:
        random = classify_sseq_opcode(0xA0)
        from_variable = classify_sseq_opcode(0xA1)
        condition = classify_sseq_opcode(0xA2)
        rand_variable = classify_sseq_opcode(0xB6)

        self.assertTrue(random.wraps_subcommand)
        self.assertEqual(random.static_risk, SseqStaticRisk.RANDOMIZED)
        self.assertTrue(from_variable.wraps_subcommand)
        self.assertEqual(from_variable.static_risk, SseqStaticRisk.DYNAMIC_ARGUMENT)
        self.assertEqual(condition.static_risk, SseqStaticRisk.STATE_DEPENDENT)
        self.assertEqual(rand_variable.static_risk, SseqStaticRisk.RANDOMIZED)

        self.assertTrue(
            authored_path_requires_runtime_confirmation([random, from_variable, condition])
        )
        self.assertFalse(
            authored_path_requires_runtime_confirmation(
                [classify_sseq_opcode(0x80), classify_sseq_opcode(0x81)]
            )
        )

    def test_musical_controls_cover_arrangement_and_performance_dimensions(self) -> None:
        semantics = [
            classify_sseq_opcode(0x3C),
            classify_sseq_opcode(0x81),
            classify_sseq_opcode(0xC1),
            classify_sseq_opcode(0xC3),
            classify_sseq_opcode(0xC8),
            classify_sseq_opcode(0xCA),
            classify_sseq_opcode(0xD4),
            classify_sseq_opcode(0xE1),
        ]
        self.assertEqual(
            musical_effects(semantics),
            frozenset(
                {
                    SseqEffect.NOTE,
                    SseqEffect.TIMBRE,
                    SseqEffect.DYNAMICS,
                    SseqEffect.PITCH,
                    SseqEffect.ARTICULATION,
                    SseqEffect.MODULATION,
                    SseqEffect.LOOP,
                    SseqEffect.TEMPO,
                }
            ),
        )

    def test_pan_is_preserved_as_mix_metadata_without_spatial_claim(self) -> None:
        pan = classify_sseq_opcode(0xC0)
        self.assertEqual(pan.name, "PAN")
        self.assertEqual(pan.effect, SseqEffect.MIX_PARAMETER)
        self.assertNotIn(pan.effect, musical_effects([pan]))

    def test_unknown_and_invalid_commands_fail_conservatively(self) -> None:
        unknown = classify_sseq_opcode(0xE2)
        self.assertEqual(unknown.effect, SseqEffect.UNKNOWN)
        self.assertEqual(unknown.static_risk, SseqStaticRisk.STATE_DEPENDENT)
        self.assertTrue(authored_path_requires_runtime_confirmation([unknown]))

        with self.assertRaises(ValueError):
            classify_sseq_opcode(-1)
        with self.assertRaises(ValueError):
            classify_sseq_opcode(256)


if __name__ == "__main__":
    unittest.main()
