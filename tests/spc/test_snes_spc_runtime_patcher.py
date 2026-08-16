from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
STRICT = ROOT / "tools" / "spc" / "patch_snes_spc_runtime_strict.py"
SPEC = importlib.util.spec_from_file_location(
    "patch_snes_spc_runtime_strict",
    STRICT,
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {STRICT}")
strict = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = strict
SPEC.loader.exec_module(strict)


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


if __name__ == "__main__":
    unittest.main()
