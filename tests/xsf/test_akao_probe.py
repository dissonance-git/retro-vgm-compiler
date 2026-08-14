from __future__ import annotations

import struct
import unittest

from components.psf.akao_probe import assess_akao_signature, scan_akao_signatures


def make_akao_v3_candidate(
    *,
    tracks: tuple[int, ...] = (0x60,),
    length: int = 0x90,
    sequence_id: int = 3,
    sample_set_id: int = 7,
    instrument_offset: int | None = 0x70,
    drumkit_offset: int | None = None,
) -> bytes:
    data = bytearray(length)
    data[:4] = b"AKAO"
    struct.pack_into("<H", data, 4, sequence_id)
    struct.pack_into("<H", data, 6, length)
    struct.pack_into("<H", data, 0x14, sample_set_id)
    struct.pack_into("<I", data, 0x20, (1 << len(tracks)) - 1)
    for index, target in enumerate(tracks):
        pointer = 0x40 + index * 2
        struct.pack_into("<H", data, pointer, target - pointer)
    if instrument_offset is not None:
        struct.pack_into("<I", data, 0x30, instrument_offset - 0x30)
    if drumkit_offset is not None:
        struct.pack_into("<I", data, 0x34, drumkit_offset - 0x34)
    return bytes(data)


class AkaoProbeTests(unittest.TestCase):
    def test_accepts_bounded_v3_structure_without_claiming_exact_version(self) -> None:
        memory = b"xx" + make_akao_v3_candidate(tracks=(0x50, 0x60), length=0x80) + b"zz"
        results = scan_akao_signatures(memory, memory_base=0x80010000)
        self.assertEqual(len(results), 1)
        result = results[0]
        self.assertTrue(result.accepted)
        self.assertEqual(result.classification, "v3-sequence-candidate")
        assert result.sequence is not None
        self.assertEqual(result.sequence.track_count, 2)
        self.assertEqual(result.sequence.sequence_id, 3)
        self.assertEqual(result.sequence.sample_set_id, 7)
        self.assertEqual(result.sequence.instrument_address, 0x80010000 + 2 + 0x70)
        self.assertIn("heuristic only", result.sequence.version_evidence)

    def test_zero_length_signature_is_not_promoted_to_sequence(self) -> None:
        data = bytearray(0x40)
        data[:4] = b"AKAO"
        result = assess_akao_signature(bytes(data), 0)
        self.assertFalse(result.accepted)
        self.assertEqual(result.classification, "non-sequence-signature")
        self.assertIn("sample collection", result.reasons[0])

    def test_rejects_nonzero_v3_reserved_word(self) -> None:
        data = bytearray(make_akao_v3_candidate())
        struct.pack_into("<I", data, 0x28, 1)
        result = assess_akao_signature(bytes(data), 0)
        self.assertFalse(result.accepted)
        self.assertIn("reserved word +0x28", " ".join(result.reasons))

    def test_rejects_track_pointer_outside_declared_span(self) -> None:
        data = bytearray(make_akao_v3_candidate(length=0x80))
        struct.pack_into("<H", data, 0x40, 0x100)
        result = assess_akao_signature(bytes(data), 0)
        self.assertFalse(result.accepted)
        self.assertIn("track 0 target", " ".join(result.reasons))

    def test_fe13_is_only_counted_as_raw_pair(self) -> None:
        data = bytearray(make_akao_v3_candidate())
        data[0x75:0x77] = b"\xFE\x13"
        result = assess_akao_signature(bytes(data), 0)
        self.assertTrue(result.accepted)
        assert result.sequence is not None
        self.assertEqual(result.sequence.raw_fe13_pairs, 1)

    def test_track_pointer_into_pointer_table_is_warning_not_fabricated_failure(self) -> None:
        data = bytearray(make_akao_v3_candidate(tracks=(0x40,), length=0x80))
        result = assess_akao_signature(bytes(data), 0)
        self.assertTrue(result.accepted)
        assert result.sequence is not None
        self.assertTrue(result.sequence.warnings)
        self.assertIn("pointer table", result.sequence.warnings[0])

    def test_sequence_layer_is_not_capped_to_playstation_spu_voice_count(self) -> None:
        # Independent AKAO tooling reports Chrono Cross: The Brink of Death with
        # 31 logical channels.  The PlayStation SPU has only 24 physical voices.
        # This synthetic structural control protects the representation boundary
        # without claiming all 31 tracks are simultaneously audible.
        track_count = 31
        pointer_table_end = 0x40 + 2 * track_count
        tracks = tuple(pointer_table_end + index for index in range(track_count))
        data = make_akao_v3_candidate(
            tracks=tracks,
            length=pointer_table_end + track_count + 0x20,
            instrument_offset=None,
        )
        result = assess_akao_signature(data, 0)
        self.assertTrue(result.accepted)
        assert result.sequence is not None
        self.assertEqual(result.sequence.track_count, 31)
        self.assertGreater(result.sequence.track_count, 24)


if __name__ == "__main__":
    unittest.main()
