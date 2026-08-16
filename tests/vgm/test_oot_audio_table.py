import struct
import unittest

from components.usf.oot_audio_table import (
    OotAudioCachePolicy,
    OotAudioStorageMedium,
    OotAudioTableError,
    assess_oot_sequence_entries,
    parse_oot_audio_table,
    scan_usf_oot_sequence_entries,
)
from components.usf.usf import UsfEffectiveState
from components.xsf.provenance import ByteContribution


def make_table(entries, *, rom_addr=0, unknown_medium_param=0):
    data = bytearray(0x10 + 0x10 * len(entries))
    struct.pack_into(">hhI", data, 0, len(entries), unknown_medium_param, rom_addr)
    for index, entry in enumerate(entries):
        struct.pack_into(
            ">IIbbhhh",
            data,
            0x10 + 0x10 * index,
            entry[0],
            entry[1],
            entry[2],
            entry[3],
            entry[4],
            entry[5],
            entry[6],
        )
    return bytes(data)


class OotAudioTableTests(unittest.TestCase):
    def test_parses_decomp_grounded_big_endian_audio_table(self):
        table_bytes = make_table(
            [
                (0x20, 4, 2, 1, 0x1234, -2, 7),
                (0x30, 8, 2, 3, 0, 0, 0),
            ],
            rom_addr=0x11223344,
            unknown_medium_param=-1,
        )
        table = parse_oot_audio_table(table_bytes, 0)
        self.assertEqual(table.num_entries, 2)
        self.assertEqual(table.unknown_medium_param, -1)
        self.assertEqual(table.rom_addr, 0x11223344)
        self.assertEqual(table.entries[0].rom_addr, 0x20)
        self.assertEqual(table.entries[0].size, 4)
        self.assertEqual(table.entries[0].medium, OotAudioStorageMedium.CART)
        self.assertEqual(table.entries[0].cache_policy, OotAudioCachePolicy.LOAD_PERSISTENT)
        self.assertEqual(table.entries[0].short_data1, 0x1234)
        self.assertEqual(table.entries[0].short_data2, -2)
        self.assertEqual(table.entries[0].table_offset, 0x10)

    def test_rejects_truncated_unobserved_and_implausible_tables(self):
        with self.assertRaises(OotAudioTableError):
            parse_oot_audio_table(b"\x00" * 8, 0)

        reserved = bytearray(make_table([]))
        reserved[8] = 1
        with self.assertRaises(OotAudioTableError):
            parse_oot_audio_table(bytes(reserved), 0)

        too_many = bytearray(0x10)
        struct.pack_into(">hhI", too_many, 0, 513, 0, 0)
        with self.assertRaises(OotAudioTableError):
            parse_oot_audio_table(bytes(too_many), 0, max_entries=512)

        complete = make_table([(0, 1, 2, 0, 0, 0, 0)])
        with self.assertRaises(OotAudioTableError):
            parse_oot_audio_table(complete, 0, observed_ranges=((0, 0x10),))

    def test_sequence_data_base_is_explicit_and_cart_spans_are_bounded(self):
        table_bytes = make_table(
            [
                (0x00, 4, 2, 0, 0, 0, 0),
                (0x04, 3, 0, 0, 0, 0, 0),
                (0x07, 0, 2, 0, 0, 0, 0),
                (0x40, 4, 2, 0, 0, 0, 0),
            ]
        )
        rom = bytearray(0xB0)
        rom[: len(table_bytes)] = table_bytes
        rom[0x80:0x84] = b"SEQ0"
        rom[0x84:0x87] = b"RAM"
        table = parse_oot_audio_table(bytes(rom), 0)
        assessments = assess_oot_sequence_entries(
            bytes(rom),
            table,
            sequence_data_base=0x80,
        )

        self.assertTrue(assessments[0].accepted)
        self.assertEqual(assessments[0].candidate.data, b"SEQ0")
        self.assertEqual(assessments[0].candidate.data_start, 0x80)
        self.assertEqual(assessments[1].classification, "non-cart-entry")
        self.assertEqual(assessments[2].classification, "zero-size-entry")
        self.assertEqual(assessments[3].classification, "rejected-sequence-span")

    def test_usf_sparse_zero_fill_and_shadowed_patches_never_become_fake_evidence(self):
        table = make_table(
            [
                (0x00, 4, 2, 0, 0, 0, 0),
                (0x08, 4, 2, 0, 0, 0, 0),
            ]
        )
        rom = bytearray(0x4C)
        rom[: len(table)] = table
        rom[0x40:0x44] = b"GOOD"
        # 0x48:0x4C stays zero in the effective allocation but has no source.

        contributions = (
            ByteContribution(
                source_id="oot.usflib",
                source_offset=0x100,
                target_start=0,
                target_end=len(table),
                stage_index=0,
                role="n64-rom",
            ),
            ByteContribution(
                source_id="oot.usflib",
                source_offset=0x180,
                target_start=0x40,
                target_end=0x44,
                stage_index=0,
                role="n64-rom",
            ),
            ByteContribution(
                source_id="song.miniusf",
                source_offset=0x200,
                target_start=0x40,
                target_end=0x44,
                stage_index=1,
                role="n64-rom",
            ),
        )
        state = UsfEffectiveState(
            root="song.miniusf",
            rom=bytes(rom),
            save_state=b"",
            contributions=contributions,
        )

        assessments = scan_usf_oot_sequence_entries(
            state,
            table_offset=0,
            sequence_data_base=0x40,
        )
        self.assertTrue(assessments[0].accepted)
        self.assertEqual(assessments[0].candidate.data, b"GOOD")
        self.assertEqual(len(assessments[0].candidate.provenance), 1)
        self.assertEqual(assessments[0].candidate.provenance[0].source_id, "song.miniusf")
        self.assertFalse(assessments[1].accepted)
        self.assertIn("unobserved USF ROM bytes", assessments[1].reasons[0])

    def test_table_itself_must_be_observed_in_usf(self):
        table = make_table([(0, 4, 2, 0, 0, 0, 0)])
        rom = table + b"\x00" * 0x40
        state = UsfEffectiveState(
            root="song.miniusf",
            rom=rom,
            save_state=b"",
            contributions=(
                ByteContribution(
                    source_id="song.miniusf",
                    source_offset=0,
                    target_start=0x40,
                    target_end=0x44,
                    stage_index=0,
                    role="n64-rom",
                ),
            ),
        )
        with self.assertRaises(OotAudioTableError):
            scan_usf_oot_sequence_entries(
                state,
                table_offset=0,
                sequence_data_base=0x40,
            )


if __name__ == "__main__":
    unittest.main()
