import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class StudioHqFmObserverPatchTest(unittest.TestCase):
    def test_observer_is_live_but_non_audible(self) -> None:
        root = Path(__file__).resolve().parents[2]
        base_header = root / "components/vgm/foo_input_vgm/src/source_aware_vgm_player.h"
        hq_patch = root / "patches/foo_input_vgm/apply_hq_nuked_fm_lift.py"
        observer_patch = root / "patches/foo_input_vgm/apply_studio_hq_fm_observer.py"

        with tempfile.TemporaryDirectory() as tmp:
            source_dir = Path(tmp)
            generated = source_dir / "source_aware_vgm_player.h"
            generated.write_bytes(base_header.read_bytes())
            for patch in (hq_patch, observer_patch):
                subprocess.run(
                    [sys.executable, "-B", str(patch), str(source_dir)],
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                )
            text = generated.read_text(encoding="utf-8-sig")

        self.assertIn('#include "studio_hq_fm_observer.h"', text)
        self.assertIn("observe_initial_studio_hq_pregen(m_ym);", text)
        self.assertIn("observe_studio_hq_fm_segment(\n                    m_ym, outputCount)", text)
        self.assertIn("studio_hq_fm_observer_valid() const noexcept", text)
        self.assertIn("studio_hq_fm_domain_started() const noexcept", text)
        self.assertIn("studio_hq_fm_first_destination_ordinal() const noexcept", text)
        self.assertIn("studio_hq_fm_next_destination_ordinal() const noexcept", text)
        self.assertIn("studio_hq_fm_next_release_ordinal() const noexcept", text)
        self.assertIn(
            "m_studio_hq_fm_observer.first_studio_destination_ordinal()", text
        )
        self.assertIn("m_studio_hq_fm_observer.next_destination_ordinal()", text)
        self.assertIn("m_studio_hq_fm_observer.next_release_ordinal()", text)
        self.assertIn("m_studio_hq_fm_observer.configure(\n                    base.resmpl.smpRateSrc,\n                    base.resmpl.smpRateDst)", text)

        # Startup order is an identity invariant: exact and HQ histories first,
        # then the same pregenerated HQ sample enters Studio ordinal zero, all
        # before the base player reset advances normal rendering.
        reset_start = text.index("    UINT8 Reset() override")
        reset_end = text.index("    UINT8 Seek", reset_start)
        reset_body = text[reset_start:reset_end]
        self.assertLess(
            reset_body.index("promote_initial_hq_pregen(m_ym);"),
            reset_body.index("observe_initial_studio_hq_pregen(m_ym);"),
        )
        self.assertLess(
            reset_body.index("observe_initial_studio_hq_pregen(m_ym);"),
            reset_body.index("const UINT8 result = VGMPlayer::Reset();"),
        )
        self.assertIn(
            "m_studio_hq_fm_active = m_studio_hq_fm_observer.configured();",
            reset_body,
        )

        # A seek discards native FIR history but keeps destination tags in the
        # absolute PlayerA playback-sample coordinate. Studio therefore becomes
        # temporarily reference-only while rebuilding lookahead, not disabled.
        seek_start = text.index("    UINT8 Seek")
        seek_end = text.index("    UINT8 Stop", seek_start)
        seek_body = text[seek_start:seek_end]
        self.assertIn("VGMPlayer::GetCurPos(PLAYPOS_SAMPLE)", seek_body)
        self.assertIn("m_studio_hq_fm_observer.reset(destination_base);", seek_body)
        self.assertIn(
            "m_studio_hq_fm_active = m_studio_hq_fm_observer.configured();",
            seek_body,
        )
        self.assertNotIn("m_studio_hq_fm_active = false;", seek_body)

        # This pass is intentionally observability-only. The audible HQ buffer is
        # still produced by the historical linear mirror and there must be no
        # Studio reconstruction write into m_hq_fm_output in this patch stage.
        mirror_start = text.index("    bool mirror_hq_fm_segment(")
        mirror_end = text.index("    void observe_initial_studio_hq_pregen", mirror_start)
        mirror_body = text[mirror_start:mirror_end]
        self.assertIn("mirror_linear_segment(", mirror_body)
        self.assertIn("m_hq_fm_output[lane].data()", mirror_body)

        observe_start = text.index("    bool observe_studio_hq_fm_segment(")
        observe_end = text.index("    template <typename Family>", observe_start)
        observe_body = text[observe_start:observe_end]
        self.assertNotIn("m_hq_fm_output", observe_body)
        self.assertIn("m_studio_hq_fm_observer.observe_segment(", observe_body)

    def test_component_patch_order_keeps_observer_between_hq_and_runtime(self) -> None:
        root = Path(__file__).resolve().parents[2]
        component = (
            root / "patches/foo_input_vgm/apply_enhanced_component.py"
        ).read_text(encoding="utf-8")
        hq = component.index('run(here / "apply_hq_nuked_fm_lift.py", source)')
        observer = component.index(
            'run(here / "apply_studio_hq_fm_observer.py", source)'
        )
        runtime = component.index('run(here / "apply_enhanced_runtime.py", source)')
        self.assertLess(hq, observer)
        self.assertLess(observer, runtime)


if __name__ == "__main__":
    unittest.main()
