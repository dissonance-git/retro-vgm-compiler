#!/usr/bin/env python3
"""Exercise the YM2151 host-reference capture patches on maintained player source."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "foo_input_vgm" / "apply_ym2151_reference_capture.py"
STARTUP_FIX = ROOT / "patches" / "foo_input_vgm" / "apply_ym2151_reference_startup_fix.py"
CHAIN = ROOT / "patches" / "foo_input_vgm" / "apply_enhanced_component.py"
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
    def test_maintained_player_composes_exact_opm_reference_plane(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ym2151-host-capture-") as temporary:
            root = Path(temporary)
            shutil.copy2(OWNED / "source_aware_vgm_player.h", root / "source_aware_vgm_player.h")
            shutil.copy2(OWNED / "ym2151_source_plane.h", root / "ym2151_source_plane.h")

            completed = run_patch(PATCH, root)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            startup = run_patch(STARTUP_FIX, root)
            self.assertEqual(startup.returncode, 0, startup.stderr)
            text = (root / "source_aware_vgm_player.h").read_text(encoding="utf-8")

            self.assertIn('#include "ym2151_source_plane.h"', text)
            self.assertIn("void ym2151_set_source_tap", text)
            self.assertIn("static constexpr std::size_t kOpmLaneCount", text)
            self.assertIn("options.emuCore[0] = FCC_MAME;", text)
            self.assertIn("opm_source_topology_supported()", text)
            self.assertIn("opm_source_output(foobar_vgm::ym2151::source_lane lane)", text)
            self.assertIn("ym2151_set_source_tap(chip, &SourceAwareVGMPlayer::opm_source_tap", text)
            self.assertIn("sum_l != static_cast<INT64>(mix_left)", text)
            self.assertIn("mirror_opm_segment(outputOffset, outputCount)", text)
            self.assertIn("m_opm_output[lane].data() + outputOffset", text)
            self.assertIn("promote_initial_pregen(m_opm);", text)

            # OPM topology is independent. The existing Genesis predicate must
            # not suddenly include m_opm or m_unsupported_opm_topology.
            start = text.index("    bool source_topology_supported() const noexcept")
            end = text.index("    bool source_block_complete() const noexcept", start)
            genesis_predicate = text[start:end]
            self.assertNotIn("m_opm", genesis_predicate)
            self.assertNotIn("m_unsupported_opm_topology", genesis_predicate)

    def test_component_chain_applies_capture_after_source_aware_player(self) -> None:
        chain = CHAIN.read_text(encoding="utf-8")
        source_aware = 'run(here / "apply_source_aware_player.py", source)'
        opm = 'run(here / "apply_ym2151_reference_capture.py", source)'
        startup = 'run(here / "apply_ym2151_reference_startup_fix.py", source)'
        shadow = 'run(here / "apply_source_aware_shadow_include.py", source)'
        self.assertIn(opm, chain)
        self.assertIn(startup, chain)
        self.assertLess(chain.index(source_aware), chain.index(opm))
        self.assertLess(chain.index(opm), chain.index(startup))
        self.assertLess(chain.index(startup), chain.index(shadow))


if __name__ == "__main__":
    unittest.main()
