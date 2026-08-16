import pathlib
import subprocess
import tempfile
import unittest


class CreativeAttributionHypothesisCompileTest(unittest.TestCase):
    def test_role_scoped_attribution_guardrails_compile_and_run(self):
        repo_root = pathlib.Path(__file__).resolve().parents[2]
        source = repo_root / "tests" / "model" / "creative_attribution_hypothesis_test.cpp"

        with tempfile.TemporaryDirectory() as temp_dir:
            executable = pathlib.Path(temp_dir) / "creative_attribution_hypothesis_test"
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
                cwd=repo_root,
                check=True,
            )
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
