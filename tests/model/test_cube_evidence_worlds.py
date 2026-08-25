from __future__ import annotations

import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
POLICY = ROOT / "research" / "projects" / "sonic3" / "cube-calibration-policy.json"
ADMISSIONS = ROOT / "research" / "projects" / "sonic3" / "attribution-control-admissions.jsonl"


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


def _corpus_from_fixture(path: str) -> str:
    parts = pathlib.PurePosixPath(path).parts
    if len(parts) < 4 or parts[:2] != ("tests", "corpus"):
        raise AssertionError(f"unexpected fixture path {path!r}")
    return parts[2]


class CubeEvidenceWorldsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.policy = json.loads(POLICY.read_text(encoding="utf-8"))
        cls.admissions = _jsonl(ADMISSIONS)
        cls.admitted_paths = {str(item["fixture_path"]) for item in cls.admissions}
        cls.admitted_corpora = {
            _corpus_from_fixture(str(item["fixture_path"])) for item in cls.admissions
        }
        cls.worlds = cls.policy["evidence_worlds"]

    def test_policy_uses_typed_evidence_worlds(self) -> None:
        self.assertEqual(self.policy["schema_version"], 4)
        self.assertNotIn("outside_validation_corpora", self.policy)
        self.assertEqual(
            set(self.worlds),
            {
                "work_level_single_composer_validation",
                "prospective_exact_control_worlds",
                "derivative_inheritance_candidates",
                "team_or_partial_participation",
                "arrangement_or_implementation_only",
                "future_acquisition_or_verification",
            },
        )

    def test_team_or_partial_sets_cannot_become_blanket_composer_grounding(self) -> None:
        team_sets = {
            str(item["corpus_id"])
            for item in self.worlds["team_or_partial_participation"]
        }
        self.assertEqual(
            team_sets,
            {
                "america-oudan-ultra-quiz-spc",
                "super-black-bass-spc",
                "battle-master-spc",
                "ecology-magic-ym2608",
                "kakinoki-shougi-spc",
            },
        )
        self.assertTrue(team_sets.isdisjoint(self.admitted_corpora))

    def test_ghox_is_work_level_validation_not_fixture_grounding(self) -> None:
        work_level = self.worlds["work_level_single_composer_validation"]
        self.assertEqual(len(work_level), 1)
        ghox = work_level[0]
        self.assertEqual(ghox["corpus_id"], "ghox-ym2151")
        self.assertEqual(ghox["candidate"], "Miyoko Takaoka")
        self.assertEqual(ghox["device_family"], "YM2151")
        self.assertNotIn("ghox-ym2151", self.admitted_corpora)

    def test_wizardry_first_three_are_inheritance_hypotheses_not_tag_promotions(self) -> None:
        candidates = self.worlds["derivative_inheritance_candidates"]
        self.assertEqual(len(candidates), 1)
        wizardry = candidates[0]
        self.assertEqual(wizardry["corpus_id"], "wizardry-iii-iv-huc6280")
        self.assertEqual(wizardry["candidate"], "Miyoko Takaoka")
        self.assertEqual(wizardry["precursor_work"], "Wizardry I・II")
        paths = set(wizardry["fixture_paths"])
        self.assertEqual(
            paths,
            {
                "tests/corpus/wizardry-iii-iv-huc6280/01 - Adventurer's Inn.vgz",
                "tests/corpus/wizardry-iii-iv-huc6280/02 - Boltac's Trading Post.vgz",
                "tests/corpus/wizardry-iii-iv-huc6280/03 - Temple of Cant.vgz",
            },
        )
        self.assertTrue(paths.isdisjoint(self.admitted_paths))

    def test_implementation_worlds_do_not_leak_into_composer_grounding(self) -> None:
        implementation = self.worlds["arrangement_or_implementation_only"]
        committed = {
            str(item["corpus_id"])
            for item in implementation
            if "corpus_id" in item
        }
        self.assertIn("dr-robotniks-mean-bean-machine-vgz", committed)
        self.assertTrue(committed.isdisjoint(self.admitted_corpora))
        named_works = {str(item.get("work", "")) for item in implementation}
        self.assertIn("E.V.O.: Search for Eden", named_works)
        self.assertIn("Ys IV: Mask of the Sun", named_works)

    def test_future_hikichi_worlds_are_candidates_not_current_grounding(self) -> None:
        future = self.worlds["future_acquisition_or_verification"]
        by_candidate: dict[str, set[str]] = {}
        for item in future:
            by_candidate.setdefault(str(item["candidate"]), set()).add(str(item["work"]))
        self.assertIn("Wizardry I・II", by_candidate["Miyoko Takaoka"])
        self.assertTrue(
            {
                "Human Sports Festival",
                "Power of the Hired",
            }.issubset(by_candidate["Masanori Hikichi"])
        )
        self.assertNotIn("Gleylancer", by_candidate["Masanori Hikichi"])
        self.assertNotIn("Galaxy Force II (Mega Drive)", by_candidate["Masanori Hikichi"])

        prospective = self.worlds["prospective_exact_control_worlds"]
        self.assertTrue(any(
            item.get("work") == "Gleylancer"
            and item.get("candidate") == "Masanori Hikichi"
            for item in prospective
        ))
        implementation = self.worlds["arrangement_or_implementation_only"]
        self.assertTrue(any(
            item.get("work") == "Galaxy Force II (Mega Drive)"
            and item.get("candidate") == "Masanori Hikichi"
            for item in implementation
        ))


if __name__ == "__main__":
    unittest.main()
