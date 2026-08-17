import json
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

    def test_builds_two_exact_game_grid_sources(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_pcm16_mono(root / "a.wav", 32)
            self.write_pcm16_mono(root / "b.wav", 48, rate=48000)
            manifest = {
                "schema": "spc-prebrr-sidecar-manifest-001",
                "sources": [
                    {
                        "source_number": 3,
                        "first_brr_block_address": "0x8000",
                        "prepared_pcm_wav": "a.wav",
                    },
                    {
                        "source_number": 9,
                        "first_brr_block_address": 0xFFF9,
                        "prepared_pcm_wav": "b.wav",
                    },
                ],
            }
            packet = build_sidecar(manifest, root)
            entries = parse_sidecar(packet)
            self.assertEqual(len(entries), 2)
            self.assertEqual(entries[0]["source_number"], 3)
            self.assertEqual(entries[0]["block_count"], 2)
            self.assertEqual(entries[1]["first_brr_block_address"], 0xFFF9)
            self.assertEqual(entries[1]["block_count"], 3)

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
