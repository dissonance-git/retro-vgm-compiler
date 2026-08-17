import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class StudioHqFmRuntimePatchTest(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(__file__).resolve().parents[2]
        self.source = self.root / "components/vgm/foo_input_vgm/src"
        self.patches = self.root / "patches/foo_input_vgm"

    @staticmethod
    def run_patch(script: Path, source_dir: Path) -> None:
        subprocess.run(
            [sys.executable, "-B", str(script), str(source_dir)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    def test_real_generated_chain_has_one_ordinal_owner_and_deferred_transport(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            generated = Path(tmp)
            for name in (
                "input_vgm.h",
                "input_vgm_shadow.cpp",
                "source_aware_vgm_player.h",
            ):
                (generated / name).write_bytes((self.source / name).read_bytes())

            # apply_enhanced_ui.py owns the real declaration. The runtime patch
            # only needs that exact generated contract, so keep this fixture
            # deliberately minimal rather than dragging UI resources into a
            # transport-generation test.
            (generated / "my_cfg_external.h").write_text(
                "extern cfg_int cfg_vgm_enhanced_enabled;\n",
                encoding="utf-8",
            )

            for script in (
                "apply_hq_nuked_fm_lift.py",
                "apply_studio_hq_fm_observer.py",
                "apply_enhanced_runtime.py",
                "apply_studio_hq_fm_runtime.py",
                "apply_studio_hq_fm_session_reset.py",
            ):
                self.run_patch(self.patches / script, generated)

            header = (generated / "input_vgm.h").read_text(encoding="utf-8-sig")
            shadow = (generated / "input_vgm_shadow.cpp").read_text(
                encoding="utf-8-sig"
            )
            source_player = (generated / "source_aware_vgm_player.h").read_text(
                encoding="utf-8-sig"
            )

        # Observer owns source-time diagnostics exactly once. The audible
        # transport consumes these methods but must not define a competing copy.
        for method in (
            "studio_hq_fm_domain_started() const noexcept",
            "studio_hq_fm_first_destination_ordinal() const noexcept",
            "studio_hq_fm_next_destination_ordinal() const noexcept",
            "studio_hq_fm_next_release_ordinal() const noexcept",
        ):
            self.assertEqual(source_player.count(method), 1, method)

        self.assertIn('#include "studio_frame_transport.h"', header)
        capacity_match = re.search(r"studio_frame_transport<(\d+)>", header)
        self.assertIsNotNone(capacity_match)
        transport_capacity = int(capacity_match.group(1))

        base_source = (self.source / "source_aware_vgm_player.h").read_text(
            encoding="utf-8-sig"
        )
        output_match = re.search(r"kOutputCapacity\s*=\s*(\d+)", base_source)
        self.assertIsNotNone(output_match)
        output_capacity = int(output_match.group(1))
        self.assertGreaterEqual(
            transport_capacity,
            2 * output_capacity + 64,
            "transport must retain two complete host blocks plus FIR support",
        )

        # Once deferred transport owns continuity, block-local linear FM and PSG
        # candidates are explicitly disabled. Protected reference remains in the
        # whole-frame queue while only Studio FM waits for future support.
        self.assertIn(
            "const bool fm_ready = !m_studio_deferred_engaged", shadow
        )
        self.assertIn(
            "const bool psg_ready = !m_studio_deferred_engaged", shadow
        )
        self.assertIn("input.protected_left = rendered_samples[frame].L;", shadow)
        self.assertIn("input.protected_right = rendered_samples[frame].R;", shadow)
        self.assertIn("input.exact_fm_left +=", shadow)
        self.assertIn("input.exact_fm_right +=", shadow)
        self.assertIn("pop_studio_hq_fm_ready_frame(ready)", shadow)
        self.assertIn("apply_studio_fm(", shadow)
        self.assertIn("ready.destination_ordinal", shadow)
        self.assertIn(
            "studio_hq_fm_next_destination_ordinal() == rendered_end", shadow
        )

        # EOF and quality failure are explicit reference paths, never implicit
        # zeros or FM-only delay.
        self.assertIn("finish_studio_hq_fm_reference_tail()", shadow)
        self.assertIn("finish_reference_tail(tail)", shadow)
        self.assertIn("m_studio_fm_transport.fail_closed_reference();", shadow)
        self.assertIn("void input_vgm::fail_studio_deferred_quality()", shadow)

        # Render-ahead changes the engine clock, so cumulative block-local source
        # capture is bypassed and shadow state follows the actual engine ordinal.
        self.assertIn(
            "m_source_capture_active = !m_studio_deferred_capture_bypass;", shadow
        )
        self.assertIn(
            "if (!m_studio_deferred_capture_bypass\n"
            "\t\t&& m_qsound_present && m_qsound_audio_shadow_valid",
            shadow,
        )
        self.assertIn(
            "advance_shadow_to(static_cast<uint_fast64_t>("
            "m_vgm_player->GetCurPos(PLAYPOS_SAMPLE)))",
            shadow,
        )

        # Every decode session is a new ordinal universe. The deferred callback
        # and retained future queue must be removed before input_base starts
        # PlayerA for the new session, not lazily on its first decode_run.
        self.assertEqual(
            header.count(
                "void decode_initialize(unsigned int p_flags, abort_callback &p_abort);"
            ),
            1,
        )
        initialize_start = shadow.index(
            "void input_vgm::decode_initialize(unsigned int p_flags, abort_callback &p_abort)"
        )
        decode_run_start = shadow.index(
            "bool input_vgm::decode_run(audio_chunk &p_chunk, abort_callback &p_abort)",
            initialize_start,
        )
        initialize = shadow[initialize_start:decode_run_start]
        unregister_init = initialize.index(
            "m_main_player.SetDeferredPostRenderProcessor(nullptr, nullptr);"
        )
        reset_init = initialize.index("m_studio_fm_transport.reset();")
        base_initialize = initialize.index(
            "input_base::decode_initialize(p_flags, p_abort);"
        )
        self.assertLess(unregister_init, reset_init)
        self.assertLess(reset_init, base_initialize)
        self.assertIn("m_studio_deferred_engaged = false;", initialize)
        self.assertIn("m_studio_deferred_active = false;", initialize)
        self.assertIn("m_studio_deferred_failed = false;", initialize)
        self.assertIn("m_studio_deferred_capture_bypass = false;", initialize)

        # Seek is a second real discontinuity. It may throw away retained future
        # only after unregistering deferred delivery.
        seek_start = shadow.index(
            "void input_vgm::decode_seek(double p_seconds, abort_callback &p_abort)"
        )
        seek = shadow[seek_start:]
        unregister = seek.index(
            "m_main_player.SetDeferredPostRenderProcessor(nullptr, nullptr);"
        )
        reset = seek.index("m_studio_fm_transport.reset();")
        self.assertLess(unregister, reset)
        self.assertIn("m_studio_deferred_engaged = false;", seek)
        self.assertIn("m_studio_deferred_active = false;", seek)
        self.assertIn("m_studio_deferred_failed = false;", seek)

    def test_component_chain_orders_observer_runtime_deferred_and_session_reset(self) -> None:
        chain = (self.patches / "apply_enhanced_component.py").read_text(
            encoding="utf-8"
        )
        hq = chain.index('run(here / "apply_hq_nuked_fm_lift.py", source)')
        observer = chain.index(
            'run(here / "apply_studio_hq_fm_observer.py", source)'
        )
        enhanced = chain.index('run(here / "apply_enhanced_runtime.py", source)')
        deferred = chain.index(
            'run(here / "apply_studio_hq_fm_runtime.py", source)'
        )
        session = chain.index(
            'run(here / "apply_studio_hq_fm_session_reset.py", source)'
        )
        self.assertLess(hq, observer)
        self.assertLess(observer, enhanced)
        self.assertLess(enhanced, deferred)
        self.assertLess(deferred, session)

    def test_deferred_runtime_does_not_patch_observer_api(self) -> None:
        runtime = (self.patches / "apply_studio_hq_fm_runtime.py").read_text(
            encoding="utf-8"
        )
        self.assertNotIn(
            'source_player = root / "source_aware_vgm_player.h"', runtime
        )
        self.assertNotIn('replace_once(\n        source_player,', runtime)
        self.assertIn(
            "# SourceAware's Studio observer owns every source-ordinal diagnostic.",
            runtime,
        )

    def test_session_reset_patch_has_single_narrow_owner(self) -> None:
        lifecycle = (
            self.patches / "apply_studio_hq_fm_session_reset.py"
        ).read_text(encoding="utf-8")
        self.assertIn(
            "A reused input_vgm instance must\ntherefore unregister any prior deferred callback",
            lifecycle,
        )
        self.assertIn(
            "input_base::decode_initialize(p_flags, p_abort);", lifecycle
        )
        self.assertNotIn("apply_studio_hq_fm_observer.py", lifecycle)
        self.assertNotIn("apply_enhanced_runtime.py", lifecycle)


if __name__ == "__main__":
    unittest.main()
