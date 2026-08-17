import re
import unittest
from collections import Counter
from pathlib import Path


EXECUTABLE_RE = re.compile(r"add_executable\s*\(\s*([A-Za-z0-9_.+-]+)", re.MULTILINE)
TEST_RE = re.compile(r"add_test\s*\(\s*NAME\s+([A-Za-z0-9_.+-]+)", re.MULTILINE)


class CMakeRegistrationUniquenessTest(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(__file__).resolve().parents[1]
        self.fragments = {
            "host": self.root / "cmake/host_transport_tests.cmake",
            "semantic": self.root / "cmake/semantic_model_tests.cmake",
        }
        self.text = {
            name: path.read_text(encoding="utf-8")
            for name, path in self.fragments.items()
        }

    def test_each_fragment_has_unique_executable_and_ctest_names(self) -> None:
        for fragment, text in self.text.items():
            with self.subTest(fragment=fragment, kind="executable"):
                names = EXECUTABLE_RE.findall(text)
                duplicates = sorted(
                    name for name, count in Counter(names).items() if count > 1
                )
                self.assertEqual(duplicates, [])

            with self.subTest(fragment=fragment, kind="ctest"):
                names = TEST_RE.findall(text)
                duplicates = sorted(
                    name for name, count in Counter(names).items() if count > 1
                )
                self.assertEqual(duplicates, [])

    def test_host_and_semantic_fragments_do_not_compete_for_names(self) -> None:
        host_executables = set(EXECUTABLE_RE.findall(self.text["host"]))
        semantic_executables = set(EXECUTABLE_RE.findall(self.text["semantic"]))
        self.assertEqual(
            sorted(host_executables & semantic_executables),
            [],
            "a CMake target can have only one registration owner",
        )

        host_tests = set(TEST_RE.findall(self.text["host"]))
        semantic_tests = set(TEST_RE.findall(self.text["semantic"]))
        self.assertEqual(
            sorted(host_tests & semantic_tests),
            [],
            "a CTest name can have only one registration owner",
        )

    def test_studio_vgm_registry_is_semantic_owned(self) -> None:
        self.assertNotIn("studio_frame_transport_test", self.text["host"])
        self.assertIn("studio_frame_transport_test", self.text["semantic"])

        runtime_script = "tests/vgm/test_studio_hq_fm_runtime_patch.py"
        occurrences = sum(text.count(runtime_script) for text in self.text.values())
        self.assertEqual(occurrences, 1)
        self.assertIn(runtime_script, self.text["semantic"])


if __name__ == "__main__":
    unittest.main()
