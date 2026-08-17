import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


FIXTURE = """\t\t\tstudio_block = source_player != nullptr
\t\t\t\t&& source_player->source_topology_supported()
\t\t\t\t&& source_player->source_block_complete()
\t\t\t\t&& source_player->source_output_count() == rendered_count
\t\t\t\t&& source_player->ym_source_expected()
\t\t\t\t&& source_player->ym_source_block_valid()
\t\t\t\t&& source_player->hq_fm_source_block_valid()
\t\t\t\t&& source_player->studio_hq_fm_observer_valid();
"""


class DeferredFamilyIndependenceTest(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(__file__).resolve().parents[2]
        self.patches = self.root / "patches/foo_input_vgm"

    def test_fm_admission_does_not_depend_on_all_family_block_health(self) -> None:
        patch = self.patches / "apply_studio_deferred_family_independence.py"
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            shadow = source / "input_vgm_shadow.cpp"
            shadow.write_text(FIXTURE, encoding="utf-8")
            subprocess.run(
                [sys.executable, "-B", str(patch), str(source)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            generated = shadow.read_text(encoding="utf-8")

        self.assertIn("source_player->source_topology_supported()", generated)
        self.assertIn("source_player->source_output_count() == rendered_count", generated)
        self.assertIn("source_player->ym_source_expected()", generated)
        self.assertIn("source_player->ym_source_block_valid()", generated)
        self.assertIn("source_player->hq_fm_source_block_valid()", generated)
        self.assertNotIn("source_player->source_block_complete()", generated)

    def test_complete_component_chain_applies_independence_after_psg_composition(self) -> None:
        chain = (self.patches / "apply_enhanced_component.py").read_text(
            encoding="utf-8"
        )
        runtime = chain.index(
            'run(here / "apply_studio_hq_fm_runtime.py", source)'
        )
        psg = chain.index(
            'run(here / "apply_studio_deferred_psg.py", source)'
        )
        independence = chain.index(
            'run(here / "apply_studio_deferred_family_independence.py", source)'
        )
        fail_closed = chain.index(
            'run(here / "apply_studio_deferred_psg_fail_closed.py", source)'
        )
        self.assertLess(runtime, psg)
        self.assertLess(psg, independence)
        self.assertLess(independence, fail_closed)


if __name__ == "__main__":
    unittest.main()
