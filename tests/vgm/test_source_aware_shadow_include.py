import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class SourceAwareShadowIncludePatchTest(unittest.TestCase):
    def test_includes_private_player_only_under_source_capture_abi(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "foo_input_vgm" / "apply_source_aware_shadow_include.py"

        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            shadow = source / "input_vgm_shadow.cpp"
            shadow.write_text(
                '#include "input_vgm.h"\n\n#include <emu/cores/sn764intf.h>\n',
                encoding="utf-8",
            )

            first = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)
            patched = shadow.read_text(encoding="utf-8")
            self.assertIn(
                '#ifdef LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI\n'
                '#include "source_aware_vgm_player.h"\n'
                '#endif',
                patched,
            )
            self.assertLess(
                patched.index('#include "source_aware_vgm_player.h"'),
                patched.index('#include <emu/cores/sn764intf.h>'),
            )

            # Guarded patches are intentionally singular. A second application
            # must fail rather than silently duplicating the private type include.
            second = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)


if __name__ == "__main__":
    unittest.main()
