from __future__ import annotations

import copy
import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "hes" / "validate_forensic_sidecar.py"
SPEC = importlib.util.spec_from_file_location("validate_forensic_sidecar", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
validate = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = validate
SPEC.loader.exec_module(validate)


def sidecar() -> dict:
    return {
        "model": validate.EXPECTED_MODEL,
        "schema_version": 1,
        "claim_boundary": "opaque execution only",
        "provenance": {
            "retro_vgm_compiler_commit": "1" * 40,
            "libgme_repository": validate.EXPECTED_LIBGME_REPOSITORY,
            "libgme_commit": validate.EXPECTED_LIBGME_COMMIT,
            "instrumentation_contract": validate.EXPECTED_INSTRUMENTATION_CONTRACT,
            "clock_rate_hz": 1000,
        },
        "capture": {
            "track_index": 7,
            "playlist_loaded": True,
            "source_size_bytes": 1234,
            "playlist_size_bytes": 234,
            "requested_seconds": 2,
            "captured_clocks": 2000,
            "warning_count": 0,
            "capture_complete": True,
        },
        "psg_writes": {
            "count": 3,
            "clock": [0, 1000, 2000],
            "register_offset": [0, 2, 9],
            "data": [1, 52, 255],
        },
        "adpcm_writes": {
            "count": 2,
            "clock": [100, 1900],
            "register_offset": [0, 1023],
            "data": [85, 170],
        },
    }


class HesForensicSidecarValidationTest(unittest.TestCase):
    def validate(self, value: dict) -> None:
        validate.validate(
            value,
            source_size=1234,
            playlist_size=234,
            playlist_loaded=True,
            track_index=7,
            seconds=2,
        )

    def test_complete_creator_blind_capture_is_admissible(self):
        self.validate(sidecar())

    def test_label_and_source_identity_leakage_fail_closed(self):
        for key in ("composer", "game", "source_path", "m3u_file"):
            with self.subTest(key=key):
                value = sidecar()
                value["diagnostics"] = {key: "poison"}
                with self.assertRaisesRegex(ValueError, "forbidden"):
                    self.validate(value)

    def test_runtime_or_instrumentation_drift_fails_closed(self):
        for key, replacement in (
            ("libgme_commit", "2" * 40),
            ("instrumentation_contract", "future-contract"),
            ("retro_vgm_compiler_commit", "unknown"),
        ):
            with self.subTest(key=key):
                value = sidecar()
                value["provenance"][key] = replacement
                with self.assertRaises(ValueError):
                    self.validate(value)

    def test_warning_or_incomplete_capture_is_not_cache_admissible(self):
        for field, replacement in (("warning_count", 1), ("capture_complete", False)):
            with self.subTest(field=field):
                value = sidecar()
                value["capture"][field] = replacement
                with self.assertRaisesRegex(ValueError, "not cache-admissible"):
                    self.validate(value)

    def test_cache_identity_must_match_source_playlist_track_and_duration(self):
        cases = {
            "source_size": {"source_size": 1235},
            "playlist_size": {"playlist_size": 235},
            "playlist_mode": {"playlist_loaded": False},
            "track": {"track_index": 8},
            "duration": {"seconds": 3},
        }
        for name, overrides in cases.items():
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate.validate(
                    sidecar(),
                    source_size=overrides.get("source_size", 1234),
                    playlist_size=overrides.get("playlist_size", 234),
                    playlist_loaded=overrides.get("playlist_loaded", True),
                    track_index=overrides.get("track_index", 7),
                    seconds=overrides.get("seconds", 2),
                )

    def test_absolute_clocks_cannot_move_backward_or_escape_capture(self):
        for clocks in ([0, 1000, 999], [0, 1000, 2001]):
            with self.subTest(clocks=clocks):
                value = sidecar()
                value["psg_writes"]["clock"] = list(clocks)
                with self.assertRaisesRegex(ValueError, "monotonic"):
                    self.validate(value)

    def test_column_lengths_and_device_register_ranges_are_exact(self):
        value = sidecar()
        value["psg_writes"]["data"] = [1, 2]
        with self.assertRaisesRegex(ValueError, "length"):
            self.validate(value)

        value = sidecar()
        value["adpcm_writes"]["register_offset"][1] = 1024
        with self.assertRaisesRegex(ValueError, "device range"):
            self.validate(value)

    def test_capture_clock_count_must_match_duration_and_runtime_rate(self):
        value = sidecar()
        value["capture"]["captured_clocks"] = 1999
        with self.assertRaisesRegex(ValueError, "clock count"):
            self.validate(value)


if __name__ == "__main__":
    unittest.main()
