from __future__ import annotations

import hashlib
import importlib.util
import io
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


def component_bytes(members: dict[str, bytes]) -> bytes:
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, payload in members.items():
            archive.writestr(name, payload)
    return stream.getvalue()


class PrivateComponentBundleContractTest(unittest.TestCase):
    def make_bundle(
        self,
        path: Path,
        *,
        commit: str = "1" * 40,
        readme: str = "enhanced and Surround remain independent controls.\n",
        mutate_manifest=None,
        mutate_sums=None,
        extra_entries: dict[str, bytes] | None = None,
    ) -> None:
        vgm_members = {
            "foo_input_vgm.dll": b"vgm-dll",
            "omniphony_source.dll": b"vgm-omniphony",
        }
        spc_members = {
            "foo_snesapu.dll": b"spc-dll",
            "spcplayer.exe": b"spcplayer-exe",
            "SNESAPU.dll": b"snesapu-dll",
            "omniphony_source.dll": b"spc-omniphony",
        }
        vgm = component_bytes(vgm_members)
        spc = component_bytes(spc_members)

        manifest = {
            "built_at_utc": "2026-08-25T00:00:00.0000000Z",
            "retro_vgm_compiler_commit": commit,
            "foo_input_vgm_bootstrap": {
                "source_page": "https://example.invalid/vgm-bootstrap",
                "sha256": "2" * 64,
            },
            "foobar_sdk": {
                "release_date": "2025-03-07",
                "source": "https://example.invalid/foobar-sdk.7z",
                "sdk_project_git_blob": "3" * 40,
                "pfc_project_git_blob": "4" * 40,
            },
            "libvgm": {"commit": "5" * 40},
            "wtl": {"commit": "6" * 40},
            "spcplay": {"commit": "7" * 40},
            "omniphony": {
                "commit": "8" * 40,
                "rust_toolchain": "1.88.0",
            },
            "outputs": list(_verifier.EXPECTED_OUTPUTS),
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
            "VGM/foo_input_vgm.dll": vgm_members["foo_input_vgm.dll"],
            "VGM/omniphony_source.dll": vgm_members["omniphony_source.dll"],
            "SPC/foo_snesapu.dll": spc_members["foo_snesapu.dll"],
            "SPC/spcplayer.exe": spc_members["spcplayer.exe"],
            "SPC/SNESAPU.dll": spc_members["SNESAPU.dll"],
            "SPC/omniphony_source.dll": spc_members["omniphony_source.dll"],
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
            self.assertEqual(manifest["retro_vgm_compiler_commit"], "1" * 40)

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

    def test_rejects_wrong_output_list(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                mutate_manifest=lambda manifest: manifest.__setitem__(
                    "outputs", [_verifier.VGM_COMPONENT]
                ),
            )
            with self.assertRaisesRegex(AssertionError, "manifest outputs mismatch"):
                _verifier.verify_bundle_metadata(bundle)

    def test_rejects_runtime_copy_drift(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "bundle.zip"
            self.make_bundle(
                bundle,
                extra_entries={"VGM/foo_input_vgm.dll": b"different-vgm-dll"},
            )
            with self.assertRaisesRegex(AssertionError, "runtime copy differs"):
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
        for name in (
            "unexpected.bin",
            "nested/extra.bin",
            "VGM/extra.bin",
            "VGM/nested/extra.bin",
        ):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                bundle = Path(tmp) / "bundle.zip"
                self.make_bundle(bundle, extra_entries={name: b"x"})
                with self.assertRaises(AssertionError):
                    _verifier.verify_bundle_metadata(bundle)


if __name__ == "__main__":
    unittest.main()
