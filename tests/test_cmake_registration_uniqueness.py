import re
import unittest
from collections import Counter
from pathlib import Path


EXECUTABLE_RE = re.compile(r"add_executable\s*\(\s*([A-Za-z0-9_.+-]+)", re.MULTILINE)
TEST_RE = re.compile(r"add_test\s*\(\s*NAME\s+([A-Za-z0-9_.+-]+)", re.MULTILINE)


class CMakeRegistrationUniquenessTest(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(__file__).resolve().parents[1]
        self.host_path = self.root / "cmake/host_transport_tests.cmake"
        self.semantic_entry_path = self.root / "cmake/semantic_model_tests.cmake"
        self.semantic_core_path = self.root / "cmake/semantic_model_tests_core.cmake"
        self.host = self.host_path.read_text(encoding="utf-8")
        self.semantic_entry = self.semantic_entry_path.read_text(encoding="utf-8")
        self.semantic_core = self.semantic_core_path.read_text(encoding="utf-8")
        self.semantic = self.semantic_core + "\n" + self.semantic_entry

    def assert_unique_names(self, text: str, regex: re.Pattern[str], label: str) -> None:
        names = regex.findall(text)
        duplicates = sorted(name for name, count in Counter(names).items() if count > 1)
        self.assertEqual(duplicates, [], label)

    def test_semantic_entry_includes_core_exactly_once(self) -> None:
        self.assertEqual(self.semantic_entry.count("semantic_model_tests_core.cmake"), 1)

    def test_each_owner_has_unique_executable_and_ctest_names(self) -> None:
        for owner, text in (("host", self.host), ("semantic", self.semantic)):
            with self.subTest(owner=owner, kind="executable"):
                self.assert_unique_names(text, EXECUTABLE_RE, f"duplicate executable in {owner}")
            with self.subTest(owner=owner, kind="ctest"):
                self.assert_unique_names(text, TEST_RE, f"duplicate CTest name in {owner}")

    def test_host_and_semantic_owners_do_not_compete_for_names(self) -> None:
        host_executables = set(EXECUTABLE_RE.findall(self.host))
        semantic_executables = set(EXECUTABLE_RE.findall(self.semantic))
        self.assertEqual(
            sorted(host_executables & semantic_executables),
            [],
            "a CMake target can have only one registration owner",
        )

        host_tests = set(TEST_RE.findall(self.host))
        semantic_tests = set(TEST_RE.findall(self.semantic))
        self.assertEqual(
            sorted(host_tests & semantic_tests),
            [],
            "a CTest name can have only one registration owner",
        )

    def test_studio_vgm_registry_is_semantic_owned(self) -> None:
        self.assertNotIn("studio_frame_transport_test", self.host)
        self.assertIn("studio_frame_transport_test", self.semantic)

        runtime_script = "tests/vgm/test_studio_hq_fm_runtime_patch.py"
        occurrences = self.host.count(runtime_script) + self.semantic.count(runtime_script)
        self.assertEqual(occurrences, 1)
        self.assertIn(runtime_script, self.semantic)

    def test_phrase_arbitration_is_registered_in_semantic_owner(self) -> None:
        self.assertIn("ionian_cadence_phrase_arbitration_test", self.semantic)
        self.assertIn("NAME ionian_cadence_phrase_arbitration", self.semantic)


if __name__ == "__main__":
    unittest.main()
