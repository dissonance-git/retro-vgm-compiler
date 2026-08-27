---
name: repo-change
description: Execute a bounded VGM Compiler repository change safely and prove what actually landed. Use for code, documentation, schema, test, configuration, cleanup, or migration edits, especially when concurrent main or GitHub connector publication matters.
---

# Repository change

This skill is procedural and subordinate to `AGENTS.md`.

## Core invariant

```text
inspect current truth
→ identify one bounded change
→ preserve unrelated work
→ stage through an allowed route
→ validate actual result
→ publish from a fresh base
→ verify publication
```

## Execution route

Use a real local checkout when available. Use `github-workspace` when GitHub is the authoritative transport. Do not require a local checkout merely because one is familiar.

For connector work, `github-workspace` owns exact snapshotting, concurrent refresh, overlays, and fast-forward publication. This skill owns mutation/completion, not a competing concurrency protocol.

## Freeze the base

Before editing, record:

```text
repository
publication target
source-head/tree
paths in scope
protected paths/evidence
acceptance conditions
required validation
```

## Inspect before editing

At minimum inspect:

1. root authority;
2. current implementation/document;
3. relevant tests/validators;
4. consumers/schemas when semantics propagate;
5. recent commits touching the area when churn is plausible.

Determine generators before editing generated outputs. Preserve immutable corpus/import identity unless migration is explicitly in scope.

## Smallest sufficient edit

Prefer:

```text
existing owner over parallel owner
one executable invariant over prose-only policy
bounded replacement over unrelated churn
semantic collapse over archive/tombstone proliferation
```

Do not broaden a task because nearby cleanup looks attractive.

## Connector-safe publication

One independent text path may use a fresh blob-SHA compare-and-swap.

Coupled work uses:

```text
fresh source head/tree
→ create every replacement blob
→ create one candidate tree
→ create one candidate commit
→ refresh main
→ inspect/absorb intervening work
→ rebuild parent/tree if still compatible
→ fast-forward only
→ re-fetch ref and changed paths
```

Never force-push.

## Validation

Match validation to the claim:

```text
documentation/ownership
→ route/link/contract tests

C++ behavior
→ focused compile/test + broader suite as warranted

source-family semantics
→ exact source tests + relevant corpus pressure

packaging/private playback
→ package/runtime workflow + artifact/runtime checks

perceptual change
→ engineering evidence + physical listening
```

A connector write is not a test pass. A test pass from another SHA is not evidence for the published bytes.

## Completion

After publication, fetch the target ref, prove the intended commit is current, inspect exact changed paths/content, inspect target-SHA validation, report unexecuted checks separately, and avoid creating a history document for completed work.
