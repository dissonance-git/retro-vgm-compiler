---
name: github-workspace
description: Treat the GitHub connector as a coherent VGM Compiler repository workspace: observe exact state, orient with bounded context, stage edits, surface concurrent work, classify interference, inspect validation, publish safely, and preserve re-entry state.
---

# GitHub workspace

This skill owns GitHub transport procedure. `AGENTS.md` remains authority.

## Core model

```text
exact Git snapshot
+ immutable blob/file reads
+ read-set
+ write-set
+ dependency-set
+ protected-set
+ staged overlay
+ remote-awareness delta
= ephemeral connector workspace
```

Do not create a durable workspace ledger.

## Operating loop

```text
observe
→ orient
→ act
→ verify
→ refresh-awareness
→ continue
```

### Observe

Freeze an exact `main` head and tree. Read mutable files by that ref or immutable blob SHA.

Search locates candidates. It does not prove absence or establish current bytes. Prefer exact trees/files for repository truth.

For large scopes, hydrate exact tree chunks or bounded recursive subtrees. A truncated/partial result never proves absence outside its observed scope.

### Orient

Build the smallest map that can change the decision:

```text
owner/path
→ symbols/headings/relations
→ consumers/tests/recent commits
→ relevant excerpts
→ full file only when needed
```

For local execution, `tools/repository_catalog.py` is the deterministic projection owner. Connector-only sessions should emulate its bounded-routing intent from exact Git evidence, not create a second permanent map.

Track explicit omissions and unknown scope.

### Act

Use the narrowest mutation primitive that preserves the intended invariant.

```text
one independent text path
→ fresh blob-SHA contents CAS

coupled paths
→ create blobs
→ create one candidate tree
→ create one candidate commit
→ guarded fast-forward publication
```

The LLM reasons about a staged overlay, not a sequence of half-published edits.

### Verify

A write response proves accepted bytes, not correctness.

Keep separate:

```text
verification observation
rerun of an existing exact-SHA workflow/job
published-SHA push-triggered execution
arbitrary fresh execution
```

Never transfer a PASS between SHAs.

### Refresh awareness

Before publication and after material interruption:

1. fetch newest `main`;
2. compare from the last accepted head;
3. inspect every intervening commit compactly;
4. deepen only where read/write/dependency/protected state or useful new context requires it.

Classify movement:

```text
remote-context-available
refresh-context
write-overlap-review
protected-owner-changed
history-diverged
```

Remote movement and conflict are not synonyms. Path-disjoint work can still introduce positive interference such as new evidence, tests, helpers, or a better owner.

If a remote path changed a premise used by the task, re-read that premise. If it touches an intended write, inspect exact overlap. If it touches a protected owner, re-enter its contract.

A parent SHA may need rebuilding without invalidating staged semantic bytes. Preserve unaffected progress.

### Continue

Carry forward only:

```text
accepted-head
read-set
write-set
dependency-set
protected-set
staged-overlay
validation-state
capability-blocks
```

Git owns history. Task state remains ephemeral.

## Capability handshake

Do not hard-code old connector limitations.

At entry, map the concrete tool surface into semantic capabilities such as:

```text
exact-ref/commit/tree-read
exact-file/blob-read
bounded-search
commit-compare
contents-cas
git-object-staging
guarded-ref-update
workflow/job/log/artifact-observation
workflow-rerun
issue-read/write
fresh-execution
```

Use the strongest admitted route. Repository control and fresh runtime execution are separate capabilities.

Partial, paginated, first-page-only, ranked, or truncated wrappers must remain explicitly partial in reasoning.

## Publication

For coupled work:

```text
refresh main
→ revalidate changed premises
→ rebuild candidate on newest accepted tree if needed
→ parent = newest accepted head
→ final refresh
→ fast-forward only
→ re-fetch main
```

Never force-push.

Substantial direct-main commits use the `vgm-task`, `vgm-change-kind`, `vgm-validation`, and optional `vgm-handoff` trailers defined by `AGENTS.md`.

## Re-entry

Preserve only the smallest sufficient capsule:

```text
goal
accepted-head/tree
read/write/dependency/protected sets
staged blob identities
verified facts
validation target/state
capability blocks
next action
```

Materialize staged upserts as Git blobs before long pauses when possible. The capsule is disposable state, not repository truth.
