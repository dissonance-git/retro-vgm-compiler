---
name: codex-handoff
description: Stage VGM Compiler work that is concrete and actionable but cannot be safely completed through the current ChatGPT/GitHub connector surface, such as missing local execution, emulator/runtime probes, dependency installation, private artifact work, or physical listening.
---

# Codex handoff

GitHub Issues are the authoritative queue for concrete capability debt.

```text
title prefix: CODEX:
body marker: <!-- vgm-compiler-codex-handoff:v1 -->
```

Do not create a parallel JSON, Markdown, roadmap, or task-file queue.

## Use a handoff when

The remaining obligation is exact/actionable but requires a capability unavailable here, for example:

- local CMake/compiler/test execution;
- emulator/native runtime execution;
- private foobar/package/runtime verification;
- hardware or physical-listening validation;
- dependency/toolchain installation;
- binary/artifact inspection;
- OS/kernel/API probing;
- CI diagnostics unavailable through the connector.

Do not stage ordinary research uncertainty, a user decision, vague future work, work still possible through the connector, publication contention, or a duplicate issue.

## Duplicate rule

Search open `CODEX:` issues for the stable handoff key or exact obligation before creating another.

## Required packet

```text
<!-- vgm-compiler-codex-handoff:v1 -->

handoff-key: <stable-lowercase-kebab-case-key>
priority: P0 | P1 | P2
blocked-interface: <exact boundary>
source-commit: <full SHA>
required-capabilities:
  - <capability>
```

Then include obligation, why staged, already completed work, affected routes, copy/pasteable re-entry, acceptance criteria, guardrails, and evidence required before closing.

## Freshness and closure

The source commit is orientation, not overwrite permission.

A local/Codex executor must fetch current `main`, inspect changes since the source commit, preserve concurrent work, and re-run the blocked obligation on current bytes.

Do not close because code was merely written. Attach the evidence satisfying acceptance criteria. Close obsolete/duplicate work explicitly rather than manufacturing success.

Codex issues are execution-handoff state, not musical truth, research evidence, or a second roadmap.
