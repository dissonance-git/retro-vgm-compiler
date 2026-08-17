from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
LOADER = ROOT / "model" / "omniphony_dynamic_backend_loader.h"


class OmniphonySiblingLoaderContractTest(unittest.TestCase):
    def test_windows_lookup_is_anchored_to_embedding_component(self) -> None:
        text = LOADER.read_text(encoding="utf-8")
        required = (
            "omniphony_loader_module_anchor",
            "GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS",
            "GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT",
            "GetModuleFileNameW",
            'constexpr wchar_t dll_name[] = L"omniphony_source.dll";',
        )
        for marker in required:
            with self.subTest(marker=marker):
                self.assertIn(marker, text)

    def test_open_default_prefers_sibling_before_search_fallback(self) -> None:
        text = LOADER.read_text(encoding="utf-8")
        function_start = text.index(
            "bool open_default(const omniphony_source_config_transport& config) noexcept"
        )
        function_end = text.index("\n    bool open(\n", function_start)
        body = text[function_start:function_end]

        sibling = "sibling_library_path(sibling.data(), sibling.size())"
        fallback = 'return open(L"omniphony_source.dll", config);'
        self.assertIn(sibling, body)
        self.assertIn(fallback, body)
        self.assertLess(body.index(sibling), body.index(fallback))

    def test_loader_still_checks_runtime_abi_after_loading(self) -> None:
        text = LOADER.read_text(encoding="utf-8")
        required = (
            'resolve<omniphony_source_abi_version_fn>("omniphony_source_abi_major")',
            'resolve<omniphony_source_abi_version_fn>("omniphony_source_abi_minor")',
            "abi_major_() != omniphony_source_abi_major_required",
            "abi_minor_() < omniphony_source_abi_minor_required",
        )
        for marker in required:
            with self.subTest(marker=marker):
                self.assertIn(marker, text)


if __name__ == "__main__":
    unittest.main()
