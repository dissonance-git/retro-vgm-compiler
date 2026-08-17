import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class SpcSpatialOmniphonyRateLifecyclePatchTest(unittest.TestCase):
    def test_rebinds_rate_specific_spc_processor_without_source_policy(self):
        repo = Path(__file__).resolve().parents[2]
        script = (
            repo
            / "patches"
            / "snesapu"
            / "apply_spatial_omniphony_private_rate_lifecycle.py"
        )
        predecessor = r'''bool input_snesapu::EnsureOmniphony() noexcept
{
	if (m_Omniphony.renderer_bound())
		return true;
	if (m_OmniphonyLoader.open())
		return m_OmniphonyLoader.bind(m_Omniphony);
	if (m_OmniphonyAttempted)
		return false;

	m_OmniphonyAttempted = true;
	vgmtooling::model::omniphony_source_config_transport config{};
	config.sample_rate_hz = static_cast<std::uint32_t>(m_CnfSampleRate);
	config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
	config.externalization = 1u;
	config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
	config.unit_scale_m = 1.0f;
	config.reflection_level = 0.22f;
	if (!m_OmniphonyLoader.open_default(config))
		return false;
	return m_OmniphonyLoader.bind(m_Omniphony);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            parent = root / "foobar2000" / "foo_snesapu"
            parent.mkdir(parents=True)
            header = parent / "input_snesapu.hpp"
            source = parent / "input_snesapu.cpp"
            header.write_text(
                "\tbool m_OmniphonyAttempted = false;\n",
                encoding="utf-8",
            )
            source.write_text(predecessor, encoding="utf-8")

            first = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)

            patched_header = header.read_text(encoding="utf-8")
            patched_source = source.read_text(encoding="utf-8")
            self.assertIn("m_OmniphonySampleRate = 0", patched_header)
            self.assertIn("desired_sample_rate", patched_source)
            self.assertIn("m_Omniphony.unbind_renderer();", patched_source)
            self.assertIn("m_OmniphonyLoader.close();", patched_source)
            self.assertIn(
                "m_OmniphonySampleRate = desired_sample_rate;",
                patched_source,
            )
            self.assertLess(
                patched_source.index("m_Omniphony.unbind_renderer();"),
                patched_source.index("m_OmniphonyLoader.close();"),
            )
            self.assertNotIn("cfg_enhanced_enabled", patched_source)

            second = subprocess.run(
                [sys.executable, str(script), str(root)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)

    def test_rate_lifecycle_runs_after_private_spatial_runtime(self):
        repo = Path(__file__).resolve().parents[2]
        chain = (
            repo / "patches" / "snesapu" / "apply_private_component.py"
        ).read_text(encoding="utf-8")
        runtime = chain.index(
            'run(here / "apply_spatial_omniphony_private_runtime.py", root)'
        )
        rate = chain.index(
            'run(here / "apply_spatial_omniphony_private_rate_lifecycle.py", root)'
        )
        self.assertLess(runtime, rate)


if __name__ == "__main__":
    unittest.main()
