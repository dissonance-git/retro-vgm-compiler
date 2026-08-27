---
name: github-workspace-liveness
description: Keep long-running GitHub-connector work moving under concurrent main pushes, context pressure, inspection backlogs, publication races, and repeated connector failures without discarding useful staged progress.
---

# GitHub workspace liveness

This is the scheduling subskill for `github-workspace`. It does not decide semantic compatibility and does not publish Git.

> **Preserve progress; bound retries; coalesce awareness; checkpoint before exhaustion.**

## Freeze-resistance rules

### Remote refresh storms

Do not chase every intermediate head.

```text
last accepted head
→ newest observed main
→ one compare
→ one continuation-frontier decision
```

Intermediate commits remain in Git history without each becoming a mandatory restart.

### Deep-inspection floods

Process deterministic attention candidates in bounded waves. Preserve the uninspected tail explicitly rather than truncating it.

### Publication race livelock

After a bounded number of compatible lost races:

```text
checkpoint task state
→ retain overlay/blob identities
→ stop immediate retries
→ resume from newest head
```

Never force-push and never restart the whole task merely because `main` is busy.

### Connector degradation

Repeated identical tool failure is not progress.

```text
checkpoint
→ stop tight retry loop
→ try another admitted route
→ preserve explicit capability block
```

Examples include exact REST fetch instead of a convenience wrapper, blob read instead of floating file read, commit compare instead of repeated search, or Git-object staging instead of sequential coupled file writes.

### Context pressure

Checkpoint before exact task state becomes fragile. Prefer staged Git blob SHAs for exact candidate bytes.

Checkpointing is not completion and not a durable workspace ledger.

## Multi-agent awareness

Classify concurrent work by both interference risk and potential information gain.

```text
compact awareness of intervening commits
→ deep-read only relevant candidates
→ absorb useful work
→ invalidate only affected support
→ preserve unaffected overlay
```

Another agent may improve the active task unexpectedly.

## Capability debt

Use `codex-handoff` only when the remaining material obligation requires a capability the current environment actually lacks. Repository churn is not a capability block.

## Completion state

When material, report accepted/published heads, remote refresh count, absorbed concurrent commits, publication races, validation actually executed, and remaining capability blocks. Do not call a task frozen merely because `main` is active.
