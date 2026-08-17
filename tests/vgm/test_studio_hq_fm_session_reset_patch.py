import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class StudioHqFmSessionResetPatchTest(unittest.TestCase):
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

    def test_decode_session_discards_prior_future_before_player_start(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            generated = Path(tmp)
            for name in (
                "input_vgm.h",
                "input_vgm_shadow.cpp",
                "source_aware_vgm_player.h",
            ):
                (generated / name).write_bytes((self.source / name).read_bytes())

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

        declaration = (
            "void decode_initialize(unsigned int p_flags, "
            "abort_callback &p_abort) override;"
        )
        self.assertEqual(header.count(declaration), 1)

        start = shadow.index(
            "void input_vgm::decode_initialize(unsigned int p_flags, "
            "abort_callback &p_abort)"
        )
        end = shadow.index("bool input_vgm::decode_run", start)
        initialize = shadow[start:end]

        unregister = initialize.index(
            "m_main_player.SetDeferredPostRenderProcessor(nullptr, nullptr);"
        )
        reset = initialize.index("m_studio_fm_transport.reset();")
        disengage = initialize.index("m_studio_deferred_engaged = false;")
        deactivate = initialize.index("m_studio_deferred_active = false;")
        clear_failure = initialize.index("m_studio_deferred_failed = false;")
        clear_bypass = initialize.index("m_studio_deferred_capture_bypass = false;")
        base = initialize.index("input_base::decode_initialize(p_flags, p_abort);")

        self.assertLess(unregister, reset)
        self.assertLess(reset, disengage)
        self.assertLess(disengage, deactivate)
        self.assertLess(deactivate, clear_failure)
        self.assertLess(clear_failure, clear_bypass)
        self.assertLess(clear_bypass, base)

    def test_component_chain_applies_session_reset_after_deferred_runtime(self) -> None:
        chain = (self.patches / "apply_enhanced_component.py").read_text(
            encoding="utf-8"
        )
        deferred = chain.index(
            'run(here / "apply_studio_hq_fm_runtime.py", source)'
        )
        session_reset = chain.index(
            'run(here / "apply_studio_hq_fm_session_reset.py", source)'
        )
        self.assertLess(deferred, session_reset)


if __name__ == "__main__":
    unittest.main()
