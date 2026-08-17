from __future__ import annotations

import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "spc" / "capture_blind_panel.py"
SPEC = importlib.util.spec_from_file_location("capture_blind_panel_mixed_test", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
capture = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = capture
SPEC.loader.exec_module(capture)


class MixedBlindPanelCaptureTest(unittest.TestCase):
    def test_load_panel_accepts_spc_and_genesis_without_identity_fields(self):
        with tempfile.TemporaryDirectory() as raw_tmp:
            tmp = pathlib.Path(raw_tmp)
            panel = tmp / "panel.json"
            panel.write_text(
                json.dumps({
                    "cues": [
                        {"cue_id": "cue-001", "fixture_path": "tests/corpus/a/one.spc"},
                        {"cue_id": "cue-002", "fixture_path": "tests/corpus/b/two.vgz"},
                    ]
                }),
                encoding="utf-8",
            )
            cues = capture.load_panel(panel)
            self.assertEqual([cue.cue_id for cue in cues], ["cue-001", "cue-002"])
            self.assertEqual(
                [cue.fixture_path.suffix.lower() for cue in cues],
                [".spc", ".vgz"],
            )

    def test_load_panel_rejects_creator_fields_and_unknown_source_types(self):
        with tempfile.TemporaryDirectory() as raw_tmp:
            tmp = pathlib.Path(raw_tmp)
            leaked = tmp / "leaked.json"
            leaked.write_text(
                json.dumps({
                    "cues": [
                        {
                            "cue_id": "cue-001",
                            "fixture_path": "tests/corpus/a/one.spc",
                            "composer": "hidden",
                        },
                        {"cue_id": "cue-002", "fixture_path": "tests/corpus/a/two.spc"},
                    ]
                }),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "creator-bearing"):
                capture.load_panel(leaked)

            unknown = tmp / "unknown.json"
            unknown.write_text(
                json.dumps({
                    "cues": [
                        {"cue_id": "cue-001", "fixture_path": "tests/corpus/a/one.mid"},
                        {"cue_id": "cue-002", "fixture_path": "tests/corpus/a/two.spc"},
                    ]
                }),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "only .spc, .vgm, or .vgz"):
                capture.load_panel(unknown)

    def test_genesis_requires_explicit_continuity_parameters(self):
        with tempfile.TemporaryDirectory() as raw_tmp:
            repo = pathlib.Path(raw_tmp)
            freeze_tool = repo / "freeze.py"
            freeze_tool.write_text("# placeholder\n", encoding="utf-8")
            cue = capture.PanelCue(
                cue_id="cue-001",
                fixture_path=pathlib.PurePosixPath("tests/corpus/g/one.vgz"),
            )
            with self.assertRaisesRegex(ValueError, "genesis-max-gap-ticks"):
                capture.capture_panel(
                    [cue],
                    repo_root=repo,
                    extractor=None,
                    output_dir=repo / "out",
                    seconds=5,
                    freeze_tool=freeze_tool,
                    freeze_output=repo / "freeze.json",
                    genesis_max_gap_ticks=None,
                    genesis_max_pitch_interval_octaves=1.5,
                )

    def test_mixed_panel_routes_spc_sidecar_and_genesis_profile_bundle_to_one_freeze(self):
        with tempfile.TemporaryDirectory() as raw_tmp:
            repo = pathlib.Path(raw_tmp)
            spc_fixture = repo / "tests/corpus/spc-world/one.spc"
            vgz_fixture = repo / "tests/corpus/genesis-world/two.vgz"
            spc_fixture.parent.mkdir(parents=True)
            vgz_fixture.parent.mkdir(parents=True)
            spc_fixture.write_bytes(b"SPC")
            vgz_fixture.write_bytes(b"VGZ")
            extractor = repo / "spc-extractor"
            extractor.write_text("placeholder\n", encoding="utf-8")
            freeze_tool = repo / "freeze.py"
            freeze_tool.write_text("# placeholder\n", encoding="utf-8")
            cached_spc = repo / "cache/spc.json"
            cached_spc.parent.mkdir(parents=True)
            cached_spc.write_text("{}\n", encoding="utf-8")

            cues = [
                capture.PanelCue("cue-001", pathlib.PurePosixPath("tests/corpus/spc-world/one.spc")),
                capture.PanelCue("cue-002", pathlib.PurePosixPath("tests/corpus/genesis-world/two.vgz")),
            ]
            bundle = {
                "model": "creator-blind persistent-part motif profile bundle",
                "representation": "genesis_vgm_persistent_part_motif",
                "provenance": {"test": 1},
                "diagnostics": {"part_profile_count": 1},
                "part_profiles": [{
                    "normalized_inter_onset_intervals": [1.0, 1.0],
                    "interval_octaves": [0.1, -0.1],
                    "pitch_contour": [1, -1],
                    "interval_semantics": "log2_frequency_ratio_octaves",
                    "evidence_confidence": 0.9,
                }],
            }

            with (
                mock.patch.object(capture.spc_cache, "build_one", return_value=(cached_spc, False)),
                mock.patch.object(
                    capture.genesis_cache,
                    "build_one",
                    return_value=(repo / "cache/genesis.json", False, {"opaque": True}),
                ) as genesis_build,
                mock.patch.object(
                    capture.genesis_parts,
                    "project",
                    return_value={"strand_projection": {}},
                ) as genesis_project,
                mock.patch.object(
                    capture.genesis_parts,
                    "make_motif_profile_bundle",
                    return_value=bundle,
                ),
                mock.patch.object(capture.subprocess, "run") as run,
            ):
                capture.capture_panel(
                    cues,
                    repo_root=repo,
                    extractor=extractor,
                    output_dir=repo / "opaque",
                    seconds=5,
                    freeze_tool=freeze_tool,
                    freeze_output=repo / "frozen.json",
                    cache_root=repo / "spc-cache",
                    genesis_cache_root=repo / "genesis-cache",
                    genesis_max_gap_ticks=500,
                    genesis_max_pitch_interval_octaves=1.5,
                )

            genesis_build.assert_called_once()
            genesis_project.assert_called_once_with(
                {"opaque": True},
                max_gap_ticks=500,
                max_pitch_interval_octaves=1.5,
                strand_min_confidence=capture.genesis_parts.DEFAULT_STRAND_MIN_CONFIDENCE,
            )
            command = run.call_args.args[0]
            self.assertIn("--cue", command)
            self.assertIn("--profile-bundle", command)
            self.assertTrue(any(str(item).startswith("cue-001=") for item in command))
            self.assertTrue(any(str(item).startswith("cue-002=") for item in command))
            self.assertEqual(run.call_args.kwargs["cwd"], repo.resolve())
            self.assertTrue(run.call_args.kwargs["check"])


if __name__ == "__main__":
    unittest.main()
