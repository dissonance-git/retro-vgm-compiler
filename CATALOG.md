# Retro VGM Compiler catalog

This is the **fast navigation projection** for the repository. It is deliberately smaller than the repository and does not own technical truth.

> **One canonical home per object. Many useful routes to it.**

> **More capability, fewer conceptual shelves.**

Use this file to answer **“do we already have this, and where does it live?”** before doing a repository-wide search.

Canonical files, source objects, tests, manifests, and research records remain authoritative. This catalog only makes them easier to reach.

## Fresh-agent entry

```text
current main
→ README.md            project identity and objective
→ AGENTS.md            operating law
→ CATALOG.md           what exists and where to look
→ docs/retro-vgm-compiler-roadmap.md
→ relevant canonical owner
→ recent commits only for the active surface
```

Do not reread the whole repository to start ordinary work. Enter through the smallest route below that preserves the distinctions needed for the task.

## Repository shape

```text
README.md               declares the project
AGENTS.md               governs work
CATALOG.md              navigates the current repository

model/                  shared provenance-aware musical semantics
components/             source-family and device-specific machinery
tests/                   executable regressions + immutable real corpus
research/                bounded investigations and named testbeds
docs/                    durable architecture and musical reasoning contracts
tools/                   corpus, audit, extraction, evaluation, and maintenance utilities
imports/                 preserved upstream/import provenance
patches/                 maintained downstream/upstream patch material
cmake/                   build/test registration support
.github/                 CI and bounded workflows
```

This is **one architecture**. VGM, SPC, xSF, composer attribution, rendering, and corpus work are capabilities inside it, not peer projects.

## Canonical ownership

| Question | Start here | Then descend to |
| --- | --- | --- |
| What is the project trying to understand? | `README.md` | `docs/holistic-soundtrack-understanding.md` |
| What rules govern changes? | `AGENTS.md` | task-specific test/research owner |
| What is the current build/research frontier? | `docs/retro-vgm-compiler-roadmap.md` | relevant component/research project |
| What source families are implemented? | `components/` | family README/module/tests |
| What musical semantics are shared? | `model/` | semantic tests in `tests/` |
| What real music do we already have? | `tests/corpus/README.md` | `tests/corpus/manifest.json`, exact corpus directory |
| What research programs already exist? | `research/README.md` | the owning research trunk/project |
| What reusable command already exists? | `tools/` | tool tests / owning research doc |
| Where is Sonic 3 work? | `research/projects/sonic3/` | project README, role-credit index, preregistrations, execution artifacts |
| Where is creator-blind cached analysis? | `research/cache/` | `tools/creator_blind_song_cache.py` |
| Where are historical bootstrap/lineage docs? | `docs/history/` | exact historical document |

## Source-family shelf

Current first-class component families:

```text
components/
├── vgm/      VGM/VGZ logged execution and device state
├── spc/      SPC/SNES snapshot, S-DSP, BRR, Studio/source-aware reconstruction
├── psf/      PSF1 / PlayStation effective-object and SPU work
├── gsf/      GSF / Game Boy Advance xSF-derived objects
├── usf/      USF / Nintendo 64 objects
├── twosf/    2SF / Nintendo DS objects
├── ncsf/     NCSF / Nintendo DS selected-SDAT objects
└── xsf/      shared xSF envelope/dependency semantics only
```

Other executable-rip and chip families may currently live primarily in corpus/audit tooling until they earn a dedicated component. **A corpus family is not automatically an implemented semantic frontend.**

## Research geography

`research/` is intentionally compressed into durable programs:

```text
research/
├── projects/     integrative named testbeds, currently led by Sonic 3
├── music/        musical understanding, composer grammar, attribution methods
├── runtime/      execution, state, synthesis, sample, and control semantics
├── formats/      source/driver/chip/platform investigations
├── validation/   observatories, controls, external comparisons, pressure tests
├── rendering/    fidelity, reconstruction, equivalence, counterfactual rendering
├── enhancement/  bounded enhanced-rendering experiments
└── cache/        reusable derived analysis objects; never source truth
```

Placement rule: extend an existing trunk before creating another peer-level document. A game gets a project folder only when it is itself an integrative testbed.

## Sonic 3 / creator-attribution route

Use this route instead of searching the repo from scratch:

```text
research/projects/sonic3/README.md
→ role-credit-index.jsonl
→ relevant frozen preregistration
→ research/cache/README.md
→ tools/creator_blind_song_cache.py
→ tools/build_admitted_composer_caches.py
→ execution/ artifacts for the exact generation
```

Role firewall always survives navigation compression:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver programming
!= patch / sample design
!= final realization
```

External tags can route candidates without proving any of those roles.

## Corpus shelf

`tests/corpus/` is permanent experimental apparatus, not miscellaneous test data.

As documented in `tests/corpus/README.md`, the current corpus contains **63 sets, 1,019 runnable fixtures, and 70 playlist sidecars**. It spans VGM/VGZ, SPC, NSF/NSFe, KSS/SGC, PSF1, GSF, USF, 2SF, NCSF, Genesis/SN76489/YM2612 controls, many Yamaha/arcade chips, Nintendo families, and cross-soundtrack composer controls.

Before acquiring another soundtrack or format, check:

1. `tests/corpus/README.md`
2. `tests/corpus/manifest.json`
3. generated inventory from `tools/repository_catalog.py`

The repository already contains high-value composer/control worlds including Golden Axe III, Sonic 3D Blast, J.League Pro Striker 1/2, Dr. Robotnik's Mean Bean Machine, Ancient Magic, Battle Master, Terranigma, Ghox, several Miyoko Takaoka controls, several Masayuki Nagao controls, several Masaru Setsumaru controls, and the sealed Sonic 3 & Knuckles set.

## Tool shelf

Prefer an existing tool over a new one. Tool names are intentionally descriptive enough to route by job.

### Corpus and inventory

```text
tools/corpus_import.py
tools/repository_catalog.py
tools/inspect_spc_collection.py
```

### Creator / attribution

```text
tools/attribution_control_registry.py
tools/blind_attribution_match_manifest.py
tools/creator_blind_song_cache.py
tools/build_admitted_composer_caches.py
tools/cross_soundtrack_vgm_audit.py
tools/maeda_calibration_*.py
tools/vgm_creator_feature_audit.py
```

### Source-family audits and semantics

Use the family/tool prefix first: `vgm_*`, `genesis_*`, `spc_*`, `nsf_*`, `xsf_*`, `z80_*`, etc. The generated catalog enumerates the exact current filenames.

### Build / maintenance

```text
tools/run_core_tests.py
tools/check_project_identity.py
tools/check_libvgm_patches.py
```

## Documentation compression law

Top-level `docs/` is for **current durable contracts and active orientation**.

```text
docs/<active-contract>.md     current architecture / semantics / roadmap
docs/history/                 superseded bootstrap and lineage material
docs/generated/               rebuildable inventory projections
```

Do not keep a historical handoff beside current contracts merely because it used to be important. Preserve it under `docs/history/` and point to the current owner.

## Generated inventory

`tools/repository_catalog.py` creates deterministic, read-only repository inventory projections from the checked-out tree:

```text
python tools/repository_catalog.py

→ docs/generated/repository-catalog.md
→ docs/generated/repository-catalog.json
```

The generated catalog answers mechanical questions such as:

- which component families exist;
- which corpus IDs exist;
- which research trunks/projects exist;
- which tools already exist;
- how many tracked files occupy each major shelf.

It deliberately does **not** hash every file or duplicate corpus provenance. Corpus identity remains owned by `tests/corpus/manifest.json` and adjacent inventories.

## Addition rule

Before adding a file or abstraction:

```text
Does a canonical owner already exist?
    yes → extend it
    no  → identify the smallest existing shelf that owns the new distinction
            ↓
         create a new object only if the distinction genuinely needs one
```

A useful repository refactor leaves fewer places an agent must search, not more names for the same thing.
