from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
STRICT = ROOT / "tools" / "spc" / "patch_snes_spc_runtime_strict.py"
FORENSIC = ROOT / "tools" / "spc" / "patch_snes_spc_forensic.py"
NATIVE_SPATIAL = ROOT / "tools" / "spc" / "patch_snes_spc_native_spatial_observer.py"
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
native_spatial = load_module("patch_snes_spc_native_spatial_observer", NATIVE_SPATIAL)


def write_native_spatial_fixture(root: pathlib.Path) -> None:
    source = root / "snes_spc"
    source.mkdir(parents=True)
    (source / "SPC_DSP.h").write_text(
        native_spatial.DSP_API_OLD
        + native_spatial.DSP_STATE_OLD
        + native_spatial.DSP_INLINE_OLD,
        encoding="utf-8",
    )
    (source / "SNES_SPC.h").write_text(
        native_spatial.SNES_API_OLD + native_spatial.SNES_INLINE_OLD,
        encoding="utf-8",
    )
    (source / "SPC_DSP.cpp").write_text(
        native_spatial.DRY_TAP_OLD
        + native_spatial.ECHO_LEFT_OLD
        + native_spatial.ECHO_RIGHT_OLD,
        encoding="utf-8",
    )


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

    def test_native_spatial_patch_is_exact_and_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            write_native_spatial_fixture(root)

            native_spatial.patch(root)
            dsp_header = (root / "snes_spc" / "SPC_DSP.h").read_text(encoding="utf-8")
            snes_header = (root / "snes_spc" / "SNES_SPC.h").read_text(encoding="utf-8")
            dsp_cpp = (root / "snes_spc" / "SPC_DSP.cpp").read_text(encoding="utf-8")

            self.assertIn("native_spatial_observer_t", dsp_header)
            self.assertIn("native_dry_source [voice_count]", dsp_header)
            self.assertIn("set_native_spatial_observer", snes_header)
            self.assertIn(
                "native_dry_source [v - m.voices] = (sample_t) m.t_output;",
                dsp_cpp,
            )
            self.assertIn("m.t_echo_in [0]", dsp_cpp)
            self.assertIn("REG(evoll)", dsp_cpp)
            self.assertIn("m.t_echo_in [1]", dsp_cpp)
            self.assertIn("REG(evoll + 0x10)", dsp_cpp)
            self.assertIn("m.t_main_out [0] = echo_output( 0 );", dsp_cpp)
            self.assertIn("int r = echo_output( 1 );", dsp_cpp)
            self.assertIn("native_spatial_observer(", dsp_cpp)
            self.assertIn("voice_count,", dsp_cpp)
            self.assertIn("native_echo_left,", dsp_cpp)
            self.assertIn("native_echo_right );", dsp_cpp)

            first = {
                path.name: path.read_text(encoding="utf-8")
                for path in (root / "snes_spc").iterdir()
            }
            native_spatial.patch(root)
            second = {
                path.name: path.read_text(encoding="utf-8")
                for path in (root / "snes_spc").iterdir()
            }
            self.assertEqual(second, first)

    def test_native_spatial_patch_rejects_partial_edit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            write_native_spatial_fixture(root)
            native_spatial.patch(root)

            dsp_cpp_path = root / "snes_spc" / "SPC_DSP.cpp"
            dsp_cpp_path.write_text(
                dsp_cpp_path.read_text(encoding="utf-8").replace(
                    "native_echo_left,\n",
                    "native_echo_left + 1,\n",
                    1,
                ),
                encoding="utf-8",
            )
            with self.assertRaises(RuntimeError):
                native_spatial.patch(root)

    def test_forensic_cmake_uses_deterministic_native_spatial_contract(self) -> None:
        cmake = FORENSIC_CMAKE.read_text(encoding="utf-8")

        self.assertIn("tools/spc/patch_snes_spc_forensic.py", cmake)
        self.assertIn("SPC_LESS_ACCURATE=0", cmake)
        self.assertIn("SPC_MORE_ACCURACY=0", cmake)
        self.assertIn("RETRO_VGM_SPC_FORENSIC_ORDERING=1", cmake)
        self.assertNotIn("SPC_MORE_ACCURACY=1", cmake)
        self.assertIn(
            "retro-vgm-compiler:snes-spc-runtime-hooks-v1-ordering-v1-native-spatial-v1",
            cmake,
        )
        self.assertIn("snes_spc_native_spatial_observer_test", cmake)


if __name__ == "__main__":
    unittest.main()
