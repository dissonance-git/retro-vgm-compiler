from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PATCH_DIR = ROOT / "patches" / "foo_input_vgm"
OWNED = ROOT / "components" / "vgm" / "foo_input_vgm" / "src"


class EnhancedDacStreamRuntimePatchTest(unittest.TestCase):
    def test_observer_session_and_mix_chain(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp)
            for name in ("input_vgm.h", "input_vgm.cpp", "input_vgm_shadow.cpp"):
                shutil.copy2(OWNED / name, src / name)

            for name in (
                "apply_enhanced_dac_stream_observer.py",
                "apply_enhanced_dac_stream_session_reset.py",
                "apply_enhanced_dac_stream_mix.py",
            ):
                subprocess.run(
                    [sys.executable, str(PATCH_DIR / name), str(src)],
                    check=True,
                    cwd=ROOT,
                )

            header = (src / "input_vgm.h").read_text(encoding="utf-8")
            player = (src / "input_vgm.cpp").read_text(encoding="utf-8")
            shadow = (src / "input_vgm_shadow.cpp").read_text(encoding="utf-8")

            self.assertIn("ym2612_pcm_source_queue.h", header)
            self.assertIn("ym2612_pcm_stream_bank m_pcm_streams", header)
            self.assertIn("m_enhanced_pcm_source_scratch", header)
            self.assertIn(
                "apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample)",
                header,
            )

            self.assertIn("SetDACStreamSourceObserver(&input_vgm::dac_stream_source_callback, this)", player)
            self.assertIn("SetDACStreamSourceObserver(nullptr, nullptr)", player)
            self.assertIn("mapped.sample", shadow)
            self.assertIn("Tick2Sample", shadow)
            self.assertIn("m_pcm_stream_queue.render_until", shadow)
            self.assertIn("m_pcm_stream_queue.pop_expected", shadow)
            self.assertIn("replace_reference", shadow)
            self.assertIn("+ enhanced.left - static_cast<std::int64_t>(exact_dac[frame].left)", shadow)
            self.assertIn("deferred_pcm_changed", shadow)
            self.assertIn("input.protected_left = deferred_pcm_changed", shadow)
            self.assertIn("reset_pcm_streams();\n\tinput_base::decode_initialize", shadow)
            self.assertIn("m_pcm_stream_queue.reset(static_cast<std::uint64_t>(seek_sample))", shadow)
            self.assertIn("!m_pcm_stream_queue_failed && !pcm_stream_changed", shadow)


if __name__ == "__main__":
    unittest.main()
