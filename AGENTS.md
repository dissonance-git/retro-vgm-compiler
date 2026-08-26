# AGENTS.md

This file is the operating contract for coding agents. Project meaning lives in `docs/architecture.md`, the musical target lives in `docs/musical-understanding.md`, and active priorities live in `docs/vgm-compiler-roadmap.md`. Do not restate those contracts here.

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

## Context law

Human attention and LLM context are finite compute.

- Route to the smallest canonical owner that can answer the task.
- Prefer mechanically derived relations over duplicated prose navigation.
- Read relation shape before opening every related file in full.
- Widen context only when required recall, uncertainty, or verification demands it.
- A smaller context is better only when correctness, provenance, required recall, task capability, and verification are preserved or improved.
- When two correct designs are otherwise equivalent, prefer the one requiring fewer searches, fewer repeated explanations, and less context to reach the same verified conclusion.

The target is useful reasoning per unit of context, not arbitrary brevity.

## Ownership

Top-level ownership is defined in `README.md`; local owner maps belong beside the objects they route. Semantic and evidence laws belong in `docs/architecture.md`. Do not turn `AGENTS.md` into a second repository manual.

Before adding a file or abstraction, ask:

```text
Does a current owner already exist?
Can this relation be derived?
Would another writable surface create competing truth?
What current obligation would this new object uniquely own?
```

If the last question has no strong answer, extend an existing owner instead.

## Evidence and source boundaries

Do not collapse source fact, musical inference, perceptual claim, documentary evidence, or hypothesis into one evidence state. Preserve source-native semantics before normalization. Shared abstractions are earned when materially different source families need the same relation without losing useful native distinctions.

The complete rules are canonical in `docs/architecture.md`.

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
