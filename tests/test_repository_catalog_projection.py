#!/usr/bin/env python3
"""Contracts for context-compressed repository projections."""
from __future__ import annotations

import sys
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import repository_catalog as catalog  # noqa: E402


class RepositoryCatalogProjectionTest(unittest.TestCase):
    def build_fixture(self, root: Path) -> list[str]:
        contents = {
            "model/cadence.h": '#pragma once\n#include "music_state.h"\nstruct Cadence {};\n',
            "model/music_state.h": "#pragma once\nstruct MusicState {};\n",
            "tests/cadence_behavior.cpp": '#include "model/cadence.h"\nint main() { return 0; }\n',
            "cmake/semantic.cmake": "add_executable(cadence_behavior tests/cadence_behavior.cpp)\n",
            "docs/architecture.md": "See [cadence owner](../model/cadence.h).\n",
            "README.md": "Synthetic repository fixture.\n",
        }
        for relative, text in contents.items():
            path = root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
        return sorted(contents)

    def test_mechanical_relations_are_typed_and_exact(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            files = self.build_fixture(root)
            relations = set(catalog.mechanical_relations(root, files))

            self.assertIn(
                catalog.Relation("model/cadence.h", "includes", "model/music_state.h"),
                relations,
            )
            self.assertIn(
                catalog.Relation(
                    "tests/cadence_behavior.cpp", "includes", "model/cadence.h"
                ),
                relations,
            )
            self.assertIn(
                catalog.Relation("docs/architecture.md", "links_to", "model/cadence.h"),
                relations,
            )
            self.assertIn(
                catalog.Relation(
                    "cmake/semantic.cmake", "registers", "tests/cadence_behavior.cpp"
                ),
                relations,
            )

    def test_focus_expands_relation_only_neighbor(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            files = self.build_fixture(root)
            projection = catalog.build_focus_projection(root, files, "cadence", limit=8)

            selected = {entry["path"]: entry for entry in projection["files"]}
            self.assertIn("model/cadence.h", selected)
            self.assertIn("model/music_state.h", selected)
            self.assertEqual(
                selected["model/music_state.h"]["signal"],
                "relation:includes",
            )
            self.assertGreater(projection["mechanical_relation_count"], 0)
            self.assertGreater(projection["relation_expanded_candidate_count"], 0)
            self.assertLessEqual(projection["selected_file_count"], 8)

            selected_relations = {
                (entry["source"], entry["type"], entry["target"])
                for entry in projection["relations"]
            }
            self.assertIn(
                ("model/cadence.h", "includes", "model/music_state.h"),
                selected_relations,
            )

    def test_external_links_do_not_become_repository_edges(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            files = ["docs/guide.md"]
            (root / "docs").mkdir(parents=True)
            (root / "docs/guide.md").write_text(
                "[external](https://example.com/cadence.md)\n",
                encoding="utf-8",
            )
            self.assertEqual(catalog.mechanical_relations(root, files), [])


if __name__ == "__main__":
    unittest.main()
