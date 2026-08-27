#!/usr/bin/env python3
"""Pin the VGM Compiler reasoning-agent first-connection contract."""

from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
AGENTS = ROOT / "AGENTS.md"
README = ROOT / "README.md"
CONNECTOR = ROOT / ".agents" / "github-connector.json"
SKILLS = ROOT / ".agents" / "skills"

REQUIRED_SKILLS = {
    "skill-preflight",
    "github-workspace",
    "github-workspace-liveness",
    "repo-change",
    "codex-handoff",
}


class AgentEntryContractTests(unittest.TestCase):
    def test_live_skill_inventory_and_frontmatter(self) -> None:
        discovered: set[str] = set()
        for skill_file in sorted(SKILLS.glob("*/SKILL.md")):
            text = skill_file.read_text(encoding="utf-8")
            self.assertTrue(text.startswith("---\n"), skill_file)
            name_line = next(
                (line for line in text.splitlines() if line.startswith("name: ")),
                None,
            )
            self.assertIsNotNone(name_line, skill_file)
            name = name_line.split(":", 1)[1].strip()
            self.assertEqual(name, skill_file.parent.name, skill_file)
            discovered.add(name)

        self.assertTrue(REQUIRED_SKILLS.issubset(discovered))
        self.assertEqual(
            (SKILLS / "skill-preflight" / "SKILL.md").is_file(),
            True,
        )

    def test_root_authority_requires_skill_preflight(self) -> None:
        agents = AGENTS.read_text(encoding="utf-8")
        readme = README.read_text(encoding="utf-8")
        for token in (
            "First-connection skill preflight",
            ".agents/skills/skill-preflight/SKILL.md",
            "enumerate .agents/skills/*/SKILL.md",
            "github-workspace-liveness",
            "repo-change",
            "codex-handoff",
        ):
            self.assertIn(token, agents)

        self.assertIn(".agents/github-connector.json", readme)
        self.assertIn(".agents/skills/", readme)

    def test_connector_declares_first_connection_funnel(self) -> None:
        data = json.loads(CONNECTOR.read_text(encoding="utf-8"))
        self.assertEqual(
            data["schema_version"],
            "vgm-compiler-github-connector-002.0",
        )
        self.assertFalse(data["canonical_truth"])
        self.assertFalse(data["writable_state"])

        funnel = data["first_connection"]
        self.assertEqual(funnel[0], "identify-vgm-compiler")
        self.assertIn("read-current-agents-authority", funnel)
        self.assertIn("enumerate-live-agent-skills", funnel)
        self.assertIn("run-skill-preflight", funnel)
        self.assertLess(
            funnel.index("run-skill-preflight"),
            funnel.index("act"),
        )
        self.assertEqual(funnel[-1], "verify")

        awareness = data["skill_awareness"]
        self.assertEqual(
            awareness["canonical_inventory"],
            ".agents/skills/*/SKILL.md",
        )
        self.assertEqual(
            awareness["preflight_owner"],
            ".agents/skills/skill-preflight/SKILL.md",
        )
        self.assertEqual(awareness["catalog"], "derive-from-current-tree")
        self.assertFalse(awareness["remembered_skill_bodies_are_authoritative"])

    def test_connector_rules_pin_concurrency_and_validation(self) -> None:
        rules = "\n".join(
            json.loads(CONNECTOR.read_text(encoding="utf-8"))["connector_rules"]
        )
        for token in (
            "exact main HEAD and tree",
            "search for discovery only",
            "moved main as awareness before conflict",
            "absorb useful concurrent work",
            "Bound publication retries",
            "Never force-push",
            "target-SHA-bound",
            "CODEX",
        ):
            self.assertIn(token, rules)


if __name__ == "__main__":
    unittest.main()
