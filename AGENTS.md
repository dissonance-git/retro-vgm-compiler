# AGENTS.md

This file is the **small operational contract for coding agents**. Project meaning lives in `docs/architecture.md`; active priorities live in `docs/vgm-compiler-roadmap.md`. Do not duplicate those documents here.

## Enter the repo

For substantive work:

```text
current main HEAD
→ README.md
→ AGENTS.md
→ docs/vgm-compiler-roadmap.md
→ smallest task owner
→ recent commits touching that owner
→ exact code/test/document
```

Use `python tools/repository_catalog.py` for mechanical inventory. Search inside the chosen owner before searching the whole repository.

## Change law

1. Work from current `main`; do not create branches or PRs unless explicitly requested.
2. Before replacing a file, re-read the current file from current `main`.
3. Make the smallest coherent change that removes the real uncertainty or duplication.
4. Preserve unrelated concurrent work. Never force-push.
5. Prefer extending a canonical owner over creating a peer document, tool, cache, or abstraction.
6. Generated data and caches must stay untracked. Git history is the archive for superseded repository state.
7. Do not rewrite immutable corpus source bytes or imported upstream evidence during cleanup/refactoring.
8. After publication, verify the resulting commit and distinguish executed evidence from intended behavior.

Direct user correction outranks repository prose.

## Ownership

```text
model/       shared musical/evidence contracts only after cross-source agreement
components/  source-family/device-specific semantics and runtime machinery
tests/       executable regressions + immutable corpus
research/    bounded evidence, experiments, testbeds, preregistrations
docs/        durable current contracts only
tools/       reusable operations, not hidden state
imports/     preserved external evidence
patches/     maintained external-source patch stacks
```

If an existing owner can absorb a distinction cleanly, use it. A successful refactor leaves fewer places the next agent must search.

## Evidence law

Never collapse these coordinates:

```text
source fact != musical inference != listener/perceptual claim
exact != derived != hypothesis
artifact identity != work identity != authorship
physical slot != voice episode != persistent part != auditory stream
```

Unknown is not negative evidence. A higher claim may compress support but must retain a route to the evidence and uncertainty beneath it. Cadence/form/attribution layers may not circularly manufacture their own prerequisites.

See `docs/architecture.md` for the full contract.

## Source and abstraction rule

Respect source semantics before normalization. VGM/VGZ logs, SPC snapshots, xSF effective objects, native drivers/sequences, MML/MIDI/tracker source, and audio expose different information.

A mechanism becomes shared only when materially different source families require the same abstraction without erasing useful native distinctions. Shared implementation convenience is not shared semantic law.

## Validation

Default core route:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Prefer focused tests first, then broader suites when the touched surface warrants them. Use real corpus controls when a claim crosses from implementation into preserved music.

Do not call CI green unless a runner actually executed successfully. Do not weaken an acceptance gate to obtain an artifact.

For private foobar2000 VGM/SPC delivery, `.github/workflows/private-foobar-build.yml` and `tools/build_private_foobar_components.ps1` are canonical. A DLL compile or archive creation alone is not publish-ready.

## Documentation rule

Keep active documentation small:

- `README.md` = identity + map + basic commands;
- `AGENTS.md` = agent operating constraints;
- `docs/architecture.md` = durable semantic/evidence architecture;
- `docs/vgm-compiler-roadmap.md` = current frontier and next discriminating work;
- specialized docs = only when they own a real independent contract.

Do not create status diaries, duplicate inventories, generated navigation files, or historical snapshots in the active tree.
