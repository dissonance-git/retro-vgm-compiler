from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FORBIDDEN_PROPER_NOUN = re.compile(r"\b(?:Enhanced|ENHANCED)\b")
TEXT_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".md", ".py", ".txt"}

# These are project-owned surfaces that define and expose source-native
# enhancement. Code identifiers such as cfg_vgm_enhanced_enabled and
# FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI are intentionally legal: underscores
# keep the uppercase fragment from matching the standalone-word rule.
#
# SPC is listed file-by-file because some historical migration patchers retain
# exact old source strings as guarded anchors. Those immutable anchor literals
# are not current naming/UI and must not be rewritten merely to satisfy prose
# style; only the maintained active surfaces belong in this wording contract.
SCAN_ROOTS = (
    ROOT / "patches" / "foo_input_vgm",
    ROOT / "docs" / "source-native-enhanced-rendering.md",
    ROOT / "research" / "enhancement",
    ROOT / "components" / "vgm" / "enhancement" / "genesis_enhanced_recomposition.h",
    ROOT / "patches" / "snesapu" / "apply_enhanced_component.py",
    ROOT / "patches" / "snesapu" / "apply_enhanced_ui.py",
    ROOT / "patches" / "snesapu" / "apply_enhanced_runtime.py",
    ROOT / "patches" / "snesapu" / "apply_current_parent_source_transport.py",
    ROOT / "patches" / "snesapu" / "apply_current_child_source_transport.py",
    ROOT / "patches" / "snesapu" / "apply_private_component.py",
)


def iter_text_files(root: Path):
    if root.is_file():
        yield root
        return
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES:
            yield path


class EnhancedWordingTest(unittest.TestCase):
    def test_enhanced_is_descriptive_not_a_proper_name(self) -> None:
        violations: list[str] = []
        for scan_root in SCAN_ROOTS:
            for path in iter_text_files(scan_root):
                text = path.read_text(encoding="utf-8", errors="replace")
                for line_number, line in enumerate(text.splitlines(), start=1):
                    if FORBIDDEN_PROPER_NOUN.search(line):
                        violations.append(
                            f"{path.relative_to(ROOT)}:{line_number}: {line.strip()}"
                        )
        self.assertEqual(
            violations,
            [],
            "enhanced is a descriptive adjective; use lowercase in prose/UI:\n"
            + "\n".join(violations),
        )


if __name__ == "__main__":
    unittest.main()
