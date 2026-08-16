import pathlib
import subprocess
import tempfile
import unittest


class GenesisPartEvidenceCompileTest(unittest.TestCase):
    def test_part_evidence_compiles_and_respects_identity_guardrails(self):
        repo_root = pathlib.Path(__file__).resolve().parents[2]
        source = repo_root / "tests" / "vgm" / "genesis_part_evidence_test.cpp"

        with tempfile.TemporaryDirectory() as temp_dir:
            executable = pathlib.Path(temp_dir) / "genesis_part_evidence_test"
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
                    "components/vgm/enhancement/genesis_state.cpp",
                    "-o",
                    str(executable),
                ],
                cwd=repo_root,
                check=True,
            )
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
