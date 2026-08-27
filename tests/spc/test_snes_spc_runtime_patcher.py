from __future__ import annotations

import importlib.util
import pathlib
import sys
import tempfile
import unittest
from unittest import mock


ROOT = pathlib.Path(__file__).resolve().parents[2]
STRICT = ROOT / "tools" / "spc" / "patch_snes_spc_runtime_strict.py"
FORENSIC = ROOT / "tools" / "spc" / "patch_snes_spc_forensic.py"
SPATIAL_REGISTER = ROOT / "tools" / "spc" / "patch_snes_spc_spatial_register_observer.py"
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
spatial_register = load_module("patch_snes_spc_spatial_register_observer", SPATIAL_REGISTER)
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


def write_spatial_register_fixture(root: pathlib.Path) -> None:
    source = root / "snes_spc"
    source.mkdir(parents=True)
    (source / "SNES_SPC.cpp").write_text(
        spatial_register.INCLUDE_OLD + spatial_register.DSP_WRITE_OLD,
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

    def test_forensic_composition_applies_native_spatial_before_runtime_state(self) -> None:
        calls: list[str] = []
        fake_root = pathlib.Path("/synthetic/snes-spc")

        with (
            mock.patch.object(
                forensic.native_spatial,
                "patch",
                side_effect=lambda root: calls.append("native_spatial"),
            ),
            mock.patch.object(
                forensic.strict.patcher,
                "patch",
                side_effect=lambda root: calls.append("runtime"),
            ),
            mock.patch.object(
                forensic.strict,
                "strict_replace_once",
                side_effect=lambda *args, **kwargs: calls.append("ordering"),
            ),
            mock.patch.object(
                forensic.spatial_register,
                "patch",
                side_effect=lambda root: calls.append("spatial_register"),
            ),
        ):
            forensic.patch(fake_root)

        self.assertEqual(
            calls,
            ["native_spatial", "runtime", "ordering", "spatial_register"],
        )

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
            self.assertIn("RETRO_VGM_SPC_FORENSIC_ORDERING", patched)
            self.assertIn("if ( time > m.dsp_time )", patched)
            self.assertIn("RUN_DSP( time, 0 );", patched)
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
            partial = forensic.ORDERING_NEW.replace("RUN_DSP( time, 0 );", "RUN_DSP( time, 1 );")
            path.write_text(partial, encoding="utf-8")
            with self.assertRaises(RuntimeError):
                strict.strict_replace_once(
                    path,
                    forensic.ORDERING_OLD,
                    forensic.ORDERING_NEW,
                    "forensic ordering fixture",
                )

    def test_spatial_register_patch_is_exact_and_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            write_spatial_register_fixture(root)
            spatial_register.patch(root)
            path = root / "snes_spc" / "SNES_SPC.cpp"
            patched = path.read_text(encoding="utf-8")
            self.assertIn("snes_spc_spatial_register_hook_bridge.h", patched)
            self.assertIn("RETRO_VGM_COMPILER_SNES_SPC_SPATIAL_REGISTER_OBSERVER", patched)
            self.assertIn("spatial_reg <= 0x71", patched)
            self.assertIn("(spatial_reg & 0x0F) <= 1", patched)
            self.assertIn("spatial_reg == SPC_DSP::r_eon", patched)
            self.assertIn("for ( int voice = 0; voice < SPC_DSP::voice_count; ++voice )", patched)
            self.assertIn("dsp.read( base + SPC_DSP::v_voll )", patched)
            self.assertIn("dsp.read( base + SPC_DSP::v_volr )", patched)
            self.assertIn("dsp.read( SPC_DSP::r_eon )", patched)
            spatial_register.patch(root)
            self.assertEqual(path.read_text(encoding="utf-8"), patched)

    def test_spatial_register_patch_rejects_partial_edit(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            write_spatial_register_fixture(root)
            spatial_register.patch(root)
            path = root / "snes_spc" / "SNES_SPC.cpp"
            path.write_text(
                path.read_text(encoding="utf-8").replace(
                    "spatial_reg == SPC_DSP::r_eon",
                    "spatial_reg == 0x00",
                    1,
                ),
                encoding="utf-8",
            )
            with self.assertRaises(RuntimeError):
                spatial_register.patch(root)

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
            self.assertIn("native_dry_source [v - m.voices] = (sample_t) m.t_output;", dsp_cpp)
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

    def test_forensic_cmake_uses_complete_spatial_contract(self) -> None:
        cmake = FORENSIC_CMAKE.read_text(encoding="utf-8")
        self.assertIn("tools/spc/patch_snes_spc_forensic.py", cmake)
        self.assertIn("SPC_LESS_ACCURATE=0", cmake)
        self.assertIn("SPC_MORE_ACCURACY=0", cmake)
        self.assertIn("RETRO_VGM_SPC_FORENSIC_ORDERING=1", cmake)
        self.assertNotIn("SPC_MORE_ACCURACY=1", cmake)
        self.assertIn(
            "retro-vgm-compiler:snes-spc-runtime-hooks-v1-ordering-v1-native-spatial-v1-route-events-v1",
            cmake,
        )
        self.assertIn("snes_spc_native_spatial_observer_test", cmake)
        self.assertIn("spc_native_routed_source_projection_test", cmake)
        self.assertIn("spc_spatial_governor_trace", cmake)


if __name__ == "__main__":
    unittest.main()
