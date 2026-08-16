from __future__ import annotations

import hashlib
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
JOIN = ROOT / "tests" / "corpus" / "sonic3-attribution-control-external-tags.jsonl"
MANIFEST = ROOT / "tests" / "corpus" / "manifest.json"
AUTHORITY = "foobar2000 external-tags.db ingested by Helix"
TARGET_PEOPLE = {
    "Tatsuyuki Maeda",
    "Tomonori Sawada",
    "Masayuki Nagao",
    "Masaru Setsumaru",
    "Miyoko Takaoka",
    "Masanori Hikichi",
}


class Sonic3ControlExternalTagsTest(unittest.TestCase):
    def test_join_is_hash_bound_and_manifest_complete(self) -> None:
        rows = [json.loads(line) for line in JOIN.read_text(encoding="utf-8").splitlines()]
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        expected_by_set = {
            record["corpus_id"]: record["external_tag_join"]["matching_fixture_count"]
            for record in manifest["sets"]
            if "external_tag_join" in record
        }
        actual_by_set: dict[str, int] = {}
        seen_paths: set[str] = set()
        source_package_hashes: set[str] = set()

        for row in rows:
            path = ROOT / row["fixture_path"]
            self.assertTrue(path.is_file(), row["fixture_path"])
            self.assertNotIn(row["fixture_path"], seen_paths)
            seen_paths.add(row["fixture_path"])
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), row["fixture_sha256"])
            self.assertEqual(row["authority"], AUTHORITY)
            self.assertTrue(row["helix_record_id"].startswith("FOOBAR-TAG-"))
            self.assertEqual(len(row["helix_source_package_sha256"]), 64)
            source_package_hashes.add(row["helix_source_package_sha256"])
            self.assertTrue(row["external_tag_artist"])
            self.assertTrue(set(row["target_control_person_match"]).issubset(TARGET_PEOPLE))
            self.assertIn("not composition", row["attribution_limit"])
            actual_by_set[row["corpus_id"]] = actual_by_set.get(row["corpus_id"], 0) + 1

        self.assertEqual(actual_by_set, expected_by_set)
        self.assertEqual(len(source_package_hashes), 1)


if __name__ == "__main__":
    unittest.main()
