import unittest

from components.usf.oot_mml import (
    OOT_MML_EXECUTION_CONTRACT,
    OotMmlEffect,
    OotMmlScope,
    OotStaticRisk,
    authored_path_requires_runtime_confirmation,
    classify_oot_opcode,
    musical_effects,
)


class OotMmlSemanticTests(unittest.TestCase):
    def test_execution_contract_preserves_parallel_authored_program(self) -> None:
        self.assertEqual(OOT_MML_EXECUTION_CONTRACT.max_channels, 16)
        self.assertEqual(OOT_MML_EXECUTION_CONTRACT.max_layers_per_channel, 4)
        self.assertEqual(OOT_MML_EXECUTION_CONTRACT.max_call_depth, 4)
        self.assertTrue(OOT_MML_EXECUTION_CONTRACT.channel_execution_is_parallel)
        self.assertTrue(OOT_MML_EXECUTION_CONTRACT.layer_execution_is_parallel)
        self.assertTrue(OOT_MML_EXECUTION_CONTRACT.local_delays_block_only_local_script)
        self.assertTrue(OOT_MML_EXECUTION_CONTRACT.sequence_memory_can_be_modified)

    def test_sequence_channel_spawn_and_embedded_indices(self) -> None:
        load_channel = classify_oot_opcode(OotMmlScope.SEQUENCE, 0x95)
        self.assertEqual(load_channel.name, "LDCHAN")
        self.assertEqual(load_channel.encoded_argument, 5)
        self.assertEqual(load_channel.effect, OotMmlEffect.TOPOLOGY)
        self.assertTrue(load_channel.starts_parallel_script)

        load_layer = classify_oot_opcode(OotMmlScope.CHANNEL, 0x8B)
        self.assertEqual(load_layer.name, "LDLAYER")
        self.assertEqual(load_layer.encoded_argument, 3)
        self.assertTrue(load_layer.starts_parallel_script)

        delete_layer = classify_oot_opcode(OotMmlScope.CHANNEL, 0x92)
        self.assertEqual(delete_layer.name, "DELLAYER")
        self.assertEqual(delete_layer.encoded_argument, 2)
        self.assertTrue(delete_layer.stops_parallel_script)

    def test_layer_note_families_expose_pitch_without_claiming_full_event_decode(self) -> None:
        note_dvg = classify_oot_opcode(OotMmlScope.LAYER, 0x23)
        note_dv = classify_oot_opcode(OotMmlScope.LAYER, 0x63)
        note_vg = classify_oot_opcode(OotMmlScope.LAYER, 0xA3)

        self.assertEqual(note_dvg.name, "NOTEDVG")
        self.assertEqual(note_dv.name, "NOTEDV")
        self.assertEqual(note_vg.name, "NOTEVG")
        self.assertEqual(note_dvg.encoded_argument, 0x23)
        self.assertEqual(note_dv.encoded_argument, 0x23)
        self.assertEqual(note_vg.encoded_argument, 0x23)
        self.assertEqual(note_dvg.effect, OotMmlEffect.NOTE)

    def test_primary_musical_controls_are_exposed_at_correct_scope(self) -> None:
        self.assertEqual(
            classify_oot_opcode("sequence", 0xDD).effect,
            OotMmlEffect.TEMPO,
        )
        self.assertEqual(
            classify_oot_opcode("sequence", 0xDF).effect,
            OotMmlEffect.PITCH,
        )
        self.assertEqual(
            classify_oot_opcode("channel", 0xC1).effect,
            OotMmlEffect.TIMBRE,
        )
        self.assertEqual(
            classify_oot_opcode("channel", 0xD8).effect,
            OotMmlEffect.MODULATION,
        )
        self.assertEqual(
            classify_oot_opcode("layer", 0xC4).effect,
            OotMmlEffect.ARTICULATION,
        )

    def test_authored_mix_parameters_are_preserved_without_spatial_interpretation(self) -> None:
        pan = classify_oot_opcode("channel", 0xDD)
        note_pan = classify_oot_opcode("layer", 0xCA)
        self.assertEqual(pan.name, "PAN")
        self.assertEqual(note_pan.name, "NOTEPAN")
        self.assertEqual(pan.effect, OotMmlEffect.MIX_PARAMETER)
        self.assertEqual(note_pan.effect, OotMmlEffect.MIX_PARAMETER)

    def test_self_modification_and_dynamic_flow_force_runtime_confirmation(self) -> None:
        static_tempo = classify_oot_opcode("sequence", 0xDD)
        stseq = classify_oot_opcode("sequence", 0xC7)
        channel_stseq = classify_oot_opcode("channel", 0xC7)
        stptr = classify_oot_opcode("channel", 0xCF)
        dyncall = classify_oot_opcode("channel", 0xE4)
        random_velocity = classify_oot_opcode("channel", 0xB9)

        self.assertEqual(stseq.static_risk, OotStaticRisk.SELF_MODIFYING)
        self.assertTrue(stseq.writes_sequence_memory)
        self.assertTrue(channel_stseq.writes_sequence_memory)
        self.assertTrue(stptr.writes_sequence_memory)
        self.assertEqual(dyncall.static_risk, OotStaticRisk.DYNAMIC_TARGET)
        self.assertEqual(random_velocity.static_risk, OotStaticRisk.RANDOMIZED)

        self.assertFalse(authored_path_requires_runtime_confirmation([static_tempo]))
        self.assertTrue(authored_path_requires_runtime_confirmation([static_tempo, stseq]))
        self.assertTrue(authored_path_requires_runtime_confirmation([dyncall]))
        self.assertTrue(authored_path_requires_runtime_confirmation([random_velocity]))

    def test_conditional_flow_is_not_flattened_into_exact_form(self) -> None:
        branch = classify_oot_opcode("sequence", 0xFA)
        jump = classify_oot_opcode("sequence", 0xFB)
        loop = classify_oot_opcode("channel", 0xF8)

        self.assertEqual(branch.name, "BEQZ")
        self.assertEqual(branch.static_risk, OotStaticRisk.STATE_DEPENDENT)
        self.assertEqual(jump.static_risk, OotStaticRisk.NONE)
        self.assertEqual(loop.name, "LOOP")
        self.assertEqual(loop.scope, OotMmlScope.CHANNEL)

    def test_musical_projection_excludes_memory_control_and_mix_metadata(self) -> None:
        semantics = [
            classify_oot_opcode("sequence", 0xDD),
            classify_oot_opcode("channel", 0xC1),
            classify_oot_opcode("channel", 0xC7),
            classify_oot_opcode("channel", 0xDD),
            classify_oot_opcode("layer", 0x01),
        ]
        effects = musical_effects(semantics)
        self.assertEqual(
            effects,
            frozenset({OotMmlEffect.TEMPO, OotMmlEffect.TIMBRE, OotMmlEffect.NOTE}),
        )

    def test_unknown_and_invalid_opcodes_fail_conservatively(self) -> None:
        unknown = classify_oot_opcode("sequence", 0xCB)
        self.assertEqual(unknown.effect, OotMmlEffect.UNKNOWN)
        self.assertEqual(unknown.static_risk, OotStaticRisk.STATE_DEPENDENT)
        self.assertTrue(authored_path_requires_runtime_confirmation([unknown]))

        with self.assertRaises(ValueError):
            classify_oot_opcode("bogus", 0xDD)
        with self.assertRaises(ValueError):
            classify_oot_opcode("sequence", 256)


if __name__ == "__main__":
    unittest.main()
