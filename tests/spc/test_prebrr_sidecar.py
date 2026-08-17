from pathlib import Path
import tempfile
import unittest
import wave

from tools.spc_prebrr_sidecar import build_sidecar, parse_sidecar


class PreBrrSidecarTests(unittest.TestCase):
    @staticmethod
    def write_pcm16_mono(path: Path, frames: int, rate: int = 32000) -> None:
        with wave.open(str(path), "wb") as handle:
            handle.setnchannels(1)
            handle.setsampwidth(2)
            handle.setframerate(rate)
            payload = bytearray()
            for frame in range(frames):
                sample = ((frame * 997) % 40000) - 20000
                payload += int(sample).to_bytes(2, "little", signed=True)
            handle.writeframes(bytes(payload))

    @staticmethod
    def write_spc(path: Path) -> None:
        data = bytearray(0x10200)
        signature = b"SNES-SPC700 Sound File Data"
        data[: len(signature)] = signature

        # DIR = $20. SRCN 3 -> $8000, two BRR blocks. SRCN 9 -> $FFF9,
        # three BRR blocks crossing the 16-bit RAM wrap.
        data[0x10100 + 0x5D] = 0x20

        def set_directory(srcn: int, start: int) -> None:
            entry = 0x100 + 0x2000 + srcn * 4
            data[entry] = start & 0xFF
            data[entry + 1] = (start >> 8) & 0xFF
            data[entry + 2] = start & 0xFF
            data[entry + 3] = (start >> 8) & 0xFF

        def set_brr_extent(start: int, blocks: int) -> None:
            address = start
            for block in range(blocks):
                # END only on the final block. Payload bytes may remain zero.
                data[0x100 + (address & 0xFFFF)] = 0x01 if block + 1 == blocks else 0x00
                address = (address + 9) & 0xFFFF

        set_directory(3, 0x8000)
        set_brr_extent(0x8000, 2)
        set_directory(9, 0xFFF9)
        set_brr_extent(0xFFF9, 3)
        path.write_bytes(data)

    def test_builds_two_exact_game_grid_sources_from_spc(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_spc(root / "music.spc")
            self.write_pcm16_mono(root / "a.wav", 32)
            self.write_pcm16_mono(root / "b.wav", 48, rate=48000)
            manifest = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "spc_file": "music.spc",
                "sources": [
                    {
                        "source_number": 3,
                        "prepared_pcm_wav": "a.wav",
                    },
                    {
                        "source_number": 9,
                        "prepared_pcm_wav": "b.wav",
                    },
                ],
            }
            packet = build_sidecar(manifest, root)
            entries = parse_sidecar(packet)
            self.assertEqual(len(entries), 2)
            self.assertEqual(entries[0]["source_number"], 3)
            self.assertEqual(entries[0]["first_brr_block_address"], 0x8000)
            self.assertEqual(entries[0]["block_count"], 2)
            self.assertEqual(entries[1]["first_brr_block_address"], 0xFFF9)
            self.assertEqual(entries[1]["block_count"], 3)

    def test_spc_address_and_extent_are_assertions(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_spc(root / "music.spc")
            self.write_pcm16_mono(root / "two-block.wav", 32)

            wrong_address = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "spc_file": "music.spc",
                "sources": [
                    {
                        "source_number": 3,
                        "first_brr_block_address": "0x9000",
                        "prepared_pcm_wav": "two-block.wav",
                    }
                ],
            }
            with self.assertRaises(ValueError):
                build_sidecar(wrong_address, root)

            self.write_pcm16_mono(root / "one-block.wav", 16)
            wrong_extent = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "spc_file": "music.spc",
                "sources": [
                    {
                        "source_number": 3,
                        "prepared_pcm_wav": "one-block.wav",
                    }
                ],
            }
            with self.assertRaises(ValueError):
                build_sidecar(wrong_extent, root)

    def test_rejects_hidden_conversion_and_ambiguity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_pcm16_mono(root / "bad-length.wav", 31)
            bad_length = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "sources": [
                    {
                        "source_number": 1,
                        "first_brr_block_address": 0,
                        "prepared_pcm_wav": "bad-length.wav",
                    }
                ],
            }
            with self.assertRaises(ValueError):
                build_sidecar(bad_length, root)

            self.write_pcm16_mono(root / "ok.wav", 16)
            duplicate = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "sources": [
                    {
                        "source_number": 5,
                        "first_brr_block_address": 0x1000,
                        "prepared_pcm_wav": "ok.wav",
                    },
                    {
                        "source_number": 5,
                        "first_brr_block_address": 0x2000,
                        "prepared_pcm_wav": "ok.wav",
                    },
                ],
            }
            with self.assertRaises(ValueError):
                build_sidecar(duplicate, root)

    def test_parser_rejects_corruption(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_pcm16_mono(root / "ok.wav", 16)
            manifest = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "sources": [
                    {
                        "source_number": 2,
                        "first_brr_block_address": "0x1234",
                        "prepared_pcm_wav": "ok.wav",
                    }
                ],
            }
            packet = bytearray(build_sidecar(manifest, root))
            packet[12] ^= 1
            with self.assertRaises(ValueError):
                parse_sidecar(bytes(packet))


if __name__ == "__main__":
    unittest.main()
