from __future__ import annotations

import importlib.util
import json
import pathlib
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "corpus_import", ROOT / "tools" / "corpus_import.py"
)
assert SPEC is not None and SPEC.loader is not None
IMPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORTER)


class CorpusImportTest(unittest.TestCase):
    def test_runnable_and_playlist_sidecars_have_distinct_counts(self) -> None:
        repo = pathlib.Path(tempfile.mkdtemp(prefix="corpus-import-"))
        corpus = repo / "tests" / "corpus" / "example"
        corpus.mkdir(parents=True)
        (repo / "tests" / "corpus" / "manifest.json").write_text(
            json.dumps({"version": 2, "sets": []}), encoding="utf-8"
        )
        (corpus / "game.sgc").write_bytes(b"SGC\x1a\x01" + bytes(0x9B))
        (corpus / "track.m3u").write_text("game.sgc::KSS,0,Track,1:00\n", encoding="utf-8")

        IMPORTER.update_direct_record(repo, "example")
        manifest = json.loads((repo / "tests" / "corpus" / "manifest.json").read_text(encoding="utf-8"))
        record = manifest["sets"][0]

        self.assertEqual(record["fixture_count"], 1)
        self.assertEqual(record["sidecar_count"], 1)
        self.assertEqual(record["canonical_file_count"], 2)
        self.assertEqual(IMPORTER.verify_manifest(repo), 0)


if __name__ == "__main__":
    unittest.main()
