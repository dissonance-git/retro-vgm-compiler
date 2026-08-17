from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "hes" / "creator_blind_hes_cache.py"
SPEC = importlib.util.spec_from_file_location("creator_blind_hes_cache", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
cache = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = cache
SPEC.loader.exec_module(cache)


def sidecar(source_size: int, playlist_size: int, track: int, seconds: int) -> dict:
    rate = 1000
    return {
        "model": cache.sidecar.EXPECTED_MODEL,
        "schema_version": 1,
        "claim_boundary": "opaque execution only",
        "provenance": {
            "retro_vgm_compiler_commit": "1" * 40,
            "libgme_repository": cache.sidecar.EXPECTED_LIBGME_REPOSITORY,
            "libgme_commit": cache.sidecar.EXPECTED_LIBGME_COMMIT,
            "instrumentation_contract": cache.sidecar.EXPECTED_INSTRUMENTATION_CONTRACT,
            "clock_rate_hz": rate,
        },
        "capture": {
            "track_index": track,
            "playlist_loaded": playlist_size > 0,
            "source_size_bytes": source_size,
            "playlist_size_bytes": playlist_size,
            "requested_seconds": seconds,
            "captured_clocks": seconds * rate,
            "warning_count": 0,
            "capture_complete": True,
        },
        "psg_writes": {"count": 1, "clock": [0], "register_offset": [0], "data": [1]},
        "adpcm_writes": {"count": 0, "clock": [], "register_offset": [], "data": []},
    }


class CreatorBlindHesCacheTest(unittest.TestCase):
    def setup_files(self, root: pathlib.Path):
        source = root / "opaque.hes"
        playlist = root / "opaque.m3u"
        extractor = root / "hes_forensic_features"
        source.write_bytes(b"HES" * 10)
        playlist.write_text("opaque playlist", encoding="utf-8")
        extractor.write_bytes(b"binary-placeholder")
        return source, playlist, extractor

    def fake_run(self, source: pathlib.Path, playlist: pathlib.Path):
        calls = []

        def run(command, check):
            self.assertTrue(check)
            calls.append(list(command))
            track = int(command[command.index("--track") + 1])
            seconds = int(command[command.index("--seconds") + 1])
            output = pathlib.Path(command[command.index("--json") + 1])
            value = sidecar(source.stat().st_size, playlist.stat().st_size, track, seconds)
            output.write_text(json.dumps(value), encoding="utf-8")
            return mock.Mock(returncode=0)

        return calls, run

    def test_identical_request_executes_once_then_reuses(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            source, playlist, extractor = self.setup_files(root)
            calls, fake = self.fake_run(source, playlist)
            kwargs = dict(
                corpus_id="opaque-corpus", track_index=7, extractor=extractor,
                seconds=5, cache_root=root / "cache", playlist=playlist,
            )
            with mock.patch.object(cache.subprocess, "run", side_effect=fake):
                destination, changed = cache.build_one(source, **kwargs)
                self.assertTrue(changed)
                self.assertTrue(destination.is_file())
                destination2, changed2 = cache.build_one(source, **kwargs)
                self.assertEqual(destination2, destination)
                self.assertFalse(changed2)
            self.assertEqual(len(calls), 1)

    def test_source_or_playlist_size_change_invalidates_without_hashing(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            source, playlist, extractor = self.setup_files(root)
            calls, fake = self.fake_run(source, playlist)
            kwargs = dict(
                corpus_id="opaque-corpus", track_index=7, extractor=extractor,
                seconds=5, cache_root=root / "cache", playlist=playlist,
            )
            with mock.patch.object(cache.subprocess, "run", side_effect=fake):
                cache.build_one(source, **kwargs)
                source.write_bytes(source.read_bytes() + b"x")
                cache.build_one(source, **kwargs)
                playlist.write_text(playlist.read_text(encoding="utf-8") + "x", encoding="utf-8")
                cache.build_one(source, **kwargs)
            self.assertEqual(len(calls), 3)

    def test_track_duration_and_playlist_mode_have_separate_destinations(self):
        source = pathlib.Path("tests/corpus/demo-hes/music.hes")
        playlist = pathlib.Path("tests/corpus/demo-hes/music.m3u")
        first = cache.destination_for(source, corpus_id="demo", track_index=1, seconds=5, playlist=playlist)
        second = cache.destination_for(source, corpus_id="demo", track_index=2, seconds=5, playlist=playlist)
        third = cache.destination_for(source, corpus_id="demo", track_index=1, seconds=10, playlist=playlist)
        raw = cache.destination_for(source, corpus_id="demo", track_index=1, seconds=5, playlist=None)
        self.assertEqual(len({first, second, third, raw}), 4)

    def test_warning_capture_is_never_reused(self):
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            source, playlist, _extractor = self.setup_files(root)
            destination = cache.destination_for(
                source, corpus_id="opaque", track_index=7, seconds=5,
                cache_root=root / "cache", playlist=playlist,
            )
            destination.parent.mkdir(parents=True)
            value = sidecar(source.stat().st_size, playlist.stat().st_size, 7, 5)
            value["capture"]["warning_count"] = 1
            value["capture"]["capture_complete"] = False
            destination.write_text(json.dumps(value), encoding="utf-8")
            self.assertFalse(cache.cache_current(
                destination, source, track_index=7, seconds=5, playlist=playlist
            ))


if __name__ == "__main__":
    unittest.main()
