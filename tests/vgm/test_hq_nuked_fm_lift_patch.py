import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class HqNukedFmLiftPatchTest(unittest.TestCase):
    def test_startup_pregeneration_survives_base_reset(self) -> None:
        root = Path(__file__).resolve().parents[2]
        base_header = root / "components/vgm/foo_input_vgm/src/source_aware_vgm_player.h"
        patch = root / "patches/foo_input_vgm/apply_hq_nuked_fm_lift.py"

        with tempfile.TemporaryDirectory() as tmp:
            source_dir = Path(tmp)
            generated = source_dir / "source_aware_vgm_player.h"
            generated.write_bytes(base_header.read_bytes())
            subprocess.run(
                [sys.executable, "-B", str(patch), str(source_dir)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            text = generated.read_text(encoding="utf-8-sig")

        reset_start = text.index("    UINT8 Reset() override")
        reset_end = text.index("    UINT8 Seek", reset_start)
        reset_body = text[reset_start:reset_end]

        self.assertIn("promote_initial_pregen(m_ym);", reset_body)
        self.assertIn("promote_initial_hq_pregen(m_ym);", reset_body)
        self.assertLess(
            reset_body.index("promote_initial_pregen(m_ym);"),
            reset_body.index("promote_initial_hq_pregen(m_ym);"),
        )
        self.assertLess(
            reset_body.index("promote_initial_hq_pregen(m_ym);"),
            reset_body.index("const UINT8 result = VGMPlayer::Reset();"),
        )

        after_base_reset = reset_body.split(
            "const UINT8 result = VGMPlayer::Reset();", 1
        )[1]
        self.assertNotIn("reset_hq_fm_histories(m_ym);", after_base_reset)

        # Fresh device attachment is still a legitimate discontinuity and must
        # clear both exact and HQ interpolation state transactionally.
        self.assertIn("reset_all_histories(m_ym);", text)
        self.assertIn("reset_hq_fm_histories(m_ym);", text)
        self.assertIn("hq_history[lane].next = family.hq_native[lane][0];", text)


if __name__ == "__main__":
    unittest.main()
