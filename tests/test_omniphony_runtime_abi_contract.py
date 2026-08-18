from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT / "tools" / "verify_omniphony_runtime_abi.py"
MODEL_PATH = ROOT / "model" / "omniphony_source_transport.h"

_spec = importlib.util.spec_from_file_location("omniphony_runtime_abi", TOOL_PATH)
if _spec is None or _spec.loader is None:
    raise RuntimeError("could not load Omniphony runtime ABI verifier")
_verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_verifier)


class FakeFunction:
    def __init__(self, value: int) -> None:
        self.value = value
        self.argtypes = None
        self.restype = None

    def __call__(self) -> int:
        return self.value


class FakeApi:
    def __init__(self, major: int = 0, minor: int = 3) -> None:
        self.omniphony_source_abi_major = FakeFunction(major)
        self.omniphony_source_abi_minor = FakeFunction(minor)
        self.omniphony_source_create = object()
        self.omniphony_source_destroy = object()
        self.omniphony_source_reset = object()
        self.omniphony_source_set_mix_budget = object()
        self.omniphony_source_process_events_f32 = object()


class OmniphonyRuntimeAbiContractTest(unittest.TestCase):
    def test_verifier_matches_compiler_transport_version(self) -> None:
        text = MODEL_PATH.read_text(encoding="utf-8")
        major = re.search(
            r"omniphony_source_abi_major_required\s*=\s*(\d+)(?:u)?", text
        )
        minor = re.search(
            r"omniphony_source_abi_minor_required\s*=\s*(\d+)(?:u)?", text
        )
        self.assertIsNotNone(major)
        self.assertIsNotNone(minor)
        self.assertEqual(int(major.group(1)), _verifier.EXPECTED_ABI_MAJOR)
        self.assertEqual(int(minor.group(1)), _verifier.MINIMUM_ABI_MINOR)

    def test_accepts_exact_required_version(self) -> None:
        self.assertEqual(_verifier.verify_api(FakeApi(0, 4)), (0, 4))

    def test_accepts_newer_minor_with_same_major(self) -> None:
        self.assertEqual(_verifier.verify_api(FakeApi(0, 7)), (0, 7))

    def test_rejects_wrong_major(self) -> None:
        with self.assertRaisesRegex(AssertionError, "ABI mismatch"):
            _verifier.verify_api(FakeApi(1, 4))

    def test_rejects_older_minor(self) -> None:
        with self.assertRaisesRegex(AssertionError, "ABI mismatch"):
            _verifier.verify_api(FakeApi(0, 3))

    def test_rejects_missing_loader_symbol(self) -> None:
        api = FakeApi()
        del api.omniphony_source_reset
        with self.assertRaisesRegex(AssertionError, "missing required symbols"):
            _verifier.verify_api(api)

    def test_sets_zero_argument_u32_signatures(self) -> None:
        api = FakeApi()
        _verifier.verify_api(api)
        self.assertEqual(api.omniphony_source_abi_major.argtypes, [])
        self.assertEqual(api.omniphony_source_abi_minor.argtypes, [])
        self.assertIs(api.omniphony_source_abi_major.restype, _verifier.ctypes.c_uint32)
        self.assertIs(api.omniphony_source_abi_minor.restype, _verifier.ctypes.c_uint32)


if __name__ == "__main__":
    unittest.main()
