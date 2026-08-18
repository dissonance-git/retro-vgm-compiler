from __future__ import annotations

import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCHES = ROOT / "patches" / "snesapu"


class SpcSurroundOmniphonyPatchContractTest(unittest.TestCase):
    def read(self, path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_patch_scripts_remain_valid_python(self) -> None:
        for name in (
            "apply_surround_ui_bridge.py",
            "apply_surround_omniphony_private_bridge.py",
            "apply_enhanced_component.py",
            "apply_private_component.py",
        ):
            ast.parse(self.read(PATCHES / name), filename=name)

    def test_historical_surround_bit_is_restored_as_the_ui_owner(self) -> None:
        ui = self.read(PATCHES / "apply_surround_ui_bridge.py")
        self.assertIn('CONTROL         "Surround",IDC_DSP_SURROUND', ui)
        self.assertIn("IDC_DSP_SURROUND", ui)
        self.assertIn("DSP_SURND", ui)
        self.assertIn("MAX_DSP_OPT 13", ui)
        self.assertNotIn('CONTROL         "Enable \\"surround\\" sound"', ui)

    def test_duplicate_sem71_preference_is_removed_not_migrated(self) -> None:
        ui = self.read(PATCHES / "apply_surround_ui_bridge.py")
        self.assertIn("remove SNES duplicate spatial preference GUID", ui)
        self.assertIn("remove SNES duplicate spatial cfg var", ui)
        self.assertIn("cfg_enhanced_enabled", ui)

    def test_old_snesapu_surround_algorithm_is_masked_from_renderer(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_private_bridge.py")
        self.assertIn("m_CnfOptions & ~DSP_SURND", bridge)
        self.assertIn("snesapu_runtime_options", bridge)
        self.assertIn("disable legacy SNESAPU surround processing", bridge)

    def test_same_saved_surround_bit_gates_source_capture_and_omniphony(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_private_bridge.py")
        self.assertIn("(m_CnfOptions & DSP_SURND) != 0", bridge)
        self.assertIn("m_Apu.SetSourceEnabled(m_Sem71Enabled);", bridge)

    def test_private_patch_stack_installs_bridge_after_omniphony_runtime(self) -> None:
        master = self.read(PATCHES / "apply_private_component.py")
        runtime = 'run(here / "apply_spatial_omniphony_private_runtime.py", root)'
        bridge = 'run(here / "apply_surround_omniphony_private_bridge.py", root)'
        self.assertIn(runtime, master)
        self.assertIn(bridge, master)
        self.assertLess(master.index(runtime), master.index(bridge))

    def test_enhanced_stays_a_separate_source_quality_option(self) -> None:
        master = self.read(PATCHES / "apply_enhanced_component.py")
        ui = self.read(PATCHES / "apply_surround_ui_bridge.py")
        self.assertIn('run(here / "apply_surround_ui_bridge.py", source)', master)
        self.assertIn('CONTROL         "enhanced",IDC_ENHANCED_ENABLED', ui)
        self.assertIn("cfg_enhanced_enabled", ui)


if __name__ == "__main__":
    unittest.main()
