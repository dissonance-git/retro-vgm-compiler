from pathlib import Path
import copy
import hashlib
import math
import struct
import tempfile
import unittest

from tools.spc_studio_source_sidecar import (
    HEADER_SIZE,
    ENTRY_SIZE,
    SPC_DIR_REGISTER,
    SPC_DSP_OFFSET,
    SPC_MIN_SIZE,
    SPC_RAM_OFFSET,
    build_sidecar,
    parse_sidecar,
)


class StudioSourceSidecarTests(unittest.TestCase):
    @staticmethod
    def write_wrapped_loop_spc(path: Path, *, loop_address: int = 0x0002) -> bytes:
        data = bytearray(SPC_MIN_SIZE)
        signature = b"SNES-SPC700 Sound File Data"
        data[: len(signature)] = signature

        directory_page = 0x4C
        source_number = 7
        first_address = 0xFFF0
        data[SPC_DSP_OFFSET + SPC_DIR_REGISTER] = directory_page

        directory_address = ((directory_page << 8) + source_number * 4) & 0xFFFF

        def write_ram_u16(address: int, value: int) -> None:
            data[SPC_RAM_OFFSET + (address & 0xFFFF)] = value & 0xFF
            data[SPC_RAM_OFFSET + ((address + 1) & 0xFFFF)] = (value >> 8) & 0xFF

        write_ram_u16(directory_address, first_address)
        write_ram_u16(directory_address + 2, loop_address)

        witness = bytearray(4 * 9)
        for block in range(4):
            witness[block * 9] = 0x03 if block == 3 else 0x00
            for byte in range(1, 9):
                witness[block * 9 + byte] = (block * 29 + byte * 7) & 0xFF

        for offset, value in enumerate(witness):
            address = (first_address + offset) & 0xFFFF
            data[SPC_RAM_OFFSET + address] = value

        path.write_bytes(data)
        return bytes(witness)

    @staticmethod
    def write_f32le(path: Path, *, include_nan: bool = False) -> bytes:
        samples = [
            0.31 * math.sin(frame * 0.17) - 0.12 * math.cos(frame * 0.43)
            for frame in range(96)
        ]
        if include_nan:
            samples[17] = math.nan
        payload = b"".join(struct.pack("<f", sample) for sample in samples)
        path.write_bytes(payload)
        return payload

    @staticmethod
    def admission() -> dict[str, object]:
        return {
            "relation": "exact_pre_brr_source",
            "evidence": "exact_upstream_source",
            "basis": "exact_upstream_pcm",
            "preparation_chain_exact": True,
            "identity_validation_passed": True,
        }

    @classmethod
    def manifest(cls, witness: bytes, pcm: bytes) -> dict[str, object]:
        return {
            "schema": "spc-studiosrc-sidecar-manifest-001",
            "spc_file": "music.spc",
            "sources": [
                {
                    "source_number": 7,
                    # Deliberately omit directory_page: the normal path should
                    # bind to the snapshot's current DSP DIR register.
                    "upstream_pcm_f32le": "source.f32le",
                    "upstream_pcm_sha256": hashlib.sha256(pcm).hexdigest(),
                    "game_brr_sha256": hashlib.sha256(witness).hexdigest(),
                    "game_brr_identity": {
                        "high": "0x1111222233334444",
                        "low": "0x5555666677778888",
                    },
                    "upstream_identity": {
                        "high": "0x9999aaaabbbbcccc",
                        "low": "0xddddeeeeffff0001",
                    },
                    "sample_rate_hz": 48000.0,
                    "game_pcm_units_per_source_unit": 32768.0,
                    "game_origin": 0.0,
                    "upstream_origin": 0.0,
                    "upstream_frames_per_game_sample": 1.0,
                    "upstream_loop_start": 32.0,
                    "admission": cls.admission(),
                }
            ],
        }

    def test_builds_content_bound_wraparound_packet_without_conversion(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            witness = self.write_wrapped_loop_spc(root / "music.spc")
            pcm = self.write_f32le(root / "source.f32le")
            manifest = self.manifest(witness, pcm)

            packet = build_sidecar(manifest, root)
            entries = parse_sidecar(packet)
            self.assertEqual(len(entries), 1)
            entry = entries[0]
            self.assertEqual(entry["source_number"], 7)
            self.assertEqual(entry["first_brr_block_address"], 0xFFF0)
            self.assertTrue(entry["loop_present"])
            self.assertEqual(entry["brr_block_count"], 4)
            self.assertEqual(entry["loop_block_ordinal"], 2)
            self.assertEqual(entry["pcm_frame_count"], 96)
            self.assertEqual(entry["sample_rate_hz"], 48000.0)
            self.assertEqual(entry["upstream_loop_start"], 32.0)

            brr_offset = int(entry["brr_offset_bytes"])
            pcm_offset = int(entry["pcm_offset_bytes"])
            self.assertEqual(packet[brr_offset : brr_offset + len(witness)], witness)
            self.assertEqual(packet[pcm_offset : pcm_offset + len(pcm)], pcm)

            # Four wrapped BRR blocks consume 36 bytes after the 120-byte
            # entry. The float payload must start at the exact 4-byte aligned
            # position expected by the C++ packet parser.
            self.assertEqual(brr_offset, HEADER_SIZE + ENTRY_SIZE)
            self.assertEqual(pcm_offset, (brr_offset + len(witness) + 3) & ~3)
            self.assertEqual(packet[brr_offset + len(witness) : pcm_offset], b"" )

    def test_hashes_and_admission_are_assertions_not_hints(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            witness = self.write_wrapped_loop_spc(root / "music.spc")
            pcm = self.write_f32le(root / "source.f32le")
            manifest = self.manifest(witness, pcm)

            wrong_brr = copy.deepcopy(manifest)
            wrong_brr["sources"][0]["game_brr_sha256"] = "00" * 32
            with self.assertRaises(ValueError):
                build_sidecar(wrong_brr, root)

            wrong_pcm = copy.deepcopy(manifest)
            wrong_pcm["sources"][0]["upstream_pcm_sha256"] = "11" * 32
            with self.assertRaises(ValueError):
                build_sidecar(wrong_pcm, root)

            weak_evidence = copy.deepcopy(manifest)
            weak_evidence["sources"][0]["admission"]["evidence"] = "probable_upstream_source"
            with self.assertRaises(ValueError):
                build_sidecar(weak_evidence, root)

            extra_admission_claim = copy.deepcopy(manifest)
            extra_admission_claim["sources"][0]["admission"]["reviewed"] = True
            with self.assertRaises(ValueError):
                build_sidecar(extra_admission_claim, root)

    def test_snapshot_loop_topology_cannot_be_overridden_by_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            # Same DIR page and SRCN, but the loop pointer is one byte off a BRR
            # block boundary. No manifest coordinate can launder that topology.
            witness = self.write_wrapped_loop_spc(
                root / "music.spc", loop_address=0x0003
            )
            pcm = self.write_f32le(root / "source.f32le")
            manifest = self.manifest(witness, pcm)
            with self.assertRaises(ValueError):
                build_sidecar(manifest, root)

            # Restore exact hardware topology, then contradict its game-sample
            # loop through the source coordinate map. This must also fail closed.
            witness = self.write_wrapped_loop_spc(root / "music.spc")
            manifest = self.manifest(witness, pcm)
            manifest["sources"][0]["upstream_loop_start"] = 31.0
            with self.assertRaises(ValueError):
                build_sidecar(manifest, root)

    def test_raw_pcm_must_be_exact_finite_binary32(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            witness = self.write_wrapped_loop_spc(root / "music.spc")
            bad_pcm = self.write_f32le(root / "source.f32le", include_nan=True)
            manifest = self.manifest(witness, bad_pcm)
            with self.assertRaises(ValueError):
                build_sidecar(manifest, root)

            # No implicit truncation or padding of a partial float frame.
            partial = b"\x00\x00\x00\x00\x01"
            (root / "source.f32le").write_bytes(partial)
            manifest = self.manifest(witness, partial)
            with self.assertRaises(ValueError):
                build_sidecar(manifest, root)

    def test_duplicate_runtime_identity_and_parser_corruption_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            witness = self.write_wrapped_loop_spc(root / "music.spc")
            pcm = self.write_f32le(root / "source.f32le")
            manifest = self.manifest(witness, pcm)

            duplicate = copy.deepcopy(manifest)
            duplicate["sources"].append(copy.deepcopy(duplicate["sources"][0]))
            with self.assertRaises(ValueError):
                build_sidecar(duplicate, root)

            packet = bytearray(build_sidecar(manifest, root))
            # Reserved entry tail begins at entry + 104.
            packet[HEADER_SIZE + 104] = 1
            with self.assertRaises(ValueError):
                parse_sidecar(bytes(packet))

            packet = bytearray(build_sidecar(manifest, root))
            packet[12] ^= 1  # corrupt declared packet size
            with self.assertRaises(ValueError):
                parse_sidecar(bytes(packet))


if __name__ == "__main__":
    unittest.main()
