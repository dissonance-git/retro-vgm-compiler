from __future__ import annotations

import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT / "tools" / "verify_build_source_provenance.py"
_spec = importlib.util.spec_from_file_location("build_source_provenance", TOOL_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError("could not load build source provenance verifier")
_verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_verifier)


def git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        text=True,
        capture_output=True,
        check=False,
    )


class BuildSourceProvenanceContractTest(unittest.TestCase):
    def make_repo(self, root: Path) -> str:
        subprocess.run(["git", "init", str(root)], check=True, capture_output=True)
        git(root, "config", "user.email", "test@example.invalid")
        git(root, "config", "user.name", "Retro VGM Test")
        (root / "tracked.txt").write_text("canonical\n", encoding="utf-8")
        self.assertEqual(git(root, "add", "tracked.txt").returncode, 0)
        self.assertEqual(git(root, "commit", "-m", "seed").returncode, 0)
        result = git(root, "rev-parse", "HEAD")
        self.assertEqual(result.returncode, 0)
        return result.stdout.strip().lower()

    def test_accepts_exact_committed_source_with_untracked_build_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            expected = self.make_repo(repo)
            (repo / ".private-component-build").mkdir()
            (repo / ".private-component-build" / "generated.txt").write_text(
                "ignored as provenance input\n", encoding="utf-8"
            )
            self.assertEqual(_verifier.verify_repository(repo, expected), expected)

    def test_rejects_unstaged_tracked_modification(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            self.make_repo(repo)
            (repo / "tracked.txt").write_text("modified\n", encoding="utf-8")
            with self.assertRaisesRegex(AssertionError, "unstaged tracked modifications"):
                _verifier.verify_repository(repo)

    def test_rejects_staged_modification(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            self.make_repo(repo)
            (repo / "tracked.txt").write_text("staged\n", encoding="utf-8")
            self.assertEqual(git(repo, "add", "tracked.txt").returncode, 0)
            with self.assertRaisesRegex(AssertionError, "staged modifications"):
                _verifier.verify_repository(repo)

    def test_rejects_changed_head_even_when_new_head_is_clean(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            original = self.make_repo(repo)
            (repo / "tracked.txt").write_text("second commit\n", encoding="utf-8")
            self.assertEqual(git(repo, "add", "tracked.txt").returncode, 0)
            self.assertEqual(git(repo, "commit", "-m", "second").returncode, 0)
            with self.assertRaisesRegex(AssertionError, "commit changed during build"):
                _verifier.verify_repository(repo, original)

    def test_rejects_invalid_expected_commit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            self.make_repo(repo)
            with self.assertRaisesRegex(AssertionError, "40 hexadecimal"):
                _verifier.verify_repository(repo, "not-a-commit")

    def test_rejects_non_repository(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(AssertionError, "40-hex HEAD"):
                _verifier.verify_repository(Path(tmp))


if __name__ == "__main__":
    unittest.main()
