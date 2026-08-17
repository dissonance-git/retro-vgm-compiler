import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class PrivateChildLaunchPathTest(unittest.TestCase):
    def test_component_uri_is_normalized_before_createprocess_command(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "snesapu" / "apply_private_child_launch_path.py"
        predecessor = r'''	std::string szCmdLine = "\"";
	szCmdLine += core_api::get_my_full_path();
	size_t slash = szCmdLine.find_last_of('\\');
	if (slash != std::string::npos)
		szCmdLine.erase(szCmdLine.begin() + slash + 1, szCmdLine.end());
	szCmdLine += "spcplayer.exe\"";
'''

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parent = root / "foobar2000" / "foo_snesapu"
            parent.mkdir(parents=True)
            controller = parent / "spcplayer_controller.cpp"
            controller.write_text(predecessor, encoding="utf-8")

            first = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)

            patched = controller.read_text(encoding="utf-8")
            self.assertIn("componentPath = core_api::get_my_full_path()", patched)
            self.assertIn('fileScheme = "file://"', patched)
            self.assertIn("componentPath.erase(0, fileScheme.size())", patched)
            self.assertIn('find_last_of("\\\\/")', patched)
            self.assertIn('spcplayer.exe\\\"', patched)
            self.assertLess(
                patched.index("componentPath.erase(0, fileScheme.size())"),
                patched.index("szCmdLine += componentPath"),
            )

            second = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)

    def test_launch_fix_is_in_private_component_chain(self):
        repo = Path(__file__).resolve().parents[2]
        chain = (
            repo / "patches" / "snesapu" / "apply_private_component.py"
        ).read_text(encoding="utf-8")
        self.assertIn(
            'run(here / "apply_private_child_launch_path.py", root)',
            chain,
        )


if __name__ == "__main__":
    unittest.main()
