from __future__ import annotations

import collections
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import build_admitted_composer_caches as all_caches


class AdmittedComposerCacheTests(unittest.TestCase):
    def test_admitted_creators_filters_roles_and_conflicts(self):
        records = [
            {"creator": "B", "role": "composer", "status": "exact"},
            {"creator": "A", "role": "composer", "status": "derived"},
            {"creator": "C", "role": "composer", "status": "conflict"},
            {"creator": "D", "role": "arranger_programmer", "status": "exact"},
        ]
        self.assertEqual(all_caches.admitted_creators(records), ["A", "B"])

    def test_admission_projection_normalizes_candidate_without_rewriting_truth(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            admissions = pathlib.Path(temp_dir) / "admissions.jsonl"
            raw = {
                "candidate": "Cube Composer",
                "fixture_path": "tests/corpus/example-spc/01 - Cue.spc",
                "role": "composer",
                "status": "exact",
                "confidence": 0.99,
            }
            admissions.write_text(json.dumps(raw) + "\n", encoding="utf-8")
            projected = all_caches.normalize_admission_records(
                all_caches.read_jsonl(admissions), admissions_path=admissions
            )

        self.assertEqual(len(projected), 1)
        self.assertEqual(projected[0]["creator"], "Cube Composer")
        self.assertEqual(projected[0]["candidate"], "Cube Composer")
        self.assertEqual(projected[0]["corpus_id"], "example-spc")
        self.assertEqual(
            projected[0]["control_source"], "canonical_attribution_admission"
        )
        self.assertEqual(
            raw,
            {
                "candidate": "Cube Composer",
                "fixture_path": "tests/corpus/example-spc/01 - Cue.spc",
                "role": "composer",
                "status": "exact",
                "confidence": 0.99,
            },
        )

    def test_build_all_reports_spc_as_backend_unavailable_when_extractor_missing(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = pathlib.Path(temp_dir)
            credits = temp / "credits.jsonl"
            records = [
                {"fixture_path": "a.vgm", "creator": "A", "role": "composer", "status": "exact"},
                {"fixture_path": "b.spc", "creator": "A", "role": "composer", "status": "exact"},
                {"fixture_path": "ignored.vgm", "creator": "C", "role": "composer", "status": "conflict"},
            ]
            credits.write_text(
                "\n".join(json.dumps(record) for record in records) + "\n",
                encoding="utf-8",
            )

            admissions = temp / "admissions.jsonl"
            admissions.write_text(
                json.dumps(
                    {
                        "fixture_path": "c.spc",
                        "candidate": "B",
                        "role": "composer",
                        "status": "exact",
                    }
                )
                + "\n",
                encoding="utf-8",
            )

            calls = []

            def fake_build_creator(**kwargs):
                filtered = all_caches.read_jsonl(kwargs["credit_index"])
                calls.append(
                    (kwargs["creator"], [record["fixture_path"] for record in filtered])
                )
                return {
                    "creator": kwargs["creator"],
                    "role": kwargs["role"],
                    "selected_tracks": len(filtered),
                    "built": len(filtered),
                    "reused": 0,
                }

            with mock.patch.object(all_caches, "build_creator", fake_build_creator):
                result = all_caches.build_all(
                    credit_index=credits,
                    admissions_path=admissions,
                    role="composer",
                    repo_root=temp,
                    cache_root=temp / "cache",
                    cache_index=temp / "cache" / "index.jsonl",
                    refresh=False,
                    admitted_statuses={"exact"},
                )

        self.assertEqual(calls, [("A", ["a.vgm"])])
        self.assertEqual(result["creator_count"], 2)
        self.assertEqual(result["role_selected_tracks"], 3)
        self.assertEqual(result["cacheable_tracks"], 1)
        self.assertEqual(result["backend_unavailable_tracks"], 2)
        self.assertEqual(result["unsupported_tracks"], 0)
        self.assertEqual(result["selected_tracks"], 1)
        self.assertEqual(result["built"], 1)
        self.assertEqual(result["reused"], 0)

        by_creator = {item["creator"]: item for item in result["results"]}
        self.assertEqual(by_creator["A"]["role_selected_tracks"], 2)
        self.assertEqual(by_creator["A"]["cacheable_tracks"], 1)
        self.assertEqual(by_creator["A"]["backend_unavailable_tracks"], 1)
        self.assertEqual(by_creator["A"]["unsupported_fixtures"], ["b.spc"])
        self.assertEqual(by_creator["B"]["role_selected_tracks"], 1)
        self.assertEqual(by_creator["B"]["cacheable_tracks"], 0)
        self.assertEqual(by_creator["B"]["backend_unavailable_tracks"], 1)
        self.assertEqual(by_creator["B"]["unsupported_fixtures"], ["c.spc"])

    def test_build_all_routes_spc_when_forensic_backend_is_supplied(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = pathlib.Path(temp_dir)
            credits = temp / "credits.jsonl"
            credits.write_text("", encoding="utf-8")
            admissions = temp / "admissions.jsonl"
            admissions.write_text(
                json.dumps(
                    {
                        "fixture_path": "tests/corpus/world-spc/01 - Cue.spc",
                        "candidate": "Cube Composer",
                        "role": "composer",
                        "status": "exact",
                    }
                )
                + "\n",
                encoding="utf-8",
            )
            extractor = temp / "spc_forensic_features"
            extractor.write_bytes(b"placeholder")
            calls = []

            def fake_spc_build_one(source, **kwargs):
                calls.append((source, kwargs))
                return temp / "spc-cache" / "cue.json", True

            with mock.patch.object(
                all_caches.spc_cache, "build_one", side_effect=fake_spc_build_one
            ):
                result = all_caches.build_all(
                    credit_index=credits,
                    admissions_path=admissions,
                    role="composer",
                    repo_root=temp,
                    cache_root=temp / "vgm-cache",
                    cache_index=temp / "vgm-cache" / "index.jsonl",
                    refresh=False,
                    admitted_statuses={"exact"},
                    spc_extractor=extractor,
                    spc_cache_root=temp / "spc-cache",
                    spc_seconds=5,
                )

        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0][0], temp / "tests/corpus/world-spc/01 - Cue.spc")
        self.assertEqual(calls[0][1]["corpus_id"], "world-spc")
        self.assertEqual(calls[0][1]["seconds"], 5)
        self.assertTrue(result["spc_backend_ready"])
        self.assertEqual(result["role_selected_tracks"], 1)
        self.assertEqual(result["cacheable_tracks"], 1)
        self.assertEqual(result["backend_unavailable_tracks"], 0)
        self.assertEqual(result["unsupported_tracks"], 0)
        self.assertEqual(result["built"], 1)

    def test_genesis_routing_index_remains_small_and_format_specific(self):
        records = all_caches.read_jsonl(ROOT / all_caches.DEFAULT_CREDITS)
        admitted = all_caches.admitted_records(records)
        counts = collections.Counter(record["creator"] for record in admitted)
        self.assertEqual(
            counts,
            {
                "Tatsuyuki Maeda": 26,
                "Jun Senoue": 12,
                "Haruyo Oguro": 5,
                "Naofumi Hataya": 4,
                "Tomonori Sawada": 2,
                "Masaru Setsumaru": 1,
                "Seirou Okamoto": 1,
            },
        )
        self.assertEqual(sum(counts.values()), 51)
        self.assertTrue(
            all(all_caches.cacheable_by_current_backend(record) for record in admitted)
        )
        self.assertFalse(
            any(
                record.get("creator") in {"Miyoko Takaoka", "Masanori Hikichi"}
                for record in admitted
            )
        )

        scorching = [
            record
            for record in admitted
            if "Scorching Sand" in str(record.get("fixture_path", ""))
        ]
        self.assertEqual(len(scorching), 1)
        self.assertEqual(scorching[0]["creator"], "Tomonori Sawada")
        self.assertEqual(scorching[0]["status"], "derived")
        self.assertIn("counterevidence", scorching[0])

    def test_canonical_admissions_extend_runtime_catalog_without_duplication(self):
        records = all_caches.combined_control_records(
            credit_index=ROOT / all_caches.DEFAULT_CREDITS,
            admissions_path=ROOT / all_caches.DEFAULT_ADMISSIONS,
        )
        admitted = all_caches.admitted_records(records)
        counts = collections.Counter(record["creator"] for record in admitted)
        self.assertEqual(
            counts,
            {
                "Tatsuyuki Maeda": 26,
                "Jun Senoue": 12,
                "Haruyo Oguro": 5,
                "Naofumi Hataya": 4,
                "Tomonori Sawada": 2,
                "Masaru Setsumaru": 1,
                "Seirou Okamoto": 1,
                "Miyoko Takaoka": 11,
                "Masanori Hikichi": 4,
            },
        )
        self.assertEqual(sum(counts.values()), 66)

        genesis = [
            record for record in admitted if all_caches.cacheable_by_current_backend(record)
        ]
        spc = [record for record in admitted if all_caches._suffix(record) in all_caches.SPC_SUFFIXES]
        self.assertEqual(len(genesis), 51)
        self.assertEqual(len(spc), 15)

        cube = [
            record
            for record in admitted
            if record.get("creator") in {"Miyoko Takaoka", "Masanori Hikichi"}
        ]
        self.assertEqual(len(cube), 15)
        self.assertEqual(
            {record["corpus_id"] for record in cube},
            {"ancient-magic-spc", "terranigma-spc"},
        )
        self.assertTrue(
            all(
                record.get("control_source") == "canonical_attribution_admission"
                for record in cube
            )
        )
        self.assertFalse(
            any("sonic-3-and-knuckles" in record["fixture_path"] for record in cube)
        )

        quarantined = {
            "tests/corpus/terranigma-spc/02 - Light and Darkness.spc",
            "tests/corpus/terranigma-spc/20 - Firm Steps Upon the Land.spc",
            "tests/corpus/terranigma-spc/45 - Overcoming Everything.spc",
            "tests/corpus/terranigma-spc/47 - The Way Home.spc",
        }
        self.assertTrue(
            quarantined.isdisjoint({record["fixture_path"] for record in cube})
        )


if __name__ == "__main__":
    unittest.main()
