# Repository representation benchmark

## Purpose

This benchmark tests whether repository representation lets the same human or language model reach **more correct, verifiable conclusions with less exposed context**.

It is a reusable validation contract, not a history or score diary. Results belong in transient experiment output unless a result earns a durable architectural change.

The benchmark treats repository design as part of the execution environment:

```text
same repository obligation
+
same model / human task
+
same tools and verification ceiling
+
different repository representation
→ compare capability and cost
```

A representation does not win merely because it is smaller. It wins only when compression preserves or improves the required result.

---

## Primary hypothesis

```text
canonical ownership
+
stable semantic naming
+
derived typed relations
+
focused task projection
→ fewer files / searches / tokens
→ equal or stronger verified decisions
```

The current focused projection is intentionally derived from tracked repository truth. It must not become a second writable semantic database.

---

## Representation conditions

Use the smallest set needed for a particular experiment. Useful controls are:

```text
A  filesystem paths only
B  lexical path/content focus
C  lexical focus + exact derived mechanical relations
D  broad repository search
E  broad repository context where technically feasible
```

Condition C is the current candidate path implemented by `tools/repository_catalog.py --focus`.

When another retrieval system is evaluated, add it as a peer condition rather than silently replacing the controls.

---

## Task families

Representation quality must be tested across several task types because one retrieval method can help one class and hurt another.

### Semantic-model location

Questions such as:

- where is a musical concept represented;
- which test establishes its current contract;
- which build registry makes that test executable;
- which architecture document owns the general rule.

### Source-family mechanism

Questions such as:

- where a format-specific runtime mechanism lives;
- which source/provider contracts constrain it;
- which tests or workflows exercise it;
- which shared model it may or may not populate.

### Build and materialization

Questions such as:

- what reconstructs a source input;
- what consumes the reconstructed object;
- which workflow invokes the route;
- which contract verifies the produced bundle.

### Research/evidence routing

Questions such as:

- where one research program owns a claim boundary;
- which exact evidence or admission object constrains it;
- which tool derives the analysis;
- which validation surface can falsify it.

### Re-entry

Give a fresh agent only a short continuation instruction and the repository. Measure whether it recovers the exact current frontier without reopening settled or historical paths.

---

## Ground truth

Do not encode benchmark ground truth into the navigation tool itself.

Each controlled task should declare its expected required surface separately from the candidate projection. A task-local benchmark fixture may name required files or relations because it is an evaluation oracle, not repository truth.

For synthetic tests, construct small repositories where at least one required neighbor contains **none of the focus vocabulary**. This distinguishes structural retrieval from lexical coincidence.

For live-repository evaluations, freeze the Git commit and task before inspecting candidate output. The oracle should be reviewed independently from the representation being tested.

---

## Current deterministic relation classes

`repository_catalog.py` may derive relations only when they are mechanically supported by current tracked text.

Current relation classes:

```text
includes       quoted local C/C++ include
links_to       local Markdown link
registers      tracked path referenced by CMake input
```

Reverse traversal is derived in memory:

```text
included_by
linked_from
registered_by
```

These relations are projections, not canonical metadata.

Do not add a semantic relation because it would make one benchmark look better. A new relation class must state how it is derived, what false positives/negatives are possible, and which task family it is expected to improve.

---

## Primary measurements

Record raw measurements before constructing convenience scores.

```text
verified task success
required-file recall
required-relation recall
irrelevant-file count
selected-file count
tokens exposed
files opened
search operations
tool calls
time to canonical owner
wrong-owner edits
verification failures
human correction / cleanup
```

When exact token accounting is unavailable, byte or character counts may be recorded as a declared proxy. Do not silently call a proxy "tokens".

---

## Compression measurements

For a cost dimension `c`:

```text
compression_gap(c) = baseline_cost(c) / candidate_cost(c)
```

A gap greater than 1 is useful only when:

```text
candidate verified utility >= baseline verified utility
```

Useful repository-level ratios include:

```text
selected files / tracked files
required files / selected files
irrelevant files / selected files
verified tasks / context tokens
verified tasks / tool calls
```

Do not optimize selection ratio by itself. Selecting one file is worthless when five required files are omitted.

---

## Pareto rule

Prefer raw cost vectors over a single decorative score.

A candidate representation dominates a baseline when it provides equal or greater verified task utility and is no worse on every declared cost dimension, with strict improvement on at least one.

If a scalar is useful inside one fixed experiment, name its weights and preserve the raw measurements beside it.

---

## Required falsifiers

A representation change fails or must be narrowed when any of these survive controlled evaluation:

- token/context cost falls but required-file recall falls materially;
- relation expansion increases irrelevant context without improving task success;
- a derived edge class produces misleading navigation often enough to raise cleanup cost;
- routing cost exceeds the context saved;
- focused projection repeatedly requires broad search to repair omissions;
- a simpler lexical or filesystem baseline performs equally well at lower total cost;
- manually maintained navigation becomes necessary to keep the derived view correct;
- context becomes smaller while verification becomes weaker.

The benchmark is designed to kill attractive compression when it only moves complexity elsewhere.

---

## Re-entry test

A reusable re-entry trial should record:

```text
reads before exact frontier recovery
context before first correct action
stale paths consulted
settled questions accidentally reopened
correct owner recovered
required tests recovered
human clarification required
```

The target is not perfect memory. It is cheap reconstruction from current canonical state.

---

## Naming and topology pressure tests

Repository representation is broader than retrieval tooling.

For naming changes, compare:

- descriptive concept-shaped name;
- abbreviation or generic name;
- old/status-shaped name when one exists in a frozen control.

Measure location time, wrong-owner selection, and explanation accuracy.

For topology changes, compare ownership-shaped placement against mixed-axis placement. Measure path guesses, backtracking, and required searches.

For document consolidation, compare one dense canonical owner against overlapping peers while preserving total decision-relevant content as closely as possible.

---

## Promotion rule

A repository representation rule earns stronger status only after it survives more than the case that inspired it.

```text
synthetic control
→ representative live tasks
→ held-out live tasks
→ different task family
→ keep / narrow / reject
```

A successful mechanism can then be folded into repository law. A failed mechanism remains useful as a constraint on future design, but does not require a permanent active diary.

---

## Current implementation boundary

The focus projection is navigation only.

```text
tracked repository truth
→ deterministic lexical + mechanical analysis
→ disposable focused projection
→ human / LLM chooses canonical files
```

It does not own musical semantics, source provenance, research claims, corpus identity, build truth, or test truth.

The highest-value future expansion is not "more graph" by default. It is whichever exact derived relation or projection measurably increases verified capability per unit of exposed context.
