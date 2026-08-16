import pathlib
import subprocess
import tempfile
import unittest


class ComposerGrammarEvidenceCompileTest(unittest.TestCase):
    def test_cross_soundtrack_composer_grammar_guardrails_compile_and_run(self):
        repo_root = pathlib.Path(__file__).resolve().parents[2]
        source = repo_root / "tests" / "model" / "composer_grammar_evidence_test.cpp"

        with tempfile.TemporaryDirectory() as temp_dir:
            executable = pathlib.Path(temp_dir) / "composer_grammar_evidence_test"
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
