from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path, PurePosixPath
import re
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "tools" / "build_private_foobar_components.ps1"
RUNTIME_VERIFIER = ROOT / "tools" / "verify_omniphony_runtime_abi.py"
PACKAGE_VERIFIER = ROOT / "tools" / "verify_private_component_packages.py"
VGM_BOOTSTRAP = ROOT / "imports" / "foo_input_vgm-0.31.zip"
EXPECTED_OMNIPHONY_COMMIT = "819668d1366710d663ae9c810edbcf9b7e923e19"
RETIRED_INCOMPATIBLE_COMMIT = "0fabccb165e6d957cefecc6eeb1264467e7406a4"
EXPECTED_VGM_BOOTSTRAP_VERSION = "0.31"
EXPECTED_VGM_BOOTSTRAP_FILES = 41
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


def vgm_bootstrap_source_identity(path: Path) -> tuple[int, str, str]:
    with zipfile.ZipFile(path) as archive:
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
    def test_private_build_pins_the_source_renderer_contract_used_by_clients(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        match = re.search(r"\$OmniphonyCommit = '([0-9a-f]{40})'", text)
        self.assertIsNotNone(match)
        assert match is not None
        self.assertEqual(match.group(1), EXPECTED_OMNIPHONY_COMMIT)
        self.assertNotIn(RETIRED_INCOMPATIBLE_COMMIT, text)

    def test_vgm_bootstrap_is_exact_supplied_031_source_tree(self) -> None:
        self.assertTrue(VGM_BOOTSTRAP.is_file(), VGM_BOOTSTRAP)
        file_count, tree_sha256, version_text = vgm_bootstrap_source_identity(VGM_BOOTSTRAP)
        self.assertEqual(file_count, EXPECTED_VGM_BOOTSTRAP_FILES)
        self.assertEqual(tree_sha256, EXPECTED_VGM_BOOTSTRAP_TREE_SHA256)
        self.assertIn(f'"{EXPECTED_VGM_BOOTSTRAP_VERSION}"', version_text)

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

    def test_generated_readme_names_the_existing_surround_control(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        self.assertIn("enhanced/Surround", text)
        self.assertIn("enhanced and Surround remain independent controls", text)
        self.assertNotIn("enhanced/Spatial", text)
        self.assertNotIn("enhanced and Spatial remain independent controls", text)


if __name__ == "__main__":
    unittest.main()
