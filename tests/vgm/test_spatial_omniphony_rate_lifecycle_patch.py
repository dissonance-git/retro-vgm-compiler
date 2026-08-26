import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class SpatialOmniphonyRateLifecyclePatchTest(unittest.TestCase):
    def test_rebinds_rate_specific_processor_without_touching_source_policy(self):
        repo = Path(__file__).resolve().parents[2]
        script = (
            repo
            / "patches"
            / "foo_input_vgm"
            / "apply_spatial_omniphony_rate_lifecycle.py"
        )
        predecessor = r'''bool input_vgm::ensure_genesis_omniphony() noexcept
{
	if (m_genesis_omniphony.renderer_bound())
		return true;

	if (!m_genesis_omniphony_loader.open())
	{
		if (m_genesis_omniphony_attempted)
			return false;
		m_genesis_omniphony_attempted = true;

		vgmtooling::model::omniphony_source_config_transport config{};
		config.sample_rate_hz = static_cast<std::uint32_t>(m_sample_rate);
		config.spatial_mode = vgmtooling::model::omniphony_source_spatial_full_sphere;
		config.externalization = 1u;
		config.hrir_source = vgmtooling::model::omniphony_source_hrir_saf_kemar;
		config.unit_scale_m = 1.0f;
		config.reflection_level = 0.22f;
		if (!m_genesis_omniphony_loader.open_default(config))
			return false;
	}

	return m_genesis_omniphony_loader.bind(m_genesis_omniphony);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            header = source / "input_vgm.h"
            shadow = source / "input_vgm_shadow.cpp"
            header.write_text(
                "\tbool m_genesis_omniphony_attempted = false;\n",
                encoding="utf-8",
            )
            shadow.write_text(predecessor, encoding="utf-8")

            first = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(first.returncode, 0, first.stderr)

            patched_header = header.read_text(encoding="utf-8")
            patched_shadow = shadow.read_text(encoding="utf-8")
            self.assertIn("m_genesis_omniphony_sample_rate = 0", patched_header)
            self.assertIn("desired_sample_rate", patched_shadow)
            self.assertIn("m_genesis_omniphony.unbind_renderer();", patched_shadow)
            self.assertIn("m_genesis_omniphony_loader.close();", patched_shadow)
            self.assertIn(
                "m_genesis_omniphony_sample_rate = desired_sample_rate;",
                patched_shadow,
            )
            self.assertLess(
                patched_shadow.index("m_genesis_omniphony.unbind_renderer();"),
                patched_shadow.index("m_genesis_omniphony_loader.close();"),
            )
            self.assertNotIn("cfg_vgm_enhanced_enabled", patched_shadow)

            second = subprocess.run(
                [sys.executable, str(script), str(source)],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(second.returncode, 0)

    def test_rate_lifecycle_is_dormant_in_product_chain(self):
        repo = Path(__file__).resolve().parents[2]
        chain = (
            repo / "patches" / "foo_input_vgm" / "apply_enhanced_component.py"
        ).read_text(encoding="utf-8")
        self.assertIn(
            'run(here / "apply_spatial_omniphony_runtime.py", source)',
            chain,
        )
        self.assertNotIn(
            'run(here / "apply_spatial_omniphony_rate_lifecycle.py", source)',
            chain,
        )
        self.assertTrue(
            (
                repo
                / "patches"
                / "foo_input_vgm"
                / "apply_spatial_omniphony_rate_lifecycle.py"
            ).is_file()
        )


if __name__ == "__main__":
    unittest.main()
