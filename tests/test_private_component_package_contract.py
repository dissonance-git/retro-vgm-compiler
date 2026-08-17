from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERIFIER_PATH = ROOT / "tools" / "verify_private_component_packages.py"
_spec = importlib.util.spec_from_file_location("private_package_verifier", VERIFIER_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError("could not load private component package verifier")
_verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_verifier)


class PrivateComponentPackageContractTest(unittest.TestCase):
    def write_zip(self, path: Path, entries: dict[str, bytes]) -> None:
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, payload in entries.items():
                archive.writestr(name, payload)

    def test_accepts_exact_vgm_and_spc_payloads(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            vgm = root / "foo_input_vgm.private.fb2k-component"
            spc = root / "foo_snesapu.private.fb2k-component"
            self.write_zip(vgm, {name: b"x" for name in _verifier.VGM_EXPECTED})
            self.write_zip(spc, {name: b"x" for name in _verifier.SPC_EXPECTED})
            _verifier.verify_archive(vgm, _verifier.VGM_EXPECTED, "VGM")
            _verifier.verify_archive(spc, _verifier.SPC_EXPECTED, "SPC")

    def test_rejects_missing_or_extra_payload(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            self.write_zip(
                package,
                {
                    "foo_input_vgm.dll": b"x",
                    "unexpected.dll": b"x",
                },
            )
            with self.assertRaisesRegex(AssertionError, "payload mismatch"):
                _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_nested_or_traversal_entries(self) -> None:
        for bad_name in ("nested/foo_input_vgm.dll", "../foo_input_vgm.dll"):
            with self.subTest(bad_name=bad_name), tempfile.TemporaryDirectory() as tmp:
                package = Path(tmp) / "bad.fb2k-component"
                self.write_zip(
                    package,
                    {
                        bad_name: b"x",
                        "omniphony_source.dll": b"x",
                    },
                )
                with self.assertRaisesRegex(AssertionError, "unsafe/nested"):
                    _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_zero_byte_runtime(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            entries = {name: b"x" for name in _verifier.VGM_EXPECTED}
            entries["omniphony_source.dll"] = b""
            self.write_zip(package, entries)
            with self.assertRaisesRegex(AssertionError, "zero-byte"):
                _verifier.verify_archive(package, _verifier.VGM_EXPECTED, "VGM")

    def test_rejects_case_colliding_entries(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "bad.fb2k-component"
            with zipfile.ZipFile(package, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                archive.writestr("foo_snesapu.dll", b"x")
                archive.writestr("FOO_SNESAPU.DLL", b"x")
                archive.writestr("spcplayer.exe", b"x")
                archive.writestr("SNESAPU.dll", b"x")
                archive.writestr("omniphony_source.dll", b"x")
            with self.assertRaisesRegex(AssertionError, "duplicate"):
                _verifier.verify_archive(package, _verifier.SPC_EXPECTED, "SPC")


if __name__ == "__main__":
    unittest.main()
