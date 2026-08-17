from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PATCH_DIR = ROOT / "patches" / "foo_input_vgm"


class EnhancedDacStreamPatchOrderTest(unittest.TestCase):
    def test_source_bank_observer_and_seek_bridge_order(self) -> None:
        text = (PATCH_DIR / "apply_enhanced_component.py").read_text(encoding="utf-8")

        def position(name: str) -> int:
            marker = f'run(here / "{name}", source)'
            result = text.find(marker)
            self.assertGreaterEqual(result, 0, marker)
            return result

        source_shadow = position("apply_source_aware_shadow_include.py")
        pcm_observer = position("apply_enhanced_dac_stream_observer.py")
        deferred_fm = position("apply_studio_hq_fm_runtime.py")
        seek_bridge = position("apply_enhanced_dac_stream_seek_order_bridge.py")
        deferred_psg = position("apply_studio_deferred_psg.py")
        direct_dac = position("apply_enhanced_dac_runtime.py")
        session_reset = position("apply_enhanced_dac_stream_session_reset.py")
        pcm_mix = position("apply_enhanced_dac_stream_mix.py")

        # The PCM observer binds while the base seek/decode anchors still exist.
        self.assertLess(source_shadow, pcm_observer)
        self.assertLess(pcm_observer, deferred_fm)

        # Deferred FM expands the seek lifecycle. The bridge restores the stable
        # adjacency the existing deferred PSG patch expects before PSG runs.
        self.assertLess(deferred_fm, seek_bridge)
        self.assertLess(seek_bridge, deferred_psg)

        # Audible source-bank composition runs only after the ordinary DAC path
        # and all deferred family machinery exist, then session state is guarded.
        self.assertLess(deferred_psg, direct_dac)
        self.assertLess(direct_dac, session_reset)
        self.assertLess(session_reset, pcm_mix)

    def test_seek_bridge_preserves_both_resets(self) -> None:
        text = (PATCH_DIR / "apply_enhanced_dac_stream_seek_order_bridge.py").read_text(
            encoding="utf-8"
        )
        self.assertIn("m_studio_deferred_capture_bypass = false", text)
        self.assertIn("reset_pcm_streams();", text)
        self.assertIn("input_base::decode_seek(p_seconds, p_abort);", text)
        self.assertIn("source-bank DAC seek reset relocation", text)


if __name__ == "__main__":
    unittest.main()
