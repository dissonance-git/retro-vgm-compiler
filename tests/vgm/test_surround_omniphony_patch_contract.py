from __future__ import annotations

import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCHES = ROOT / "patches" / "foo_input_vgm"


class VgmSurroundOmniphonyPatchContractTest(unittest.TestCase):
    def read(self, path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_patch_scripts_remain_valid_python(self) -> None:
        for name in (
            "apply_enhanced_ui.py",
            "apply_surround_omniphony_bridge.py",
            "apply_enhanced_component.py",
        ):
            ast.parse(self.read(PATCHES / name), filename=name)

    def test_existing_surround_preference_owns_spatial_presentation(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_bridge.py")
        self.assertIn("cfg_surround_sound", bridge)
        self.assertIn("pa_cfg.chnInvert = 0x00;", bridge)
        self.assertIn("!cfg_surround_sound || !sources_ready", bridge)

    def test_old_libvgm_surround_is_not_stacked_with_omniphony(self) -> None:
        bridge = self.read(PATCHES / "apply_surround_omniphony_bridge.py")
        self.assertIn("disable legacy VGM surround inversion", bridge)
        self.assertNotIn("pa_cfg.chnInvert = cfg_surround_sound ? 0x02 : 0x00;\n\tpa_cfg.chnInvert", bridge)

    def test_master_patch_orders_source_selection_before_surround_bridge(self) -> None:
        master = self.read(PATCHES / "apply_enhanced_component.py")
        runtime = 'run(here / "apply_spatial_omniphony_runtime.py", source)'
        bridge = 'run(here / "apply_surround_omniphony_bridge.py", source)'
        self.assertIn(runtime, master)
        self.assertIn(bridge, master)
        self.assertLess(master.index(runtime), master.index(bridge))

    def test_enhanced_remains_independent_and_no_second_spatial_ui_is_created(self) -> None:
        ui = self.read(PATCHES / "apply_enhanced_ui.py")
        self.assertIn('CONTROL         "enhanced",IDC_ENHANCED_ENABLED_VGM', ui)
        self.assertIn("cfg_surround_sound", ui)
        self.assertIn("cfg_vgm_enhanced_enabled", ui)
        self.assertNotIn('CONTROL         "Spatial"', ui)

    def test_shared_product_contract_is_only_surround_plus_enhanced(self) -> None:
        model = self.read(ROOT / "model" / "spatial_playback_options.h")
        self.assertIn("bool surround = false;", model)
        self.assertIn("bool enhanced = false;", model)
        self.assertNotIn("source_full_sphere", model)
        self.assertNotIn("externalization =", model)
        self.assertNotIn("depth =", model)


if __name__ == "__main__":
    unittest.main()
