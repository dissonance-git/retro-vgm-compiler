from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
STRICT = ROOT / "tools" / "spc" / "patch_snes_spc_runtime_strict.py"
FORENSIC = ROOT / "tools" / "spc" / "patch_snes_spc_forensic.py"
FORENSIC_CMAKE = ROOT / "tools" / "spc" / "forensic" / "CMakeLists.txt"


def load_module(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


strict = load_module("patch_snes_spc_runtime_strict", STRICT)
forensic = load_module("patch_snes_spc_forensic", FORENSIC)


class SnesSpcRuntimePatcherTest(unittest.TestCase):
    def test_pristine_sentinel_is_replaced_once_and_is_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "upstream.cpp"
            old = "alpha\noriginal sentinel\nomega\n"
            new = "alpha\npatched replacement\nomega\n"
            path.write_text(old, encoding="utf-8")

            strict.strict_replace_once(path, old, new, "fixture")
            self.assertEqual(path.read_text(encoding="utf-8"), new)

            # A second application must recognize the exact replacement rather
            # than attempting to modify it again.
            strict.strict_replace_once(path, old, new, "fixture")
            self.assertEqual(path.read_text(encoding="utf-8"), new)

    def test_missing_or_foreign_sentinel_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "upstream.cpp"
            path.write_text(
                "RETRO_VGM_COMPILER_SNES_SPC_RUNTIME_HOOKS\nforeign partial edit\n",
                encoding="utf-8",
            )

            with self.assertRaises(RuntimeError):
                strict.strict_replace_once(
                    path,
                    "expected pristine text\n",
                    "expected patched text\n",
                    "fixture",
                )

    def test_duplicate_pristine_sentinel_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "upstream.cpp"
            old = "sentinel\n"
            new = "replacement\n"
            path.write_text(old + old, encoding="utf-8")

            with self.assertRaises(RuntimeError):
                strict.strict_replace_once(path, old, new, "fixture")

    def test_duplicate_patched_replacement_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "upstream.cpp"
            old = "sentinel\n"
            new = "replacement\n"
            path.write_text(new + new, encoding="utf-8")

            with self.assertRaises(RuntimeError):
                strict.strict_replace_once(path, old, new, "fixture")

    def test_forensic_ordering_patch_is_narrow_and_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "SNES_SPC.cpp"
            path.write_text(forensic.ORDERING_OLD, encoding="utf-8")

            strict.strict_replace_once(
                path,
                forensic.ORDERING_OLD,
                forensic.ORDERING_NEW,
                "forensic ordering fixture",
            )
            patched = path.read_text(encoding="utf-8")

            self.assertIn(
                "RETRO_VGM_SPC_FORENSIC_ORDERING",
                patched,
            )
            self.assertIn("if ( time > m.dsp_time )", patched)
            self.assertIn("RUN_DSP( time, 0 );", patched)

            # The upstream broad validation branch remains present but separate.
            # The forensic branch must not emulate SPC_MORE_ACCURACY by replacing
            # or deleting that branch.
            self.assertIn("#elif SPC_MORE_ACCURACY", patched)
            self.assertIn("RUN_DSP( time, max_reg_time );", patched)

            strict.strict_replace_once(
                path,
                forensic.ORDERING_OLD,
                forensic.ORDERING_NEW,
                "forensic ordering fixture",
            )
            self.assertEqual(path.read_text(encoding="utf-8"), patched)

    def test_forensic_ordering_rejects_partial_edit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = pathlib.Path(temporary) / "SNES_SPC.cpp"
            partial = forensic.ORDERING_NEW.replace(
                "RUN_DSP( time, 0 );",
                "RUN_DSP( time, 1 );",
            )
            path.write_text(partial, encoding="utf-8")

            with self.assertRaises(RuntimeError):
                strict.strict_replace_once(
                    path,
                    forensic.ORDERING_OLD,
                    forensic.ORDERING_NEW,
                    "forensic ordering fixture",
                )

    def test_forensic_cmake_uses_only_deterministic_ordering_mode(self) -> None:
        cmake = FORENSIC_CMAKE.read_text(encoding="utf-8")

        self.assertIn("tools/spc/patch_snes_spc_forensic.py", cmake)
        self.assertIn("SPC_LESS_ACCURATE=0", cmake)
        self.assertIn("SPC_MORE_ACCURACY=0", cmake)
        self.assertIn("RETRO_VGM_SPC_FORENSIC_ORDERING=1", cmake)
        self.assertNotIn("SPC_MORE_ACCURACY=1", cmake)
        self.assertIn(
            "retro-vgm-compiler:snes-spc-runtime-hooks-v1-ordering-v1",
            cmake,
        )


if __name__ == "__main__":
    unittest.main()
