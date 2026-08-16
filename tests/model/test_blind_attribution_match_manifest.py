from __future__ import annotations

import importlib.util
import pathlib
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "blind_attribution_match_manifest.py"
SPEC = importlib.util.spec_from_file_location("blind_attribution_match_manifest", TOOL)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"could not load {TOOL}")
manifest = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = manifest
SPEC.loader.exec_module(manifest)


def feature(interval: str, patch: str) -> dict[str, object]:
    return {
        "musical_trajectory": {
            "interval_histogram_semitones": {interval: 4},
            "interval_bigram_histogram": {f"{interval},{interval}": 3},
            "normalized_onset_gap_histogram": {"1.00": 4},
            "contour_histogram": {"up": 4},
        },
        "realization": {
            "core_patch_usage": {patch: 4},
            "algorithm_histogram": {"4": 4},
            "feedback_histogram": {"2": 4},
            "pan_histogram": {"3": 4},
        },
    }


class BlindAttributionMatchManifestTest(unittest.TestCase):
    def setUp(self) -> None:
        self.registry = {
            "controls": [
                {
                    "candidate": "Composer A",
                    "fixture_path": "tests/corpus/a/track-a.vgz",
                    "corpus_id": "soundtrack-a",
                    "platform": "Mega Drive",
                    "role": "composer",
                    "status": "exact",
                    "confidence": 0.96,
                    "source": "direct-credit-a",
                    "work_family_id": "family-a",
                    "implementation_family_id": "driver-a",
                },
                {
                    "candidate": "Programmer B",
                    "fixture_path": "tests/corpus/b/track-b.vgz",
                    "corpus_id": "soundtrack-b",
                    "platform": "Mega Drive",
                    "role": "arranger_programmer",
                    "status": "exact",
                    "confidence": 0.93,
                    "source": "direct-credit-b",
                    "work_family_id": "family-b",
                    "implementation_family_id": "driver-b",
                },
                {
                    "candidate": "Composer C",
                    "fixture_path": "tests/corpus/c/track-c.spc",
                    "corpus_id": "soundtrack-c",
                    "platform": "Super NES",
                    "role": "composer",
                    "status": "exact",
                    "confidence": 0.95,
                    "source": "direct-credit-c",
                },
            ]
        }

    def test_composer_manifest_uses_only_structural_view(self) -> None:
        controls = manifest.eligible_controls(self.registry, "composer")
        self.assertEqual(len(controls), 1)
        query = feature("2", "query-patch")
        control_features = {
            "tests/corpus/a/track-a.vgz": feature("2", "different-patch"),
        }
        result = manifest.build_match_manifest(
            query_id="sonic3-sandopolis-1",
            query_path=pathlib.Path("tests/corpus/sonic-3-beta-vgm/Sandopolis.vgz"),
            role="composer",
            query_feature=query,
            controls=controls,
            control_features=control_features,
            query_platform_id="Mega Drive",
            query_implementation_family_id="smps-sonic3",
        )
        self.assertEqual(result["match_count"], 1)
        match = result["matches"][0]
        self.assertEqual(match["candidate"], "Composer A")
        self.assertEqual(match["dimension"], "melody")
        self.assertEqual(match["source"], "blind-genesis-vgm:musical_trajectory")
        self.assertAlmostEqual(match["match_strength"], 1.0)
        self.assertEqual(match["status"], "hypothesis")

    def test_arranger_manifest_uses_realization_not_composer_view(self) -> None:
        controls = manifest.eligible_controls(self.registry, "arranger_programmer")
        self.assertEqual(len(controls), 1)
        query = feature("7", "same-patch")
        control_features = {
            "tests/corpus/b/track-b.vgz": feature("-5", "same-patch"),
        }
        result = manifest.build_match_manifest(
            query_id="sonic3-data-select",
            query_path=pathlib.Path("tests/corpus/sonic-3-beta-vgm/Data Select.vgz"),
            role="arranger_programmer",
            query_feature=query,
            controls=controls,
            control_features=control_features,
        )
        match = result["matches"][0]
        self.assertEqual(match["dimension"], "arrangement_orchestration")
        self.assertEqual(match["source"], "blind-genesis-vgm:realization")
        self.assertAlmostEqual(match["match_strength"], 1.0)

    def test_non_vgm_controls_are_not_silently_forced_through_genesis_frontend(self) -> None:
        controls = manifest.eligible_controls(self.registry, "composer")
        self.assertEqual([item["candidate"] for item in controls], ["Composer A"])

    def test_candidate_filter_is_explicit_and_role_scoped(self) -> None:
        controls = manifest.eligible_controls(
            self.registry,
            "composer",
            {"Composer A"},
        )
        self.assertEqual(len(controls), 1)
        self.assertEqual(controls[0]["candidate"], "Composer A")
        with self.assertRaises(ValueError):
            manifest.eligible_controls(self.registry, "artist")


if __name__ == "__main__":
    unittest.main()
