from __future__ import annotations

import hashlib
import importlib.util
import io
from pathlib import Path, PurePosixPath
import re
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "tools" / "build_private_foobar_components.ps1"
RECONSTRUCTOR = ROOT / "tools" / "reconstruct_vgm031_bootstrap.py"
MATERIALIZER = ROOT / "tools" / "materialize_foo_input_vgm.py"
RUNTIME_VERIFIER = ROOT / "tools" / "verify_omniphony_runtime_abi.py"
PACKAGE_VERIFIER = ROOT / "tools" / "verify_private_component_packages.py"
EXPECTED_VGM_BOOTSTRAP_VERSION = "0.31"
EXPECTED_VGM_BOOTSTRAP_FILES = 41
EXPECTED_VGM_BOOTSTRAP_ARCHIVE_SHA256 = (
    "e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1"
)
EXPECTED_VGM_BOOTSTRAP_TREE_SHA256 = (
    "36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc"
)


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def vgm_bootstrap_source_identity(archive_bytes: bytes) -> tuple[int, str, str]:
    with zipfile.ZipFile(io.BytesIO(archive_bytes)) as archive:
        names = [name for name in archive.namelist() if not name.endswith("/")]
        markers = [
            name
            for name in names
            if name.endswith("foo_input_vgm/src/my_component_client.cpp")
        ]
        if len(markers) != 1:
            raise AssertionError(
                "foo_input_vgm bootstrap must contain exactly one component version marker: "
                f"{markers}"
            )
        marker = PurePosixPath(markers[0])
        root = marker.parents[1].as_posix().rstrip("/") + "/"
        entries: list[tuple[str, str]] = []
        for name in sorted(candidate for candidate in names if candidate.startswith(root)):
            relative = name[len(root) :]
            if not relative:
                continue
            digest = hashlib.sha256(archive.read(name)).hexdigest()
            entries.append((relative, digest))
        manifest = "".join(
            f"{digest}  {relative}\n" for relative, digest in entries
        ).encode("utf-8")
        tree_sha256 = hashlib.sha256(manifest).hexdigest()
        version_text = archive.read(markers[0]).decode("utf-8-sig")
    return len(entries), tree_sha256, version_text


class OmniphonyPrivateBuildContractTest(unittest.TestCase):
    def test_private_build_pins_and_exercises_omniphony_source_renderer(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        match = re.search(r"\$OmniphonyCommit = '([0-9a-f]{40})'", text)
        self.assertIsNotNone(match)
        self.assertIn(
            "Clone-Pin 'https://github.com/dissonance-git/Omniphony-Headphones.git'",
            text,
        )
        self.assertIn("'test', '-p', 'source_ffi', '--lib'", text)
        self.assertIn("'test', '-p', 'source_ffi', '--test', 'abi_layout'", text)
        self.assertIn("'build', '--profile', 'release-deploy', '-p', 'source_ffi'", text)
        self.assertIn("verify_omniphony_runtime_abi.py", text)

    def test_repository_transport_reconstructs_exact_031_source_tree(self) -> None:
        reconstructor = load_module(RECONSTRUCTOR, "vgm031_reconstructor_for_test")
        archive = reconstructor.load_transport()
        self.assertEqual(
            hashlib.sha256(archive).hexdigest(),
            EXPECTED_VGM_BOOTSTRAP_ARCHIVE_SHA256,
        )
        file_count, tree_sha256, version_text = vgm_bootstrap_source_identity(archive)
        self.assertEqual(file_count, EXPECTED_VGM_BOOTSTRAP_FILES)
        self.assertEqual(tree_sha256, EXPECTED_VGM_BOOTSTRAP_TREE_SHA256)
        self.assertIn(f'"{EXPECTED_VGM_BOOTSTRAP_VERSION}"', version_text)

    def test_reconstructor_owns_current_offline_transport(self) -> None:
        reconstructor = load_module(RECONSTRUCTOR, "vgm031_transport_contract_for_test")
        self.assertEqual(
            reconstructor.SOURCE_DIR,
            ROOT / "imports" / "bootstrap" / "foo_input_vgm-0.31.base64-parts",
        )
        self.assertEqual(
            reconstructor.TARGET,
            ROOT / "imports" / "foo_input_vgm-0.31.zip",
        )
        self.assertEqual(
            reconstructor.EXPECTED_ARCHIVE_SHA256,
            EXPECTED_VGM_BOOTSTRAP_ARCHIVE_SHA256,
        )
        self.assertEqual(reconstructor.EXPECTED_SOURCE_FILES, EXPECTED_VGM_BOOTSTRAP_FILES)
        self.assertGreater(len(reconstructor.PARTS), 1)

    def test_vgm_materializer_owns_canonical_reconstruction(self) -> None:
        text = MATERIALIZER.read_text(encoding="utf-8")
        self.assertIn("import os", text)
        self.assertIn("def canonical_archive(repo: Path) -> Path:", text)
        self.assertIn('reconstructor = repo / "tools" / "reconstruct_vgm031_bootstrap.py"', text)
        self.assertIn('archive = repo / "imports" / "foo_input_vgm-0.31.zip"', text)
        self.assertIn('os.environ.get("VGM_COMPILER_BOOTSTRAP_ARCHIVE")', text)

    def test_build_executes_runtime_contract_verifier_before_packaging(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        verifier_call = "tools\\verify_omniphony_runtime_abi.py"
        package_stage = "== 4. Patch and build pinned libvgm"
        self.assertIn(verifier_call, text)
        self.assertIn(package_stage, text)
        self.assertLess(text.index(verifier_call), text.index(package_stage))

    def test_runtime_verifier_requires_mix_budget_contract(self) -> None:
        verifier = load_module(RUNTIME_VERIFIER, "omniphony_runtime_contract_for_test")
        self.assertEqual(verifier.EXPECTED_ABI_MAJOR, 0)
        self.assertEqual(verifier.MINIMUM_ABI_MINOR, 4)
        self.assertIn("omniphony_source_set_mix_budget", verifier.REQUIRED_SYMBOLS)
        self.assertIn("omniphony_source_process_events_f32", verifier.REQUIRED_SYMBOLS)

    def test_package_verifier_requires_same_mix_budget_export(self) -> None:
        verifier = load_module(PACKAGE_VERIFIER, "private_package_contract_for_test")
        self.assertIn(
            "omniphony_source_set_mix_budget",
            verifier.OMNIPHONY_REQUIRED_EXPORTS,
        )
        self.assertIn(
            "omniphony_source_process_events_f32",
            verifier.OMNIPHONY_REQUIRED_EXPORTS,
        )

    def test_generated_readme_names_current_surround_controls(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        self.assertIn(
            "Surround enables Omniphony source-aware Genesis rendering.", text
        )
        self.assertIn(
            "Surround enables Omniphony source-aware SPC rendering.", text
        )
        self.assertIn("enhanced is independent.", text)


if __name__ == "__main__":
    unittest.main()
