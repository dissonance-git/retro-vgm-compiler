import re
import unittest
from collections import Counter
from pathlib import Path


PYTHON_REGISTRATION_RE = re.compile(
    r"vgm_compiler_add_python_(?:unittest|test)\(\s*"
    r"([A-Za-z0-9_.+-]+)\s+([A-Za-z0-9_.+-]+)\s+([A-Za-z0-9_.+-]+)\s*\)"
)


class CMakeRegistrationUniquenessTest(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(__file__).resolve().parents[1]
        self.project_cmake = (self.root / "CMakeLists.txt").read_text(encoding="utf-8")
        self.test_cmake = (self.root / "tests/CMakeLists.txt").read_text(encoding="utf-8")

    def test_root_build_has_one_current_project_identity(self) -> None:
        for marker in (
            "project(vgm_compiler LANGUAGES CXX)",
            "VGM_COMPILER_BUILD_TESTS",
            "add_library(vgm_compiler_core STATIC",
            "add_subdirectory(tests)",
        ):
            self.assertIn(marker, self.project_cmake)

        for retired in (
            "foobar2000_game_audio_core",
            "GAMEAUDIO_BUILD_CORE_TESTS",
            "gameaudio_vgm_core",
            "GAMEAUDIO_TEST_TARGETS",
            "semantic_model_tests",
            "host_transport_tests",
        ):
            self.assertNotIn(retired, self.project_cmake)

    def test_superseded_registry_files_are_gone(self) -> None:
        for relative in (
            "cmake/semantic_model_tests.cmake",
            "cmake/semantic_model_tests_core.cmake",
            "cmake/host_transport_tests.cmake",
        ):
            self.assertFalse((self.root / relative).exists(), relative)

    def test_cpp_registration_is_derived_from_test_ownership(self) -> None:
        self.assertIn("file(GLOB VGM_COMPILER_CPP_TEST_SOURCES CONFIGURE_DEPENDS", self.test_cmake)
        self.assertIn('"${CMAKE_CURRENT_SOURCE_DIR}/model/*_test.cpp"', self.test_cmake)
        self.assertIn('"${CMAKE_CURRENT_SOURCE_DIR}/vgm/*_test.cpp"', self.test_cmake)
        self.assertIn('"${CMAKE_CURRENT_SOURCE_DIR}/spc/*_test.cpp"', self.test_cmake)

        sources = sorted(
            path
            for directory in ("model", "vgm", "spc")
            for path in (self.root / "tests" / directory).glob("*_test.cpp")
        )
        self.assertTrue(sources, "expected dependency-free C++ tests")

        counts = Counter(path.stem for path in sources)
        duplicates = sorted(name for name, count in counts.items() if count > 1)
        self.assertEqual(
            duplicates,
            [],
            "derived CMake target names must be unique across test owners",
        )

    def test_python_contracts_have_one_registration_owner(self) -> None:
        registrations = PYTHON_REGISTRATION_RE.findall(self.test_cmake)
        self.assertTrue(registrations, "expected curated Python CTest contracts")

        names = [name for name, _directory, _script in registrations]
        scripts = [f"{directory}/{script}" for _name, directory, script in registrations]

        duplicate_names = sorted(name for name, count in Counter(names).items() if count > 1)
        duplicate_scripts = sorted(path for path, count in Counter(scripts).items() if count > 1)
        self.assertEqual(duplicate_names, [], "a CTest name can have only one owner")
        self.assertEqual(duplicate_scripts, [], "a Python test script can have only one CTest owner")

        self.assertEqual(scripts.count("vgm/test_studio_hq_fm_runtime_patch.py"), 1)

    def test_python_registrations_point_to_current_files(self) -> None:
        for _name, directory, script in PYTHON_REGISTRATION_RE.findall(self.test_cmake):
            path = self.root / "tests" / directory / script
            with self.subTest(path=path.relative_to(self.root)):
                self.assertTrue(path.is_file())


if __name__ == "__main__":
    unittest.main()
