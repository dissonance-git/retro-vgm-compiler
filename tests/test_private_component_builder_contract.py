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

    def test_sdk_shared_library_and_package_verifier_are_required(self) -> None:
        self.assertIn("shared\\shared-x64.lib", self.text)
        self.assertIn("verify_private_component_packages.py", self.text)
        self.assertIn("final_playback_contract_hz = 48000", self.text)


if __name__ == "__main__":
    unittest.main()
