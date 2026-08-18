#!/usr/bin/env python3
"""Guard the exact libvgm MAME YM2151 source-tap transformation."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
PATCH = ROOT / "patches" / "libvgm" / "apply_ym2151_source_tap.py"
CHAIN = ROOT / "patches" / "libvgm" / "apply_source_capture.py"

HEADER_FIXTURE = """#pragma once
extern DEV_DEF devDef_YM2151_MAME;
"""

SOURCE_FIXTURE = """typedef struct {
\tUINT8       lastreg;
\tvoid (*irqhandler)(void *param, UINT8 irq);
\tvoid (*portwritehandler)(void *param, UINT8 ofs, UINT8 data);
} YM2151;

static UINT8 ym2151_r(void *chip, UINT8 offset)
{
\tif (offset & 1)
\t\treturn ym2151_read_status(chip);
\telse
\t\treturn 0xff;    /* confirmed on a real YM2151 */
}

void ym2151_update(void *chip, DEV_SMPL **buffers, UINT32 samples)
{
\tUINT32 i;
\tint ch;
\tDEV_SMPL outl, outr;

\tfor (i = 0; i < samples; i++) {
\t\toutl = 0;
\t\toutr = 0;
\t\tfor(ch=0; ch<8; ch++) {
\t\t\toutl += PSG->chanout[ch] & PSG->pan[2*ch];
\t\t\toutr += PSG->chanout[ch] & PSG->pan[2*ch+1];
\t\t}
\t\tbuffers[0][i] = outl;
\t\tbuffers[1][i] = outr;
\t}
}
"""


class Ym2151SourceTapPatchTest(unittest.TestCase):
    def make_tree(self, root: Path, *, drift: bool = False) -> None:
        cores = root / "emu" / "cores"
        cores.mkdir(parents=True)
        (cores / "ym2151.h").write_text(HEADER_FIXTURE, encoding="utf-8")
        source = SOURCE_FIXTURE
        if drift:
            source = source.replace("for(ch=0; ch<8; ch++)", "for (ch = 0; ch < 8; ++ch)", 1)
        (cores / "ym2151.c").write_text(source, encoding="utf-8")

    def run_patch(self, root: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, "-B", str(PATCH), str(root)],
            cwd=str(ROOT),
            capture_output=True,
            text=True,
            check=False,
        )

    def test_exact_pinned_shape_gains_eight_channel_presum_tap(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ym2151-tap-") as temporary:
            root = Path(temporary)
            self.make_tree(root)
            completed = self.run_patch(root)
            self.assertEqual(completed.returncode, 0, completed.stderr)

            header = (root / "emu" / "cores" / "ym2151.h").read_text(encoding="utf-8")
            source = (root / "emu" / "cores" / "ym2151.c").read_text(encoding="utf-8")
            self.assertIn("typedef void (*YM2151_SOURCE_TAP)", header)
            self.assertIn("ym2151_set_source_tap", header)
            self.assertIn("YM2151_SOURCE_TAP sourceTap;", source)
            self.assertIn("DEV_SMPL sourceL[8], sourceR[8];", source)
            self.assertIn("sourceL[ch] = PSG->chanout[ch] & PSG->pan[2*ch];", source)
            self.assertIn("sourceR[ch] = PSG->chanout[ch] & PSG->pan[2*ch+1];", source)
            self.assertIn("outl += sourceL[ch];", source)
            self.assertIn("outr += sourceR[ch];", source)
            self.assertIn(
                "PSG->sourceTap(PSG->sourceTapUser, sourceL, sourceR, outl, outr);",
                source,
            )
            self.assertLess(source.index("buffers[1][i] = outr;"), source.index("PSG->sourceTap("))

            # Idempotency is part of the maintained patch contract.
            second = self.run_patch(root)
            self.assertEqual(second.returncode, 0, second.stderr)
            source2 = (root / "emu" / "cores" / "ym2151.c").read_text(encoding="utf-8")
            self.assertEqual(source2.count("PSG->sourceTap("), 1)

    def test_upstream_sum_drift_fails_instead_of_guessing(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ym2151-tap-drift-") as temporary:
            root = Path(temporary)
            self.make_tree(root, drift=True)
            completed = self.run_patch(root)
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("expected exactly one match", completed.stderr)

    def test_canonical_source_capture_chain_runs_tap(self) -> None:
        chain = CHAIN.read_text(encoding="utf-8")
        self.assertIn('run(here / "apply_ym2151_source_tap.py", root)', chain)
        self.assertLess(
            chain.index('run(here / "apply_sn76496_source_tap.py", root)'),
            chain.index('run(here / "apply_ym2151_source_tap.py", root)'),
        )


if __name__ == "__main__":
    unittest.main()
