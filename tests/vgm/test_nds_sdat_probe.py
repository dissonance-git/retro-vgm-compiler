import struct
import unittest

from components.nds.sdat_probe import (
    SDAT_HEADER_SIZE,
    accepted_sdat_candidates,
    assess_sdat_at,
    scan_sdat_candidates,
)
from components.twosf.twosf import TwoSfEffectiveState
from components.xsf.provenance import ByteContribution


def make_minimal_sdat() -> bytes:
    info_offset = 0x40
    info_size = 8
    fat_offset = info_offset + info_size
    fat_size = 12
    total_size = fat_offset + fat_size

    data = bytearray(total_size)
    data[:8] = b"SDAT\xff\xfe\x00\x01"
    struct.pack_into("<IHH", data, 8, total_size, SDAT_HEADER_SIZE, 2)
    struct.pack_into("<II", data, 0x18, info_offset, info_size)
    struct.pack_into("<II", data, 0x20, fat_offset, fat_size)
    data[info_offset : info_offset + 4] = b"INFO"
    struct.pack_into("<I", data, info_offset + 4, info_size)
    data[fat_offset : fat_offset + 4] = b"FAT "
    struct.pack_into("<II", data, fat_offset + 4, fat_size, 0)
    return bytes(data)


def contribution(source, start, end, stage=0):
    return ByteContribution(
        source_id=source,
        source_offset=0,
        target_start=start,
        target_end=end,
        stage_index=stage,
        role="nds-rom-map",
    )


class NdsSdatProbeTests(unittest.TestCase):
    def test_accepts_structural_candidate_with_observed_rom_span(self) -> None:
        sdat = make_minimal_sdat()
        prefix = b"ROM"
        state = TwoSfEffectiveState(
            root="song.mini2sf",
            rom=prefix + sdat,
            save_state=b"",
            rom_allocated_size=128,
            save_records=(),
            contributions=(contribution("base.2sflib", 0, len(prefix) + len(sdat)),),
        )
        candidates = accepted_sdat_candidates(state)
        self.assertEqual(len(candidates), 1)
        self.assertEqual(candidates[0].offset, len(prefix))
        self.assertEqual(candidates[0].data, sdat)
        self.assertEqual(candidates[0].provenance[0].source_id, "base.2sflib")

    def test_ascii_magic_with_invalid_sections_is_rejected(self) -> None:
        fake = bytearray(make_minimal_sdat())
        fake[0x40:0x44] = b"NOPE"
        assessment = assess_sdat_at(
            bytes(fake),
            0,
            contributions=(contribution("fake.2sf", 0, len(fake)),),
        )
        self.assertFalse(assessment.accepted)
        self.assertIn("INFO section magic", " ".join(assessment.reasons))

    def test_sparse_rom_zero_fill_cannot_complete_candidate(self) -> None:
        sdat = make_minimal_sdat()
        state = TwoSfEffectiveState(
            root="song.mini2sf",
            rom=sdat,
            save_state=b"",
            rom_allocated_size=128,
            save_records=(),
            contributions=(contribution("song.mini2sf", 0, SDAT_HEADER_SIZE),),
        )
        assessments = scan_sdat_candidates(state)
        self.assertEqual(len(assessments), 1)
        self.assertFalse(assessments[0].accepted)
        self.assertIn("unobserved 2SF ROM bytes", " ".join(assessments[0].reasons))

    def test_later_overlay_owns_final_candidate_provenance(self) -> None:
        sdat = make_minimal_sdat()
        state = TwoSfEffectiveState(
            root="song.mini2sf",
            rom=sdat,
            save_state=b"",
            rom_allocated_size=128,
            save_records=(),
            contributions=(
                contribution("base.2sflib", 0, len(sdat), 0),
                contribution("song.mini2sf", 0, len(sdat), 1),
            ),
        )
        candidates = accepted_sdat_candidates(state)
        self.assertEqual(len(candidates), 1)
        self.assertEqual(len(candidates[0].provenance), 1)
        self.assertEqual(candidates[0].provenance[0].source_id, "song.mini2sf")

    def test_multiple_signature_hits_remain_separate_assessments(self) -> None:
        sdat = make_minimal_sdat()
        fake = b"SDAT\xff\xfe\x00\x01" + b"\x00" * 64
        rom = sdat + fake
        state = TwoSfEffectiveState(
            root="song.mini2sf",
            rom=rom,
            save_state=b"",
            rom_allocated_size=256,
            save_records=(),
            contributions=(contribution("base.2sflib", 0, len(rom)),),
        )
        assessments = scan_sdat_candidates(state)
        self.assertEqual(len(assessments), 2)
        self.assertTrue(assessments[0].accepted)
        self.assertFalse(assessments[1].accepted)


if __name__ == "__main__":
    unittest.main()
