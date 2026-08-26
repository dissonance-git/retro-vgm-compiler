#!/usr/bin/env python3
"""Exercise the independent YM2151 host-capture reference experiment."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH_DIR = ROOT / "patches" / "foo_input_vgm"
PATCH = PATCH_DIR / "apply_ym2151_reference_capture.py"
COMPAT = PATCH_DIR / "apply_ym2151_hq_fm_compat.py"
HQ = PATCH_DIR / "apply_hq_nuked_fm_lift.py"
STARTUP_FIX = PATCH_DIR / "apply_ym2151_reference_startup_fix.py"
CHAIN = PATCH_DIR / "apply_enhanced_component.py"
OWNED = ROOT / "components" / "vgm" / "foo_input_vgm" / "src"


def run_patch(script: Path, root: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, "-B", str(script), str(root)],
        cwd=str(ROOT),
        capture_output=True,
        text=True,
        check=False,
    )


class Ym2151ReferenceCapturePatchTest(unittest.TestCase):
    def test_maintained_player_composes_with_genesis_hq_fm(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ym2151-host-capture-") as temporary:
            root = Path(temporary)
            shutil.copy2(OWNED / "source_aware_vgm_player.h", root / "source_aware_vgm_player.h")
            shutil.copy2(OWNED / "ym2151_source_plane.h", root / "ym2151_source_plane.h")

            for script in (PATCH, COMPAT, HQ, STARTUP_FIX):
                completed = run_patch(script, root)
                self.assertEqual(
                    completed.returncode,
                    0,
                    f"{script.name} failed:\nstdout={completed.stdout}\nstderr={completed.stderr}",
                )

            text = (root / "source_aware_vgm_player.h").read_text(encoding="utf-8")
            self.assertIn('#include "ym2151_source_plane.h"', text)
            self.assertIn("void ym2151_set_source_tap", text)
            self.assertIn("static constexpr std::size_t kOpmLaneCount", text)
            self.assertIn("static constexpr std::size_t kHqFmLaneCount", text)
            self.assertIn("options.emuCore[0] = FCC_MAME;", text)
            self.assertIn("opm_source_topology_supported()", text)
            self.assertIn("opm_source_output(foobar_vgm::ym2151::source_lane lane)", text)
            self.assertIn("ym2151_set_source_tap(chip, &SourceAwareVGMPlayer::opm_source_tap", text)
            self.assertIn("sum_l != static_cast<INT64>(mix_left)", text)
            self.assertIn("mirror_opm_segment(outputOffset, outputCount)", text)
            self.assertIn("m_opm_output[lane].data() + outputOffset", text)
            self.assertIn("promote_initial_hq_pregen(m_ym);", text)
            self.assertIn("promote_initial_pregen(m_opm);", text)
            self.assertIn("m_hq_fm_block_valid = false;", text)
            self.assertIn("m_opm_block_valid = false;", text)

            # OPM topology remains an independent reference lane. The Genesis
            # predicate must not silently absorb OPM state.
            start = text.index("    bool source_topology_supported() const noexcept")
            end = text.index("    bool source_block_complete() const noexcept", start)
            genesis_predicate = text[start:end]
            self.assertNotIn("m_opm", genesis_predicate)
            self.assertNotIn("m_unsupported_opm_topology", genesis_predicate)

    def test_installable_component_keeps_reference_capture_independent(self) -> None:
        chain = CHAIN.read_text(encoding="utf-8")
        self.assertIn('run(here / "apply_source_aware_player.py", source)', chain)
        self.assertIn('run(here / "apply_hq_nuked_fm_lift.py", source)', chain)
        for experiment in (
            "apply_ym2151_reference_capture.py",
            "apply_ym2151_hq_fm_compat.py",
            "apply_ym2151_reference_startup_fix.py",
        ):
            self.assertNotIn(experiment, chain)


if __name__ == "__main__":
    unittest.main()
