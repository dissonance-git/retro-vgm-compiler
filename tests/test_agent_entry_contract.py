#!/usr/bin/env python3
"""Pin the VGM Compiler reasoning-agent first-connection contract."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
AGENTS = ROOT / "AGENTS.md"
README = ROOT / "README.md"
SKILLS = ROOT / ".agents" / "skills"
BOOTSTRAP = ROOT / "tools" / "github_agent_bootstrap.py"

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
        self.assertTrue((SKILLS / "skill-preflight" / "SKILL.md").is_file())

    def test_root_authority_requires_live_bootstrap_and_skill_preflight(self) -> None:
        agents = AGENTS.read_text(encoding="utf-8")
        readme = README.read_text(encoding="utf-8")
        for token in (
            "First-connection skill preflight",
            ".agents/skills/skill-preflight/SKILL.md",
            "enumerate .agents/skills/*/SKILL.md",
            "github-workspace-liveness",
            "repo-change",
            "codex-handoff",
            "tools/github_agent_bootstrap.py",
            "derive/fetch current agent bootstrap",
        ):
            self.assertIn(token, agents)

        self.assertIn("tools/github_agent_bootstrap.py", readme)
        self.assertIn(".agents/skills/", readme)
        self.assertNotIn(".agents/github-connector.json", readme)

    def test_bootstrap_is_derived_from_current_git_and_live_skills(self) -> None:
        completed = subprocess.run(
            [sys.executable, str(BOOTSTRAP), "--json"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        data = json.loads(completed.stdout)
        self.assertEqual(data["schema_version"], "vgm-github-agent-bootstrap-001.0")
        self.assertEqual(data["system"], "VGM Compiler")
        self.assertEqual(data["transport"], "github-connector")
        self.assertEqual(data["authority_path"], "AGENTS.md")
        self.assertEqual(data["identity_path"], "README.md")
        self.assertEqual(
            data["skill_preflight_path"],
            ".agents/skills/skill-preflight/SKILL.md",
        )
        self.assertEqual(
            data["skill_catalog"]["canonical_truth"],
            ".agents/skills/*/SKILL.md",
        )
        self.assertGreaterEqual(data["skill_catalog"]["count"], len(REQUIRED_SKILLS))
        self.assertEqual(len(data["skill_catalog"]["sha256"]), 64)
        self.assertEqual(data["bootstrap_sequence"][0], "identify-vgm-compiler")
        self.assertIn("freeze-github-head", data["bootstrap_sequence"])
        self.assertLess(
            data["bootstrap_sequence"].index("run-skill-preflight"),
            data["bootstrap_sequence"].index("act"),
        )
        self.assertEqual(data["bootstrap_sequence"][-1], "verify")

        head = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        tree = subprocess.run(
            ["git", "rev-parse", "HEAD^{tree}"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        self.assertEqual(data["head_sha"], head)
        self.assertEqual(data["tree_sha"], tree)

    def test_static_connector_state_is_not_a_required_owner(self) -> None:
        self.assertFalse((ROOT / ".agents" / "github-connector.json").exists())


if __name__ == "__main__":
    unittest.main()
