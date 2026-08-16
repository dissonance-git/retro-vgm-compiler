import gzip
import importlib.util
import pathlib
import struct
import sys
import tempfile
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "cross_soundtrack_vgm_audit.py"

spec = importlib.util.spec_from_file_location("cross_soundtrack_vgm_audit", TOOL_PATH)
audit = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = audit
assert spec.loader is not None
spec.loader.exec_module(audit)


def ym(register, value, port=0):
    return bytes([0x52 if port == 0 else 0x53, register, value])


def wait(samples):
    return bytes([0x61]) + struct.pack("<H", samples)


def key_on(channel=0):
    encoded = [0, 1, 2, 4, 5, 6][channel]
    return ym(0x28, 0xF0 | encoded)


def set_pitch(fnum, block, channel=0):
    port = 0 if channel < 3 else 1
    local = channel if channel < 3 else channel - 3
    return (
        ym(0xA0 + local, fnum & 0xFF, port)
        + ym(0xA4 + local, ((block & 7) << 3) | ((fnum >> 8) & 7), port)
    )


def patch(seed, channel=0):
    port = 0 if channel < 3 else 1
    local = channel if channel < 3 else channel - 3
    out = b""
    for slot in range(4):
        offset = slot * 4 + local
        out += ym(0x30 + offset, (seed + slot) & 0x7F, port)
        out += ym(0x40 + offset, (seed * 3 + slot) & 0x7F, port)
        out += ym(0x50 + offset, (seed + 5 + slot) & 0x7F, port)
        out += ym(0x60 + offset, (seed + 10 + slot) & 0x7F, port)
        out += ym(0x70 + offset, (seed + 15 + slot) & 0x7F, port)
        out += ym(0x80 + offset, (seed + 20 + slot) & 0xFF, port)
        out += ym(0x90 + offset, slot & 0x0F, port)
    out += ym(0xB0 + local, (seed & 7) | ((seed & 7) << 3), port)
    out += ym(0xB4 + local, 0xC0 | (seed & 7), port)
    return out


def make_vgm(commands):
    header = bytearray(0x40)
    header[:4] = b"Vgm "
    struct.pack_into("<I", header, 8, 0x150)
    struct.pack_into("<I", header, 0x34, 0)
    raw = bytearray(bytes(header) + commands + b"\x66")
    struct.pack_into("<I", raw, 4, len(raw) - 4)
    return bytes(raw)


def make_sequence(fnums, seed, gap):
    commands = patch(seed)
    for fnum in fnums:
        commands += set_pitch(fnum, 4)
        commands += key_on()
        commands += wait(gap)
        commands += ym(0x28, 0x00)
    return make_vgm(commands)


class CrossSoundtrackVgmAuditTest(unittest.TestCase):
    def test_cross_soundtrack_neighbors_exclude_local_shortcuts(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            soundtrack_one = root / "soundtrack-one"
            soundtrack_two = root / "soundtrack-two"
            soundtrack_one.mkdir()
            soundtrack_two.mkdir()

            (soundtrack_one / "A.vgz").write_bytes(
                gzip.compress(make_sequence([900, 1010, 1134, 1273], 1, 100))
            )
            (soundtrack_one / "C.vgz").write_bytes(
                gzip.compress(make_sequence([900, 802, 714, 636], 1, 100))
            )
            (soundtrack_two / "B.vgz").write_bytes(
                gzip.compress(make_sequence([1000, 1122, 1260, 1414], 2, 200))
            )

            result = audit.audit_soundtracks(
                [soundtrack_one, soundtrack_two],
                neighbor_count=2,
                cross_soundtrack_only=True,
            )

            self.assertTrue(result["cross_soundtrack_only"])
            self.assertEqual(result["soundtracks"], ["soundtrack-one", "soundtrack-two"])
            self.assertIn("No composer/artist metadata", result["label_policy"])

            structural = result["top_structural_neighbors"]
            realization = result["top_realization_neighbors"]

            self.assertEqual(
                structural["soundtrack-one::A.vgz"][0]["soundtrack_id"],
                "soundtrack-two",
            )
            self.assertEqual(
                structural["soundtrack-one::A.vgz"][0]["file"],
                "B.vgz",
            )
            self.assertAlmostEqual(
                structural["soundtrack-one::A.vgz"][0]["score"],
                1.0,
            )

            # The matching trajectory is intentionally not a matching FM
            # realization, proving the two views remain independent.
            self.assertLess(
                realization["soundtrack-one::A.vgz"][0]["score"],
                0.5,
            )

            for neighbor in structural["soundtrack-two::B.vgz"]:
                self.assertNotEqual(neighbor["soundtrack_id"], "soundtrack-two")


if __name__ == "__main__":
    unittest.main()
