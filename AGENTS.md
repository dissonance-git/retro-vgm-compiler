# VGM Compiler development contract

This file is the canonical operating law for repository work.

> **More capability, fewer conceptual machines.**
>
> **One concept, one writable owner.**
>
> **Living owners keep current obligations. Git keeps the walk.**

Direct user correction or instruction outranks repository prose.

## 1. First-connection skill preflight

Every substantive reasoning-agent entry inherits [`.agents/skills/skill-preflight/SKILL.md`](.agents/skills/skill-preflight/SKILL.md).

Before repository, research, implementation, review, or publication action:

```text
identify VGM Compiler
→ read current AGENTS.md authority
→ read the smallest relevant README.md identity/routing surface
→ enumerate .agents/skills/*/SKILL.md
→ compare skill names/descriptions with the exact obligation
→ select process/control skills first
→ read selected current skill bodies
→ build the smallest sufficient context
→ act
→ verify
```

Do not front-load every skill body. Skill awareness is broad; skill-body retrieval is selective. Current repository bytes outrank remembered procedure.

A plausible applicable skill must be inspected when omitting it could change decomposition, evidence collection, falsification, mutation, concurrency handling, publication, re-entry, or capability-debt handling.

If `AGENTS.md`, `README.md`, `.agents/skills/`, or the task obligation changes materially during a long run, refresh skill awareness.

The first-connection funnel applies to reasoning-agent transports, including GitHub-connector sessions. Low-level exact API callers are not required to imitate an autonomous agent bootstrap.

For GitHub-native work, the preferred bootstrap is derived from current repository state rather than stored as a mutable connector map. When local execution is available, `python tools/github_agent_bootstrap.py --json` emits the exact-head/tree, authority path, skill-preflight owner, live skill count/fingerprint, and bootstrap sequence. Connector-only agents should emulate that packet from exact GitHub evidence at the current head. A stored bootstrap packet is disposable projection state, never repository truth.

## 2. Enter through current authority

For substantive repository work:

```text
current main HEAD
→ README.md
→ AGENTS.md at the same HEAD
→ selected current skills
→ smallest task owner
→ recent commits touching that owner
→ exact code / test / contract
```

For a local checkout, use `python tools/repository_catalog.py --focus <concept>` when its mechanical projection can cheaply narrow the task.

Read `docs/vgm-compiler-roadmap.md` only when current priority/frontier matters. Read durable contracts only when the touched surface depends on them.

Work from `main`. Do not create branches or pull requests unless explicitly requested. Never force-push. Preserve unrelated concurrent work.

## 3. One owner per concept

Canonical roles are:

```text
README.md
  repository identity / routing

AGENTS.md
  repository / evidence / concurrency / publication law

docs/architecture.md
  shared semantic / provenance / evidence / abstraction law

docs/musical-understanding.md
  musical north star

docs/vgm-compiler-roadmap.md
  current unresolved/active frontier

focused docs/*.md
  distinct durable subsystem contracts

research/
  bounded evidence / experiments / controls / preregistration

model/
  source-independent semantics that earned sharing

components/
  source-family/device/driver/execution/rendering implementation

tests/
  executable invariants + immutable real-music controls

tools/
  reusable operations + derived projections

imports/
  immutable external inputs

patches/
  maintained external-source transformations

.github/workflows/
  executable validation / packaging routes

.agents/
  agent procedure / connector re-entry only
```

Generated inventories, search results, task maps, caches, status summaries, connector overlays, and handoff capsules are projections. They are not peer truth stores.

## 4. Semantic collapse and cleanup

Repository cleanup is:

```text
identify current obligations + exact evidence
→ choose one owner for every overlap
→ fold surviving consequences into owners/tests
→ derive recoverable views
→ remove duplicate/completed/lifecycle-shaped surfaces
→ verify inbound routes and protected behavior
→ let Git retain the walk
```

Before keeping or adding a file, registry, cache, abstraction, wrapper, directory, status surface, or research artifact, ask:

1. What current obligation does it uniquely own?
2. Is there already a canonical owner?
3. Can the relation or inventory be derived mechanically?
4. Is the exact object still required evidence or only repository history?
5. Which weaker surface can disappear if this survives?

Prefer existing owner over parallel owner, derive over duplicate, fold over archive, semantic names over lifecycle names, and executable invariants over prose duplication.

Do not create active canonical namespaces such as `new`, `v2`, `final`, `replacement`, `old`, `archive`, `legacy`, `misc`, or `backup` merely to avoid understanding ownership.

Historical material stays tracked only when the exact bytes remain current evidence, provenance, a reproducibility input, an immutable corpus fixture, or an implementation obligation. Otherwise Git history is sufficient.

Never rewrite immutable corpus bytes or imported upstream evidence during cleanup/refactoring.

## 5. Naming and hierarchy

Conventional root files keep conventional names.

New human-facing Markdown, folders, task/handoff keys, route labels, and repository slugs use lowercase kebab-case when external contracts permit it. Python scripts and identifiers use lowercase snake_case. Language-native code identifiers keep their language convention.

Existing external ABI/schema IDs, third-party paths, and compatibility filenames stay exact until a deliberate migration proves every consumer.

Folders own stable semantic categories, not chronology or work sessions.

## 6. Evidence and semantic discipline

Follow `docs/architecture.md` when work crosses source fact, musical inference, perceptual claim, documentary evidence, provenance, identity, or availability.

Preserve source-native semantics before normalization. Shared abstractions are earned only when materially different source families require the same semantic relation without erasing useful native distinctions.

Keep `exact`, `derived`, `hypothesis`, `unknown`, `unavailable`, and `not-applicable` distinct. Unknown is not false. Missing capture is not silence. A hypothesis does not overwrite its support.

Research can pressure a durable contract, but research detail does not become architecture merely because it is extensive.

## 7. Research and corpus law

`research/` may retain exact evidence objects when regeneration is insufficient for the current experiment/reproducibility obligation. Derived caches, matrices, reports, and feature projections remain disposable unless explicitly promoted by their owner.

A named game is normally a control/testbed unless the game itself is the integrative research question.

A promotion from research into shared semantics requires a discriminating route such as:

```text
explicit evidence object
+ provenance / uncertainty
+ executable falsifier
+ independent source-family/corpus pressure when applicable
→ candidate shared capability
```

Do not weaken a corpus or acceptance gate merely to make a run complete.

## 8. Repository mutation and concurrency

Reasoning-agent GitHub entry is:

```text
connect
→ identify VGM Compiler
→ derive/fetch current agent bootstrap
→ read AGENTS.md authority
→ run live skill preflight
→ state exact obligation
→ acquire smallest sufficient context
→ act
→ verify
```

A GitHub transport must not present the repository as a generic tool bag while omitting this funnel.

Repository changes use [`.agents/skills/repo-change/SKILL.md`](.agents/skills/repo-change/SKILL.md).

GitHub-backed work additionally uses [`.agents/skills/github-workspace/SKILL.md`](.agents/skills/github-workspace/SKILL.md). Long or hot-`main` connector work also uses [`.agents/skills/github-workspace-liveness/SKILL.md`](.agents/skills/github-workspace-liveness/SKILL.md).

The operating loop is:

```text
observe exact state
→ orient narrowly
→ stage an overlay
→ verify what can actually execute
→ refresh awareness of current main
→ continue from the newest accepted head
```

Remote movement is awareness before conflict. Path-disjoint concurrent work may still introduce useful evidence, tests, helpers, or owners. Absorb relevant positive interference without discarding unaffected task progress.

Validation belongs to an exact target SHA. A pass from one commit does not transfer to another.

For substantial direct-main agent commits, use retrospective routing trailers after the actual diff/validation state is known:

```text
vgm-task: <lowercase-kebab-case-key>
vgm-change-kind: <actual-landed-kind>
vgm-validation: <actual-validation-state>
vgm-handoff: <optional issue numbers>
```

These trailers are coordination hints, not evidence.

## 9. Runtime capability and Codex handoff

Repository control and fresh runtime execution are separate capabilities.

Finish all repository-native work first. If a concrete actionable remainder is blocked specifically by the current interface/runtime, use [`.agents/skills/codex-handoff/SKILL.md`](.agents/skills/codex-handoff/SKILL.md).

The authoritative capability-debt queue is open GitHub issues with:

```text
title prefix: CODEX:
body marker: <!-- vgm-compiler-codex-handoff:v1 -->
```

Do not create a parallel JSON/Markdown queue.

## 10. Verification

Default core route:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Prefer focused tests first, then broader suites proportionate to the change. Use real corpus controls when a claim crosses from implementation into preserved music.

Keep source correctness, semantic correctness, compile/link success, unit/integration tests, real-corpus behavior, package/runtime verification, and perceptual/listening validation separate.

Do not call CI green unless a runner executed successfully. A workflow not planned, runner/backend startup failure, workflow runtime failure, test failure, and successful execution are different states.

Private foobar2000 VGM/SPC delivery is owned by `.github/workflows/private-foobar-build.yml` and `tools/build_private_foobar_components.ps1`. A DLL compile or archive alone is not delivery proof.

## 11. Completion

Before publication, refresh `main`, inspect changed supporting/protected premises, preserve concurrent work, inspect the exact candidate diff, run or route proportionate validation, and remove duplicate surfaces made redundant by the change.

After publication, re-fetch `main`, verify the intended commit and paths/content, inspect target-SHA validation, and report only what actually executed.

Do not create a history document to memorialize completed cleanup. The repository should be cheaper to understand and safer to resume after every change.
