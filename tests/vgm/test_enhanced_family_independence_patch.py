from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "foo_input_vgm" / "apply_enhanced_family_independence.py"


class EnhancedFamilyIndependencePatchTest(unittest.TestCase):
    def test_global_block_complete_veto_is_removed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shadow = root / "input_vgm_shadow.cpp"
            shadow.write_text(
                "void fixture()\n"
                "{\n"
                "\tauto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);\n"
                "\tif (source_player == nullptr || !source_player->source_topology_supported()\n"
                "\t\t|| !source_player->source_block_complete()\n"
                "\t\t|| source_player->source_output_count() != sample_count)\n"
                "\t\treturn;\n"
                "}\n",
                encoding="utf-8",
            )

            subprocess.run([sys.executable, str(PATCH), str(root)], check=True, cwd=ROOT)
            text = shadow.read_text(encoding="utf-8")

            self.assertIn("source_topology_supported()", text)
            self.assertIn("source_output_count() != sample_count", text)
            self.assertNotIn("source_block_complete()", text)


if __name__ == "__main__":
    unittest.main()
