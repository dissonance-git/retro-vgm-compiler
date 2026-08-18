from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "tools" / "build_private_foobar_components.ps1"


class PrivateComponentBuilderContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.text = BUILDER.read_text(encoding="utf-8")

    def test_vgm_overlay_is_the_only_shadow_project_integration(self) -> None:
        self.assertIn("Directory.Build.targets", self.text)
        self.assertNotIn("Could not locate input_vgm.cpp project item", self.text)
        self.assertNotIn("projectText.Replace", self.text)

    def test_vgm_solution_dependency_graph_is_preserved_when_available(self) -> None:
        self.assertIn(
            "$vgmSolution = Join-Path $VgmComponent 'foo_input_vgm.sln'",
            self.text,
        )
        self.assertIn(
            "$vgmBuildTarget = if (Test-Path $vgmSolution) { $vgmSolution } else { $vgmProject }",
            self.text,
        )
        self.assertIn("Run $msbuild @($vgmBuildTarget,", self.text)

    def test_outputs_are_explicit_not_recursively_discovered(self) -> None:
        self.assertNotIn("Find-ReleaseFile", self.text)
        for marker in (
            "$VgmOutDir = Join-Path $WorkRoot 'out-vgm-x64'",
            "$SpcPlayerOutDir = Join-Path $WorkRoot 'out-spcplayer-x86'",
            "$SpcComponentOutDir = Join-Path $WorkRoot 'out-spc-x64'",
            "$FooVgm = Join-Path $VgmOutDir 'foo_input_vgm.dll'",
            "$SpcPlayer = Join-Path $SpcPlayerOutDir 'spcplayer.exe'",
            "$FooSpc = Join-Path $SpcComponentOutDir 'foo_snesapu.dll'",
        ):
            self.assertIn(marker, self.text)

    def test_runtime_architectures_are_asserted_before_packaging(self) -> None:
        expected = {
            "Assert-PEMachine $OmniDll 0x8664": "Omniphony x64",
            "Assert-PEMachine $FooVgm 0x8664": "VGM x64",
            "Assert-PEMachine $SnesapuDll 0x014C": "SNESAPU x86",
            "Assert-PEMachine $SpcPlayer 0x014C": "spcplayer x86",
            "Assert-PEMachine $FooSpc 0x8664": "SPC component x64",
        }
        package = self.text.index("== 9. Package and audit")
        for marker, label in expected.items():
            with self.subTest(label=label):
                self.assertIn(marker, self.text)
                self.assertLess(self.text.index(marker), package)

    def test_patched_libvgm_integration_runs_before_vgm_component(self) -> None:
        patch = self.text.index("patches\\libvgm\\apply_source_capture.py")
        integration = self.text.index("tests\\integration\\libvgm-source")
        vgm_component = self.text.index("== 5. Materialize and build the VGM component")
        self.assertLess(patch, integration)
        self.assertLess(integration, vgm_component)
        self.assertIn("-DLIBVGM_ROOT=$Libvgm", self.text)
        self.assertIn("'--test-dir', $LibvgmSourceTestBuild", self.text)

    def test_source_commit_is_captured_and_reverified_before_manifest(self) -> None:
        capture = "$retroCommit = (& git -C $RetroRoot rev-parse HEAD).Trim().ToLowerInvariant()"
        bookend = "verify_build_source_provenance.py'), $RetroRoot, '--expected-commit', $retroCommit"
        manifest = "$manifest = [ordered]@{"
        self.assertIn(capture, self.text)
        self.assertIn(bookend, self.text)
        self.assertLess(self.text.index(capture), self.text.index("== 2. Reconstruct external"))
        self.assertLess(self.text.index(bookend), self.text.index(manifest))
        self.assertNotIn("$retroCommit = 'unversioned'", self.text)

    def test_sdk_shared_library_and_component_verifier_are_required(self) -> None:
        self.assertIn("shared\\shared-x64.lib", self.text)
        self.assertIn("verify_private_component_packages.py", self.text)
        self.assertIn("final_playback_contract_hz = 48000", self.text)

    def test_final_bundle_is_verified_after_creation(self) -> None:
        create = self.text.index("Compress-Archive -Path @($VgmComponentPackage")
        verifier = self.text.index("verify_private_component_bundle.py")
        self.assertGreater(verifier, create)
        self.assertIn("$Bundle = Join-Path $OutputRoot 'private-foobar-vgm-spc.zip'", self.text)

    def test_generated_readme_keeps_enhanced_descriptive(self) -> None:
        self.assertIn("every enhanced/Surround", self.text)
        self.assertIn("enhanced and Surround remain independent controls", self.text)
        self.assertNotIn("every Enhanced/Surround", self.text)
        self.assertNotIn("Enhanced and Surround remain independent controls", self.text)


if __name__ == "__main__":
    unittest.main()
