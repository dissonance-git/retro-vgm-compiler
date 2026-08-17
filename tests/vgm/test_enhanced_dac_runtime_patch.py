from __future__ import annotations

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
PATCHES = ROOT / "patches/foo_input_vgm"


class EnhancedDacRuntimePatchTests(unittest.TestCase):
    def test_patch_preserves_source_identity_and_fails_closed(self) -> None:
        text = (PATCHES / "apply_enhanced_dac_runtime.py").read_text(encoding="utf-8")
        self.assertIn("source_lane::ym2612_dac", text)
        self.assertIn("m_dac_capture.events(0)", text)
        self.assertIn("m_dac_capture.pan_changed(0)", text)
        self.assertIn("source_player->ym_source_volume", text)
        self.assertIn("m_enhanced_dac_source_block.render", text)
        self.assertIn("- static_cast<std::int64_t>(exact[f].left)", text)
        self.assertIn("- static_cast<std::int64_t>(exact[f].right)", text)
        self.assertIn("!m_studio_deferred_engaged", text)
        self.assertIn("m_enhanced_dac_block_rendered = true", text)

    def test_complete_chain_applies_dac_after_deferred_state_exists(self) -> None:
        chain = (PATCHES / "apply_enhanced_component.py").read_text(encoding="utf-8")
        deferred = chain.index('run(here / "apply_studio_hq_fm_runtime.py", source)')
        dac = chain.index('run(here / "apply_enhanced_dac_runtime.py", source)')
        self.assertLess(deferred, dac)


if __name__ == "__main__":
    unittest.main()
