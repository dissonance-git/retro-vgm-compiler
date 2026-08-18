from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERIFIER_PATH = ROOT / "tools" / "verify_private_component_bundle.py"
_spec = importlib.util.spec_from_file_location("private_bundle_verifier", VERIFIER_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError("could not load private component bundle verifier")
_verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_verifier)


class PrivateComponentBundleContractTest(unittest.TestCase):
    def make_bundle(
        self,
        path: Path,
        *,
        vgm: bytes = b"vgm-component",
        spc: bytes = b"spc-component",
        commit: str = "1" * 40,
        readme: str = "enhanced and Surround remain independent controls.\n",
        mutate_manifest=None,
        mutate_sums=None,
        extra_entries: dict[str, bytes] | None = None,
    ) -> None:
        manifest = {
            "retro_vgm_compiler": commit,
            "final_playback_contract_hz": 48000,
            "binary_architecture": dict(_verifier.EXPECTED_ARCHITECTURE),
            "packages": list(_verifier.COMPONENTS),
        }
        if mutate_manifest is not None:
            mutate_manifest(manifest)

        sums = {
            _verifier.VGM_COMPONENT: hashlib.sha256(vgm).hexdigest(),
            _verifier.SPC_COMPONENT: hashlib.sha256(spc).hexdigest(),
        }
        if mutate_sums is not None:
            mutate_sums(sums)
        sums_text = "".join(f"{digest}  {name}\n" for name, digest in sums.items())

        entries = {
            _verifier.VGM_COMPONENT: vgm,
            _verifier.SPC_COMPONENT: spc,
            "build-manifest.json": json.dumps(manifest).encode("utf-8"),
            "SHA256SUMS.txt": sums_text.encode("ascii"),
            "README.txt": readme.encode("utf-8"),
        }
        if extra_entries:
            entries.update(extra_entries)

        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, payload in entries.items():
                archive.writestr(name, payload)

    def test_accepts_consistent_bundle_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(bundle)
            manifest = _verifier.verify_bundle_metadata(bundle)
            self.assertEqual(manifest["retro_vgm_compiler"], "1" * 40)

    def test_rejects_hash_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                mutate_sums=lambda sums: sums.__setitem__(
                    _verifier.VGM_COMPONENT, "0" * 64
                ),
            )
            with self.assertRaisesRegex(AssertionError, "SHA256SUMS mismatch"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_unversioned_source(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(bundle, commit="unversioned")
            with self.assertRaisesRegex(AssertionError, "40-hex source commit"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_wrong_architecture_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                mutate_manifest=lambda manifest: manifest["binary_architecture"].__setitem__(
                    "spcplayer", "x64"
                ),
            )
            with self.assertRaisesRegex(AssertionError, "binary_architecture mismatch"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_wrong_package_list(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                mutate_manifest=lambda manifest: manifest.__setitem__(
                    "packages", [_verifier.VGM_COMPONENT]
                ),
            )
            with self.assertRaisesRegex(AssertionError, "manifest packages"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_proper_name_enhanced_in_readme(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                readme="Enhanced and Surround remain independent controls.\n",
            )
            with self.assertRaisesRegex(AssertionError, "lowercase descriptive"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_extra_or_nested_bundle_file(self) -> None:
        for name in ("unexpected.bin", "nested/extra.bin"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                bundle = Path(tmp) / "bundle.zip"
                self.make_bundle(bundle, extra_entries={name: b"x"})
                with self.assertRaises(AssertionError):
                    _verifier.verify_bundle_metadata(bundle)


if __name__ == "__main__":
    unittest.main()
