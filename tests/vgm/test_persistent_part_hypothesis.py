import pathlib
import subprocess
import tempfile
import unittest


class PersistentPartHypothesisCompileTest(unittest.TestCase):
    def test_model_compiles_and_enforces_identity_boundaries(self):
        repo_root = pathlib.Path(__file__).resolve().parents[2]
        source = repo_root / "tests" / "model" / "persistent_part_hypothesis_test.cpp"

        with tempfile.TemporaryDirectory() as temp_dir:
            executable = pathlib.Path(temp_dir) / "persistent_part_hypothesis_test"
            subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Wpedantic",
                    "-Werror",
                    f"-I{repo_root}",
                    str(source),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
