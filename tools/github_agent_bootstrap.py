#!/usr/bin/env python3
"""Emit a disposable GitHub reasoning-agent bootstrap from current repository truth."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


SCHEMA_VERSION = "vgm-github-agent-bootstrap-001.0"
ROOT = Path(__file__).resolve().parents[1]
SKILLS_ROOT = ROOT / ".agents" / "skills"


def git_value(*args: str) -> str:
    completed = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return completed.stdout.strip()


def live_skills() -> list[dict[str, str]]:
    skills: list[dict[str, str]] = []
    for path in sorted(SKILLS_ROOT.glob("*/SKILL.md")):
        text = path.read_text(encoding="utf-8")
        name = path.parent.name
        description = ""
        lines = text.splitlines()
        for index, line in enumerate(lines):
            if line.startswith("name: "):
                declared = line.split(":", 1)[1].strip()
                if declared != name:
                    raise RuntimeError(
                        f"skill name/path mismatch: {path.relative_to(ROOT)} -> {declared}"
                    )
            if line.startswith("description: "):
                description = line.split(":", 1)[1].strip().strip(">")
                if description:
                    break
                for follow in lines[index + 1 :]:
                    if not follow.startswith(" "):
                        break
                    description += (" " if description else "") + follow.strip()
                break
        skills.append(
            {
                "name": name,
                "path": path.relative_to(ROOT).as_posix(),
                "sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
                "description": description.strip(),
            }
        )
    return skills


def catalog_fingerprint(skills: list[dict[str, str]]) -> str:
    payload = json.dumps(
        [{"name": item["name"], "path": item["path"], "sha256": item["sha256"]} for item in skills],
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def build_bootstrap() -> dict[str, object]:
    skills = live_skills()
    return {
        "schema_version": SCHEMA_VERSION,
        "system": "VGM Compiler",
        "transport": "github-connector",
        "semantic_mode": "reasoning-agent-workspace",
        "head_sha": git_value("rev-parse", "HEAD"),
        "tree_sha": git_value("rev-parse", "HEAD^{tree}"),
        "authority_path": "AGENTS.md",
        "identity_path": "README.md",
        "roadmap_path": "docs/vgm-compiler-roadmap.md",
        "skill_preflight_path": ".agents/skills/skill-preflight/SKILL.md",
        "skill_catalog": {
            "count": len(skills),
            "sha256": catalog_fingerprint(skills),
            "canonical_truth": ".agents/skills/*/SKILL.md",
            "skills": skills,
        },
        "bootstrap_sequence": [
            "identify-vgm-compiler",
            "freeze-github-head",
            "read-agents",
            "read-smallest-relevant-readme-surface",
            "enumerate-live-skills",
            "run-skill-preflight",
            "state-exact-obligation",
            "assess-github-capabilities",
            "acquire-smallest-sufficient-context",
            "act",
            "verify",
        ],
        "canonical_truth": False,
        "writable_state": False,
        "note": (
            "This packet is a disposable projection. Current AGENTS.md, README.md, "
            "live skill bytes, exact Git state, code/tests, and evidence owners outrank it."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", default=False)
    args = parser.parse_args()
    packet = build_bootstrap()
    if args.json:
        print(json.dumps(packet, indent=2, sort_keys=True))
    else:
        print(
            f"VGM Compiler GitHub agent bootstrap\n"
            f"HEAD: {packet['head_sha']}\n"
            f"skills: {packet['skill_catalog']['count']}\n"
            f"skill catalog sha256: {packet['skill_catalog']['sha256']}\n"
            "next: read AGENTS.md, run live skill preflight, then state the exact obligation"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
