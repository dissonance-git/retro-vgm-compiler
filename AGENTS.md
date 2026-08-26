# AGENTS.md

This file is the operating contract for coding agents. Project identity and repository ownership live in `README.md`; semantic and evidence laws live in `docs/architecture.md`; the musical target lives in `docs/musical-understanding.md`; active priorities live in `docs/vgm-compiler-roadmap.md`. Do not restate those contracts here.

## Enter the repository

For substantive work:

```text
current main HEAD
→ README.md
→ AGENTS.md
→ smallest task owner
→ recent commits touching that owner
→ exact code / test / contract
```

Use `python tools/repository_catalog.py --focus <concept>` before broad repository search. Search inside the selected owner before widening. Use the unfiltered catalog only when repository shape itself is the task.

Read the roadmap only when the task depends on project priority or current frontier. Read durable contracts only when the touched surface depends on them. Do not load every project document by default.

## Change law

1. Work from current `main`. Do not create branches or pull requests unless explicitly requested.
2. Re-read a file from current `main` immediately before replacing it.
3. Prefer one canonical owner over a peer summary, redirect document, duplicate registry, compatibility tombstone, or manually maintained index.
4. Git history owns superseded repository state. Keep historical-origin material only when it remains current evidence, provenance, a reproducibility input, or an implementation obligation.
5. Preserve unrelated concurrent work. Never force-push.
6. Never rewrite immutable corpus bytes or imported upstream evidence during cleanup or refactoring.
7. Generated data, caches, build products, and task projections are disposable unless a current contract explicitly makes an artifact canonical evidence.
8. After publication, verify the resulting commit and distinguish executed evidence from intended behavior.

Direct user correction outranks repository prose.

## Authority discipline

A project fact should have one writable authority.

Before adding or duplicating a file, section, registry, wrapper, or abstraction, ask:

```text
What exact obligation does this object uniquely own?
Does a current owner already exist?
Can this relation be derived instead of stored?
Would this create a second place that can become stale?
```

If the first question has no strong answer, extend or derive from an existing owner instead.

Current-state statements belong in `docs/vgm-compiler-roadmap.md`. Durable semantic laws belong in `docs/architecture.md`. Musical goals belong in `docs/musical-understanding.md`. Repository identity and routing belong in `README.md`.

Specialized documents under `docs/` must own a distinct durable contract. Research material belongs under the narrowest durable question in `research/`; it does not become architecture merely by being detailed.

## Context law

Human attention and model context are finite compute.

- Route to the smallest canonical owner that can answer the task.
- Prefer mechanically derived relations over duplicated prose navigation.
- Read relation shape before opening every related file in full.
- Widen context only when required recall, uncertainty, or verification demands it.
- A smaller context is better only when correctness, provenance, required recall, task capability, and verification are preserved or improved.
- When two correct designs are otherwise equivalent, prefer the one requiring fewer searches, fewer repeated explanations, and less context to reach the same verified conclusion.

The target is useful reasoning per unit of context, not arbitrary brevity.

## Semantic discipline

Do not invent local substitutes for project-wide evidence or semantic laws. When work crosses source fact, musical inference, perceptual claim, documentary evidence, provenance, identity, or availability, follow `docs/architecture.md` exactly.

Preserve source-native semantics before normalization. Shared abstractions are earned when materially different source families need the same relation without losing useful native distinctions.

## Verification

Default core route:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Prefer focused tests first, then broader suites when the touched surface warrants them. Use real corpus controls when a claim crosses from implementation into preserved music.

Do not call CI green unless a runner executed successfully. Do not weaken an acceptance gate merely to obtain an artifact.

Private foobar2000 VGM/SPC delivery is owned by `.github/workflows/private-foobar-build.yml` and `tools/build_private_foobar_components.ps1`. A DLL compile or archive creation alone is not publish-ready.
