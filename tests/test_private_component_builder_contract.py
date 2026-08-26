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
        self.assertIn("$vgmSolution = Join-Path $VgmComponent 'foo_input_vgm.sln'", self.text)
        self.assertIn("$vgmBuildTarget = if (Test-Path $vgmSolution) { $vgmSolution } else { $vgmProject }", self.text)
        self.assertIn("Run $msbuild @($vgmBuildTarget,", self.text)

    def test_outputs_are_explicit_not_recursively_discovered(self) -> None:
        self.assertNotIn("Find-ReleaseFile", self.text)
        for marker in (
            "$VgmOutDir = Join-Path $WorkRoot 'out-vgm-x64'",
            "$SpcPlayerOutDir = Join-Path $WorkRoot 'out-spcplayer-x86'",
            "$SpcComponentOutDir = Join-Path $WorkRoot 'out-spc-x64'",
            "$FooVgm = Join-Path $VgmOutDir 'foo_input_vgm.dll'",
            "$SpcPlayerExe = Join-Path $SpcPlayerOutDir 'spcplayer.exe'",
            "$FooSpc = Join-Path $SpcComponentOutDir 'foo_snesapu.dll'",
        ):
            self.assertIn(marker, self.text)

    def test_spcplayer_consumes_the_fresh_patched_snesapu_build(self) -> None:
        self.assertIn("$spcPlayerIncludeArg = '/p:SNESAPUIncludeDir=' + $SnesapuSource", self.text)
        self.assertIn("$spcPlayerLibArg = '/p:SNESAPULibDir=' + $SnesapuLibDir", self.text)

    def test_runtime_architectures_are_asserted_before_packaging(self) -> None:
        expected = {
            "Assert-PEMachine $FooVgm 0x8664": "VGM x64",
            "Assert-PEMachine $SnesapuDll 0x014C": "SNESAPU x86",
            "Assert-PEMachine $SpcPlayerExe 0x014C": "spcplayer x86",
            "Assert-PEMachine $FooSpc 0x8664": "SPC component x64",
        }
        package = self.text.index("== 9. Package installable private components and final bundle ==")
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

    def test_vgm_zlib_compatibility_uses_the_pinned_built_library(self) -> None:
        self.assertIn("$builtZlib = Join-Path $Libvgm 'libs\\lib\\zlib64.lib'", self.text)
        self.assertIn("Copy-Item $builtZlib (Join-Path $zlibCompatRelease 'zs.lib') -Force", self.text)

    def test_source_commit_is_captured_and_written_to_manifest(self) -> None:
        capture = "$sourceCommit = (& git -C $RepoRoot rev-parse HEAD).Trim().ToLowerInvariant()"
        self.assertIn(capture, self.text)
        self.assertIn("vgm_compiler_commit = $sourceCommit", self.text)
        self.assertNotIn("unversioned", self.text)

    def test_bootstrap_manifest_points_at_current_canonical_input(self) -> None:
        self.assertIn("$VgmBootstrapSource = 'repository:imports/bootstrap/foo_input_vgm-0.31.base64-parts'", self.text)
        self.assertIn("$VgmBootstrapSha256 = 'e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1'", self.text)

    def test_input_packages_do_not_build_decoder_side_omniphony(self) -> None:
        for retired in ("$OmniphonyCommit", "$RustToolchain", "source_ffi", "omniphony_source.dll", "verify_omniphony_runtime_abi.py"):
            self.assertNotIn(retired, self.text)
        self.assertIn("verify_private_component_packages.py", self.text)

    def test_final_bundle_is_verified_after_creation(self) -> None:
        bundle = "$Bundle = Join-Path $OutputRoot 'private-foobar-vgm-spc.zip'"
        create = "try { Run '7z' @('a', '-tzip', '-mx=9', $Bundle, '*') }"
        verifier = "verify_private_component_bundle.py"
        self.assertIn(bundle, self.text)
        self.assertIn(create, self.text)
        self.assertIn(verifier, self.text)

    def test_generated_readme_describes_7_1_output_boundary(self) -> None:
        self.assertIn("Surround emits a source-derived standard 7.1 Genesis bed.", self.text)
        self.assertIn("Surround emits a source-derived standard 7.1 SPC bed.", self.text)
        self.assertIn("Use the Omniphony foobar output component separately", self.text)
        self.assertIn("enhanced is disabled for this milestone.", self.text)


if __name__ == "__main__":
    unittest.main()
