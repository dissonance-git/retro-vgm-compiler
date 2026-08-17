from __future__ import annotations

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
WRAPPER = ROOT / "tools" / "run_vgmspc_retirement_gate.ps1"
WORKFLOW = ROOT / ".github" / "workflows" / "vgmspc-retirement-gate.yml"
DOC = ROOT / "imports" / "vgmspc" / "RETIREMENT_GATE.md"


class VgmspcRetirementGateContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.wrapper = WRAPPER.read_text(encoding="utf-8")
        cls.workflow = WORKFLOW.read_text(encoding="utf-8")
        cls.doc = DOC.read_text(encoding="utf-8")

    def test_wrapper_orders_canonical_build_smoke_and_final_bundle_audit(self) -> None:
        builder = "build_private_foobar_components.ps1"
        smoke_source = "snesapu_provider_export_smoke.cpp"
        smoke_run = '"$SmokeExe" "$PackagedSnesapu"'
        bundle_audit = "verify_private_component_bundle.py"
        for marker in (builder, smoke_source, smoke_run, bundle_audit):
            with self.subTest(marker=marker):
                self.assertIn(marker, self.wrapper)

        self.assertLess(self.wrapper.index(builder), self.wrapper.index(smoke_run))
        self.assertLess(self.wrapper.index(smoke_run), self.wrapper.index(bundle_audit))
        self.assertIn("vcvars32.bat", self.wrapper)
        self.assertIn("/std:c++17", self.wrapper)
        self.assertIn("foo_snesapu.private.fb2k-component", self.wrapper)
        self.assertIn("private-foobar-vgm-spc.zip", self.wrapper)

    def test_wrapper_never_claims_or_performs_destructive_retirement(self) -> None:
        self.assertIn("This gate does not delete anything automatically.", self.wrapper)
        forbidden = (
            "git push --delete",
            "gh repo delete",
            "Remove-Item $RetroRoot",
            "Remove-Item $OutputRoot -Recurse -Force",  # canonical builder owns output cleanup
            "repos/delete",
        )
        lowered = self.wrapper.lower()
        for marker in forbidden:
            with self.subTest(marker=marker):
                self.assertNotIn(marker.lower(), lowered)

        # The one Remove-Item in this wrapper may only reset its disposable
        # provider-smoke directory inside WorkRoot.
        remove_lines = [
            line.strip()
            for line in self.wrapper.splitlines()
            if re.search(r"\bRemove-Item\b", line, re.IGNORECASE)
        ]
        self.assertEqual(
            remove_lines,
            ["Remove-Item $SmokeRoot -Recurse -Force -ErrorAction SilentlyContinue"],
        )

    def test_manual_workflow_is_dispatch_only_and_runs_the_wrapper(self) -> None:
        self.assertRegex(self.workflow, r"(?m)^\s*workflow_dispatch:\s*$")
        for forbidden_trigger in ("push:", "pull_request:", "schedule:"):
            with self.subTest(trigger=forbidden_trigger):
                self.assertNotRegex(
                    self.workflow,
                    rf"(?m)^\s*{re.escape(forbidden_trigger)}\s*$",
                )
        self.assertIn("windows-latest", self.workflow)
        self.assertIn("run_vgmspc_retirement_gate.ps1", self.workflow)
        self.assertIn("if: success()", self.workflow)
        self.assertIn("vgmspc-retirement-proof-${{ github.sha }}", self.workflow)

    def test_documentation_does_not_prematurely_authorize_deletion(self) -> None:
        self.assertIn("Not yet executed successfully", self.doc)
        self.assertIn("must not be destructively deleted", self.doc)
        self.assertIn("The gate never deletes", self.doc)
        self.assertIn("run_vgmspc_retirement_gate.ps1", self.doc)
        self.assertIn("vgmspc retirement execution gate PASSED.", self.doc)


if __name__ == "__main__":
    unittest.main()
