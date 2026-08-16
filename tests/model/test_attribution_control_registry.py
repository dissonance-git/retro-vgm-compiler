from __future__ import annotations

import importlib.util
import pathlib
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "attribution_control_registry.py"
SPEC = importlib.util.spec_from_file_location("attribution_control_registry", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
registry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = registry
SPEC.loader.exec_module(registry)


class AttributionControlRegistryTest(unittest.TestCase):
    def setUp(self) -> None:
        self.external = [
            {
                "candidate_dummy": "unused",
                "attribution_limit": (
                    "curated artist metadata only; not composition, arrangement, "
                    "programming, or authorship proof"
                ),
                "authority": "external-tags-test",
                "corpus_id": "known-game",
                "external_tag_artist": "Candidate A",
                "external_tag_platform": "Mega Drive",
                "fixture_path": "tests/corpus/known-game/01 Track.vgz",
                "target_control_person_match": ["Candidate A"],
            }
        ]

    def test_metadata_remains_locator_only_until_admitted(self) -> None:
        locators = registry.locators_from_external_tags(self.external)
        result = registry.build_registry(locators, [])
        self.assertEqual(result["locator_count"], 1)
        self.assertEqual(result["admitted_control_count"], 0)
        self.assertEqual(result["locator_only_count"], 1)

        admissions = registry.admissions_from_records(
            [
                {
                    "candidate": "Candidate A",
                    "fixture_path": "tests/corpus/known-game/01 Track.vgz",
                    "role": "composer",
                    "status": "exact",
                    "confidence": 0.97,
                    "source": "direct exact-track composer credit",
                    "work_family_id": "track-family",
                    "implementation_family_id": "known-driver",
                }
            ]
        )
        admitted = registry.build_registry(locators, admissions)
        self.assertEqual(admitted["admitted_control_count"], 1)
        self.assertEqual(admitted["locator_only_count"], 0)
        self.assertEqual(admitted["controls"][0]["role"], "composer")
        self.assertEqual(admitted["controls"][0]["status"], "exact")

    def test_unbacked_admission_is_rejected(self) -> None:
        locators = registry.locators_from_external_tags(self.external)
        admissions = registry.admissions_from_records(
            [
                {
                    "candidate": "Candidate A",
                    "fixture_path": "tests/corpus/other-game/99 Missing.vgz",
                    "role": "composer",
                    "status": "exact",
                    "confidence": 0.95,
                    "source": "documentary-source",
                }
            ]
        )
        with self.assertRaises(ValueError):
            registry.build_registry(locators, admissions)

    def test_invalid_role_and_confidence_are_rejected(self) -> None:
        with self.assertRaises(ValueError):
            registry.admissions_from_records(
                [
                    {
                        "candidate": "Candidate A",
                        "fixture_path": "tests/corpus/known-game/01 Track.vgz",
                        "role": "artist",
                        "status": "exact",
                        "confidence": 0.9,
                        "source": "bad-role-test",
                    }
                ]
            )

        with self.assertRaises(ValueError):
            registry.admissions_from_records(
                [
                    {
                        "candidate": "Candidate A",
                        "fixture_path": "tests/corpus/known-game/01 Track.vgz",
                        "role": "composer",
                        "status": "exact",
                        "confidence": 1.1,
                        "source": "bad-confidence-test",
                    }
                ]
            )


if __name__ == "__main__":
    unittest.main()
