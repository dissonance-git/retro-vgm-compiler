from __future__ import annotations

import importlib.util
import pathlib
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "z80_rip_corpus_audit", ROOT / "tools" / "z80_rip_corpus_audit.py"
)
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class Z80RipCorpusAuditTest(unittest.TestCase):
    def test_sgc_and_unique_playlist_routes_are_admitted(self) -> None:
        root = pathlib.Path(tempfile.mkdtemp(prefix="sgc-audit-"))
        (root / "game.sgc").write_bytes(b"SGC\x1a\x01" + bytes(0x9B))
        (root / "01.m3u").write_text("game.sgc::KSS,0,First,1:00\n", encoding="utf-8")
        (root / "02.m3u").write_text("game.sgc::KSS,1,Second,1:00\n", encoding="utf-8")

        report = AUDIT.audit_directory(root)

        self.assertTrue(report["valid"], report["errors"])
        self.assertEqual(report["container_count"], 1)
        self.assertEqual(report["playlist_route_count"], 2)

    def test_kss_signature_is_checked(self) -> None:
        root = pathlib.Path(tempfile.mkdtemp(prefix="kss-audit-"))
        (root / "game.kss").write_bytes(b"NOPE" + bytes(12))

        report = AUDIT.audit_directory(root)

        self.assertFalse(report["valid"])
        self.assertIn("missing KSCC/KSSX signature", report["containers"][0]["errors"])

    def test_missing_playlist_target_is_rejected(self) -> None:
        root = pathlib.Path(tempfile.mkdtemp(prefix="sgc-audit-"))
        (root / "game.sgc").write_bytes(b"SGC\x1a\x01" + bytes(0x9B))
        (root / "01.m3u").write_text("missing.sgc::KSS,0,First,1:00\n", encoding="utf-8")

        report = AUDIT.audit_directory(root)

        self.assertFalse(report["valid"])
        self.assertTrue(any("not a retained" in error for error in report["errors"]))


if __name__ == "__main__":
    unittest.main()
