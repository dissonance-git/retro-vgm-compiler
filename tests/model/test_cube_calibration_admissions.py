from __future__ import annotations

import collections
import importlib.util
import json
import pathlib
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "attribution_control_registry.py"
ADMISSIONS = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"
POLICY = ROOT / "research" / "projects" / "sonic3" / "cube-calibration-policy.json"
LOCATORS = ROOT / "tests" / "corpus" / "sonic3-attribution-control-external-tags.jsonl"

SPEC = importlib.util.spec_from_file_location("attribution_control_registry", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
registry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = registry
SPEC.loader.exec_module(registry)


def _jsonl(path: pathlib.Path) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            stripped = line.strip()
            if stripped:
                value = json.loads(stripped)
                if not isinstance(value, dict):
                    raise AssertionError(f"{path}: expected JSON object records")
                result.append(value)
    return result


class CubeCalibrationAdmissionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.policy = json.loads(POLICY.read_text(encoding="utf-8"))
        external = _jsonl(LOCATORS)
        admission_records = _jsonl(ADMISSIONS)
        cls.locators = registry.locators_from_external_tags(
            external,
            people={"Miyoko Takaoka", "Masanori Hikichi"},
        )
        cls.admissions = registry.admissions_from_records(admission_records)
        cls.control_registry = registry.build_registry(cls.locators, cls.admissions)
        cls.controls = cls.control_registry["controls"]
        cls.control_paths = {str(item["fixture_path"]) for item in cls.controls}

    def test_grounding_core_has_only_role_specific_high_confidence_controls(self) -> None:
        self.assertEqual(self.control_registry["admitted_control_count"], 15)
        counts = collections.Counter(str(item["candidate"]) for item in self.controls)
        self.assertEqual(counts["Miyoko Takaoka"], 11)
        self.assertEqual(counts["Masanori Hikichi"], 4)
        self.assertEqual(set(counts), {"Miyoko Takaoka", "Masanori Hikichi"})

        for control in self.controls:
            self.assertEqual(control["role"], "composer")
            self.assertGreaterEqual(float(control["confidence"]), 0.96)
            self.assertIn(control["status"], {"exact", "derived"})
            self.assertTrue(str(control["source"]).strip())

    def test_ancient_magic_grounding_uses_only_takaoka_direct_identifications(self) -> None:
        ancient = [
            item for item in self.controls
            if item["corpus_id"] == "ancient-magic-spc"
        ]
        self.assertEqual(len(ancient), 3)
        self.assertEqual({item["candidate"] for item in ancient}, {"Miyoko Takaoka"})
        self.assertEqual(
            {item["fixture_path"] for item in ancient},
            {
                "tests/corpus/ancient-magic-spc/02 - Prologue.spc",
                "tests/corpus/ancient-magic-spc/05 - Battle.spc",
                "tests/corpus/ancient-magic-spc/13 - Dungeon.spc",
            },
        )

        evaluation_only = {
            str(item["fixture_path"])
            for item in self.policy["evaluation_only_clues"]
        }
        self.assertEqual(
            evaluation_only,
            {
                "tests/corpus/ancient-magic-spc/01 - Title.spc",
                "tests/corpus/ancient-magic-spc/03 - Menu.spc",
                "tests/corpus/ancient-magic-spc/10 - Boss Battle.spc",
            },
        )
        self.assertTrue(evaluation_only.isdisjoint(self.control_paths))

    def test_joint_and_disputed_terranigma_cues_cannot_enter_individual_training(self) -> None:
        shared = self.policy["shared_composition_holdouts"]
        self.assertEqual(len(shared), 3)
        expected_people = {"Masanori Hikichi", "Miyoko Takaoka"}
        shared_paths: set[str] = set()
        for item in shared:
            self.assertEqual(set(item["composers"]), expected_people)
            shared_paths.add(str(item["fixture_path"]))
        self.assertEqual(
            shared_paths,
            {
                "tests/corpus/terranigma-spc/02 - Light and Darkness.spc",
                "tests/corpus/terranigma-spc/45 - Overcoming Everything.spc",
                "tests/corpus/terranigma-spc/47 - The Way Home.spc",
            },
        )
        self.assertTrue(shared_paths.isdisjoint(self.control_paths))

        disputed = self.policy["disputed_mapping_holdouts"]
        self.assertEqual(len(disputed), 1)
        disputed_path = str(disputed[0]["fixture_path"])
        self.assertEqual(
            disputed_path,
            "tests/corpus/terranigma-spc/20 - Firm Steps Upon the Land.spc",
        )
        self.assertNotIn(disputed_path, self.control_paths)

    def test_locator_metadata_cannot_override_documentary_holdouts(self) -> None:
        locator_by_path = collections.defaultdict(set)
        for locator in self.locators:
            locator_by_path[locator.fixture_path].add(locator.candidate)

        # These two local-library tags currently point only at Hikichi. The
        # commercial soundtrack credits both composers, so the policy must keep
        # them out of individual supervision despite the locator metadata.
        for path in (
            "tests/corpus/terranigma-spc/45 - Overcoming Everything.spc",
            "tests/corpus/terranigma-spc/47 - The Way Home.spc",
        ):
            self.assertEqual(locator_by_path[path], {"Masanori Hikichi"})
            self.assertNotIn(path, self.control_paths)

    def test_third_party_terranigma_decoy_stays_outside_cube_classes(self) -> None:
        decoys = self.policy["third_party_decoys"]
        self.assertEqual(len(decoys), 1)
        self.assertEqual(decoys[0]["candidate"], "Yuzo Koshiro")
        self.assertEqual(
            decoys[0]["fixture_path"],
            "tests/corpus/terranigma-spc/42 - Genius's Playground.spc",
        )
        self.assertNotIn(decoys[0]["fixture_path"], self.control_paths)

    def test_both_mixed_composer_corpora_are_primary_calibration_worlds(self) -> None:
        self.assertEqual(
            set(self.policy["primary_mixed_composer_corpora"]),
            {"ancient-magic-spc", "terranigma-spc"},
        )
        grounded_corpora = {str(item["corpus_id"]) for item in self.controls}
        self.assertEqual(grounded_corpora, {"ancient-magic-spc", "terranigma-spc"})


if __name__ == "__main__":
    unittest.main()
