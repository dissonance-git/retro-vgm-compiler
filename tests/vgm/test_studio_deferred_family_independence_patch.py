import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class StudioDeferredFamilyIndependencePatchTest(unittest.TestCase):
    def test_removes_only_all_family_dynamic_gate(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "foo_input_vgm" / "apply_studio_deferred_family_independence.py"
        predecessor = """\t\t\tstudio_block = source_player != nullptr
\t\t\t\t&& source_player->source_topology_supported()
\t\t\t\t&& source_player->source_block_complete()
\t\t\t\t&& source_player->source_output_count() == rendered_count
\t\t\t\t&& source_player->ym_source_expected()
\t\t\t\t&& source_player->ym_source_block_valid()
\t\t\t\t&& source_player->hq_fm_source_block_valid()
\t\t\t\t&& source_player->hq_fm_source_count() == rendered_count;
"""
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            shadow = source / "input_vgm_shadow.cpp"
            shadow.write_text(predecessor, encoding="utf-8")
            first = subprocess.run([sys.executable, str(script), str(source)], cwd=repo, capture_output=True, text=True, check=False)
            self.assertEqual(first.returncode, 0, first.stderr)
            patched = shadow.read_text(encoding="utf-8")
            self.assertNotIn("source_player->source_block_complete()", patched)
            self.assertIn("source_player->source_topology_supported()", patched)
            self.assertIn("source_player->ym_source_block_valid()", patched)
            self.assertIn("source_player->hq_fm_source_block_valid()", patched)
            second = subprocess.run([sys.executable, str(script), str(source)], cwd=repo, capture_output=True, text=True, check=False)
            self.assertNotEqual(second.returncode, 0)

    def test_component_chain_places_family_gate_after_psg_runtime(self):
        repo = Path(__file__).resolve().parents[2]
        chain = (repo / "patches" / "foo_input_vgm" / "apply_enhanced_component.py").read_text(encoding="utf-8")
        deferred = chain.index('run(here / "apply_studio_hq_fm_runtime.py", source)')
        psg = chain.index('run(here / "apply_studio_deferred_psg.py", source)')
        independence = chain.index('run(here / "apply_studio_deferred_family_independence.py", source)')
        psg_fail = chain.index('run(here / "apply_studio_deferred_psg_fail_closed.py", source)')
        self.assertLess(deferred, psg)
        self.assertLess(psg, independence)
        self.assertLess(independence, psg_fail)


if __name__ == "__main__":
    unittest.main()
