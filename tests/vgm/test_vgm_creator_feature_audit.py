import gzip
import importlib.util
import pathlib
import struct
import sys
import tempfile
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL_PATH = REPO_ROOT / "tools" / "vgm_creator_feature_audit.py"

spec = importlib.util.spec_from_file_location("vgm_creator_feature_audit", TOOL_PATH)
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


class VgmCreatorFeatureAuditTest(unittest.TestCase):
    def test_structural_and_realization_views_can_disagree(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            corpus = pathlib.Path(temp_dir)

            # A and B have the same +2-semitone contour and normalized rhythm,
            # but deliberately different FM programming.
            (corpus / "A.vgz").write_bytes(
                gzip.compress(make_sequence([900, 1010, 1134, 1273], 1, 100))
            )
            (corpus / "B.vgz").write_bytes(
                gzip.compress(make_sequence([1000, 1122, 1260, 1414], 2, 200))
            )

            # C reuses A's realization but reverses the musical contour.
            (corpus / "C.vgz").write_bytes(
                gzip.compress(make_sequence([900, 802, 714, 636], 1, 100))
            )

            result = audit.audit_corpus(corpus, neighbor_count=2)

            self.assertIn("No embedded composer/artist metadata is read", result["label_policy"])
            self.assertIn("not yet persistent-part", result["claim_boundary"])

            structural = result["top_structural_neighbors"]
            realization = result["top_realization_neighbors"]

            self.assertEqual(structural["A.vgz"][0]["file"], "B.vgz")
            self.assertAlmostEqual(structural["A.vgz"][0]["score"], 1.0)

            self.assertEqual(realization["A.vgz"][0]["file"], "C.vgz")
            self.assertAlmostEqual(realization["A.vgz"][0]["score"], 1.0)

            tracks = {track["file"]: track for track in result["tracks"]}
            self.assertEqual(
                tracks["A.vgz"]["musical_trajectory"]["interval_histogram_semitones"],
                {"2": 3},
            )
            self.assertEqual(
                tracks["C.vgz"]["musical_trajectory"]["interval_histogram_semitones"],
                {"-2": 3},
            )
            self.assertNotEqual(
                set(tracks["A.vgz"]["realization"]["core_patch_usage"]),
                set(tracks["B.vgz"]["realization"]["core_patch_usage"]),
            )
            self.assertEqual(
                set(tracks["A.vgz"]["realization"]["core_patch_usage"]),
                set(tracks["C.vgz"]["realization"]["core_patch_usage"]),
            )


if __name__ == "__main__":
    unittest.main()
