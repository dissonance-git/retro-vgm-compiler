# Retro VGM Compiler

Executable understanding, musical analysis, source-native rendering, perceptual organization, and human-readable reasoning for digital game music.

## Objective

Retro VGM Compiler follows the transformations that actually produce and are heard as game music instead of flattening every source into MIDI, stems, final PCM, or a detached analytical summary.

**The primary objective is holistic musical understanding.** Exact decoding, execution tracing, synthesis reconstruction, source isolation, driver archaeology, and provenance are supporting machinery. They matter because they improve, constrain, explain, or validate the musical model.

The compiler architecture and current/future roadmap live in `docs/retro-vgm-compiler-roadmap.md`.

The strongest composer-facing benchmark is deliberately difficult:

> If a game credits several composers but leaves some cue authorship unresolved, can the system understand the music deeply enough to learn each candidate's recurring grammar across independent works and different soundtracks, then attribute a held-out cue for defensible musical reasons?

That benchmark is downstream of understanding, not a replacement for it.

```text
native encoded truth
        ↓
authoring language / source project / sequence semantics
        ↓
compiler / assembler / conversion tool
        ↓
driver program / logical tracks / scheduling / arbitration
        ↓
device / synthesis / sample state
        ↓
performed gestures and persistent musical parts
        ↓
melody • bass • rhythm • harmony • timbre • articulation
        ↓
phrase • motif • cadence • counterpoint • form • arrangement
        ↓
integrated cross-representation song model
        ↓
recurring rules across independent works
        ↓
cross-soundtrack composer grammar
        ↓
blind composer-attribution stress tests
        ↓
holistic soundtrack understanding and human explanation
```

The project is vertically end-to-end. The lowest useful fact may be a bit field, byte, address, pointer rule, register write, sample, driver state transition, or machine cycle. The highest useful result may be an integrated explanation of what a cue is doing musically, how it relates to the soundtrack, and which creator-specific behaviors recur in other projects.

## Two generalization axes

Composer-level understanding must generalize in two directions.

### Same work across representations

```text
MIDI / MML / tracker / native sequence / source ASM
↕
driver program / compiled sequence / runtime state
↕
VGM / SPC / PSF-family execution
↕
patches / samples / synthesis state
↕
rendered audio / auditory organization
↕
external transcription / documentary evidence
```

Each representation is a sensor. They should cross-check and teach one another where a real correspondence exists.

But they are not interchangeable:

```text
MIDI track
!= MML voice
!= tracker channel
!= driver logical track
!= physical chip channel
!= physical voice episode
!= persistent musical part
```

Likewise:

```text
MIDI program != FM patch != BRR sample != tracker instrument
```

A correspondence may be strong without becoming an equivalence.

### Same composer across different soundtracks

A composer model built from one soundtrack can accidentally learn that soundtrack's driver, patch/sample bank, arranger, platform, production period, cue functions, or related themes.

The stronger question is:

> What musical behaviors follow the composer when the soundtrack around them changes?

Prefer controls from different games/soundtracks, platforms, collaborators, and career periods whenever possible.

Useful validation includes:

```text
leave-one-work-family-out
leave-one-soundtrack-out
leave-one-platform-out
leave-one-arranger-out
leave-one-career-period-out
```

The strongest evidence often appears where both axes agree: a creator-specific musical relation is recovered consistently through several representations and also recurs across unrelated soundtracks.

See `docs/composer-level-understanding.md`, `docs/holistic-musical-understanding.md`, and `research/music/composer-grammar-attribution.md`.

## Composer grammar is multi-view

There is no privileged composer representation.

Potential creator-specific evidence can come from several views:

```text
STRUCTURE
melody • bass • harmony • rhythm • phrase • motif • counterpoint • cadence • form

ARRANGEMENT
register • density • doubling • role assignment • countermelody • orchestration

TIMBRE / SYNTHESIS
patch/sample choices • envelopes • modulation • timbral contrast • form-linked synthesis

PERFORMANCE / EXECUTION
articulation • pitch control • attack/release • dynamics • microtiming • negative space

SOUNDTRACK RELATIONSHIPS
thematic reuse • cue families • transformation strategies • recurring dramatic solutions
```

The strongest traits are relational rather than isolated counts.

```text
uses syncopation
```

is weaker than:

```text
repeatedly delays the same melodic arrival across harmonic contexts,
then resolves the displacement at a formal return
```

A creator-specific relation may span several views:

```text
retained melodic cell
+ changed bass motion
+ widened register
+ brighter timbral assignment
+ delayed cadence
→ characteristic return strategy
```

## Role scope, not modality censorship

Game music often distributes creative work across several people.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= final realization
```

This does not mean timbre, arrangement, synthesis, or execution are forbidden from composer attribution. It means **every observation carries role provenance**.

A patch, articulation, orchestration, or control habit can support composer attribution when historical evidence shows that the composer authored or reliably controlled that layer. The same feature may instead belong to an arranger, programmer, shared library, driver, or platform in another soundtrack.

A legitimate result may therefore be:

```text
cross-representation composer grammar → composer A
arrangement/programming subgrammar   → programmer B
patch/sample vocabulary              → shared team/library
```

These are complementary claims, not one averaged `artist` score.

## Composer evolution

A composer is not a frozen centroid.

The model should distinguish:

```text
stable long-range habits
career-period habits
soundtrack-local habits
collaborator-dependent habits
platform-dependent habits
one-off experiments
```

A composer grammar is a structured region with trajectories, not one static fingerprint.

## Evidence boundaries

The project deliberately keeps identities and analytical levels separate.

```text
physical slot != voice episode != persistent musical part != auditory stream
```

```text
register frequency
!= nominal frequency
!= programmed pitch
!= transposed pitch
!= performed pitch
!= heard pitch
!= notation spelling
```

```text
simultaneous pitches
!= chord spelling
!= harmonic function
!= cadence
!= form
!= composer grammar
!= authorship
```

Higher analysis may summarize lower evidence, but it may not erase uncertainty underneath it.

The same rule now applies explicitly above chip execution:

```text
same chip state != same upstream intention
same opcode != same meaning across driver / scope / revision
same driver family != same revision behavior
same physical channel != same logical musical part
transformed runtime != authoring source
reconstructed source candidate != exact authored source
```

## Source families

Current work spans materially different representations and architectures, including:

- VGM/VGZ command streams;
- SPC snapshots and S-DSP state;
- NSF and other executable-rip formats;
- PSF1, GSF, USF, 2SF, and NCSF executable-object families;
- native music drivers and sequence formats;
- MIDI, MML, tracker source, source ASM, and other symbolic music representations;
- ROM-derived samples, patches, sequences, executables, and control data;
- rendered audio and documentary evidence.

These remain source-specific until independent systems force the same abstraction.

PSF1, GSF, USF, 2SF, and NCSF share an xSF envelope/dependency mechanism here, but their effective objects remain platform-specific executable or memory objects. A reconstructed effective object is not automatically an understood driver, sequence, part structure, or musical interpretation.

The common execution substrate lives in `model/`. Source-specific work lives under `components/` and retains its own timing, device, driver, and provenance semantics.

## Driver and toolchain ancestry

Accurate chip playback is not the top of the causal graph. On systems such as the Mega Drive/Genesis, very different software worlds can converge on the same YM2612 + SN76489 + DAC hardware.

Retro VGM Compiler therefore treats the authoring and driver cloud as first-class evidence:

```text
authored source / MML / tracker / ASM
        ↓
compiler / assembler / conversion tool
        ↓
driver dialect + revision
        ↓
logical tracks / envelopes / articulation / control flow
        ↓
channel arbitration / PCM ownership / timing
        ↓
chip writes
```

Driver identity is not a decorative label. Revision can change command meaning, pointer interpretation, note/rest continuation, timing arithmetic, modulation behavior, envelope arithmetic, pitch tables, instrument layout, and DAC playback timing.

Recent SMPS research-pack inspection strengthened this with concrete differential evidence across pre-SMPS, 68k SMPS, Z80 Type 1/Type 2, Sonic-family revisions, DAC subdrivers, prototypes, 32X branches, and other variants. The transport archives were inspected and deleted; only the useful research conclusions remain.

Particularly important findings include:

- 68k, Ristar-like, and Z80 pointer rules differ;
- identical command bytes can mean different operations in different SMPS families;
- tick multiplier behavior differs in how it affects note timeout;
- note/rest/delay continuation differs between 68k and Z80 semantics;
- envelope multipliers and modulation algorithms differ across pre-SMPS, 68k, and Z80;
- FM operator ordering and FM/PSG frequency tables can be dialect-specific;
- DAC sample rate may be an emergent property of driver loop/timer timing rather than an explicit source field;
- old extraction tools contain useful structural ideas for finding driver images and pointer tables in ROMs, but their labels must be independently validated.

The strongest future experiment is not merely another disassembly. It is a paired forward/inverse test:

```text
known source-native sequence + known driver
→ execute
→ hide source
→ observe device-facing trajectory
→ blind reconstruct upstream semantics
→ compare with hidden answer key
```

Every field is scored independently as exact, non-unique, ambiguous, lost, or wrong.

See:

- `research/formats/genesis/genesis-driver-source-ledger.md`
- `research/formats/genesis/smps-research-pack-harvest.md`
- `research/formats/genesis-driver-dialect-census.md`
- `research/formats/genesis-open-driver-anatomy.md`
- `research/formats/genesis/genesis-authoring-driver-toolchain-quarry.md`
- `research/formats/genesis/genesis-driver-source-vgm-boundary.md`

## Real corpus

The permanent corpus is a scientific control surface with immutable files, hashes, provenance, and manifest metadata.

It spans Genesis and SNES material plus controls for multiple Yamaha, PSG, Nintendo, Namco, Sega, Capcom, PlayStation, GBA, N64, and DS source/device families.

See `tests/CORPUS.md`, `tests/corpus/README.md`, and `tests/corpus/manifest.json`.

A source that breaks an assumption is as valuable as one that confirms it.

> **Shared abstractions should be discovered by agreement and disagreement.**

## Driver and tracker observatories

Source-available trackers, compilers, engines, reconstructed drivers, historical disassemblies, and driver extraction tools expose the semantic layer between symbolic music and hardware writes.

They help establish mechanisms such as:

```text
source note / instrument / effect
+ previous state
+ control flow
+ driver timing
+ dialect / revision
        ↓
performed device trajectory
```

They do not prove that an unrelated commercial soundtrack used the same toolchain. Historical linkage must be established independently.

A project-native driver detector should therefore emit evidence and confidence rather than silently mapping a ROM filename to a driver name:

```text
ROM / executable bytes
→ structural loader evidence
→ candidate driver image
→ code-family evidence
→ bounded dialect/revision hypothesis
→ pointer/table recovery
```

See `research/validation/game-music-driver-observatories.md` and the Genesis driver documents above.

## Musical understanding

Higher claims are dependency-aware rather than vocabulary-first.

```text
performance evidence
↓
persistent parts
↓
melody / bass / accompaniment / inner voices
↓
harmony / rhythm / timbre / articulation
↓
voice leading / counterpoint
↓
cadence / phrase
↓
motivic and formal relations
↓
integrated song model
↓
recurring rules across works and soundtracks
↓
creator grammar
```

The goal is not to concatenate separate analyses. The goal is to understand how those dimensions cooperate to make a passage, cue, soundtrack, and creator's broader body of work behave musically.

## Attribution evaluation discipline

For a game with several plausible credited composers:

1. freeze the candidate set before inspecting disputed cues;
2. keep disputed cues completely out of model construction;
3. group prototypes, ports, reprises, arrangements, and derivative cues by work family;
4. gather secure candidate controls from other soundtracks where possible;
5. build multi-view creator grammars rather than score-only fingerprints;
6. use matched controls such as same driver/different composer and same composer/different soundtrack;
7. run leave-one-soundtrack-out and leave-one-platform-out tests;
8. intervene on patch, timbre, platform, tempo, transposition, soundtrack-local, and arranger/programmer cues;
9. require supporting and contradicting musical explanations;
10. allow abstention when evidence does not converge.

A desired result looks more like:

```text
composer A probable
  cross-soundtrack phrase/form relation: strong support
  melodic-development relation: strong support
  bass/harmony relation: medium support
  timbral relation: weak, likely collaborator-dependent
  cadence behavior: counterevidence
  survives patch masking and leave-one-soundtrack-out

arranger/programmer B probable
  realization grammar: strong support
```

See `research/music/composer-grammar-attribution.md` and `research/music/musicological-authorship-attribution.md`.

## Human musical discourse

Human-facing language is a projection over evidence, not another truth layer.

The aspirational standard is composer-grade structural understanding without invented intent: explain why a passage works, how material develops, how arrangement and timbre reinforce structure, how a cue participates in its soundtrack, and why a held-out cue resembles one creator's recurring grammar across other soundtracks.

See `docs/human-musical-discourse.md`, `docs/composer-level-understanding.md`, and `docs/holistic-musical-understanding.md`.

## Source-native enhanced rendering

The accurate/reference renderer is the scientific control, not the quality ceiling.

Enhanced rendering asks whether an implementation ceiling can be relaxed while preserving the same musical object, parts, gestures, timing relationships, and arrangement.

See `docs/source-native-enhanced-rendering.md`.

## Source-aware immersive playback

When a source family can expose real causal voices or channels, Retro VGM Compiler can hand those objects to Omniphony before historical stereo collapse.

The audible Surround goal is intentionally creative:

> **Make the soundtrack sound as though those real sources had always been mixed for a larger immersive format.**

This does not change source provenance. Native route, timing, identity, effects structure, and genuinely authored position remain source facts. Width, rear depth, height, distance, and source extent may be modern `DERIVED` presentation choices in Omniphony `FullSphere`.

```text
source-native truth
→ recovered causal objects
→ authored route / timing / identity
→ musical presentation evidence
→ Omniphony FullSphere
→ 8.1.4.4 semantic world + dynamic objects
→ 22-direction shell
→ binaural
```

The compiler does not manufacture 17 speaker-bed lanes. Omniphony does not decide which emulator or reconstruction is truthful. The protected historical/reference path remains available as the control.

See `docs/omniphony-realtime-spatial-path.md`.

## Installable foobar2000 component delivery

Retro VGM Compiler owns the reproducible build and publication path for the private VGM and SPC decoder components. A successful compile is not a delivery. The canonical path is:

```text
current main + pinned upstream inputs
→ source/materialization regression guards
→ VGM x64 decoder build
→ patched x86 SNESAPU build
→ source-aware x86 spcplayer build
→ SPC x64 decoder build
→ .fb2k-component packaging
→ package structure / PE / ABI verification
→ Windows packaged-runtime verification
→ combined bundle verification
→ packaged SNESAPU provider/runtime smoke
→ GitHub Actions artifact upload
→ rolling verified delivery release
```

Production owners:

- `.github/workflows/private-foobar-build.yml` owns the end-to-end Windows delivery gate.
- `tools/build_private_foobar_components.ps1` owns the pinned build and packaging procedure.
- `tools/run_private_foobar_build_with_diagnostics.ps1` exposes focused failures without weakening the real exit status.
- `tools/verify_private_component_packages.py` verifies exact component payloads, PE architecture/exports, private import boundaries, Omniphony ABI, and packaged child startup.
- `tools/verify_private_component_bundle.py` verifies the final combined delivery envelope and byte-for-byte agreement with its embedded/manual runtime copies.
- `tools/verify_snesapu_package_runtime.ps1` owns the packaged x86 SNESAPU provider/runtime smoke.

Installable decoder outputs:

```text
foo_input_vgm.private.fb2k-component
  foo_input_vgm.dll
  omniphony_source.dll

foo_snesapu.private.fb2k-component
  foo_snesapu.dll
  spcplayer.exe
  SNESAPU.dll
  omniphony_source.dll
```

`private-foobar-vgm-spc.zip` intentionally also contains `VGM/` and `SPC/` runtime folders for manual replacement. Those folders are part of the declared bundle format; the two `.fb2k-component` files themselves remain flat sibling packages.

A decoder pair is **publish-ready** only when the GitHub Actions job is green through build, component verification, bundle verification, packaged SNESAPU runtime smoke, artifact upload, and rolling delivery publication. A loose DLL, a successful compiler invocation, or an archive created before those gates is not the published test deliverable.

The Omniphony output component is built and published by the Omniphony repository. The matched listening set is therefore:

```text
foo_input_vgm.private.fb2k-component
+ foo_snesapu.private.fb2k-component
+ foo_out_omniphony.fb2k-component
```

Decoder `enhanced` and `Surround` remain independent controls. `enhanced` changes source reconstruction/quality; `Surround` requests Omniphony source-aware spatial presentation. Packaging or output selection must never collapse them.

## Current Genesis execution frontier

The driver/source research does not replace the current empirical execution program.

The immediate Genesis target remains:

```text
VGZ bytes
→ pinned/patched libvgm PlayerA + source-aware capture
→ exact FM1-6 + DAC + PSG1-4 source planes
→ Genesis realtime musical/Omniphony pipeline
→ passive ABI 0.4 renderer
→ block + continuity validators
→ creator/game/title-blind JSON sidecar
→ local SHA-driven corpus orchestrator
```

The frozen real control surface is the 58-file Sonic 3 & Knuckles VGZ set already preregistered for the spatial-governor experiment. Driver archaeology may improve future semantic calibration, but it must not move preregistered thresholds or promote pair-aware presentation controls before the real corpus executes and passes.

## Relationship to other projects

- **Helix** supplies shared research execution, provenance discipline, and project continuity.
- **Retro VGM Compiler** owns game-music source, driver, device, performance, analysis, source-native rendering, and source evidence.
- **libaural** is the general artificial-hearing research layer.
- **Omniphony** owns the creative immersive presentation, canonical 8.1.4.4 world, full-sphere render shell, and headphone spatial rendering.

Chip-specific machinery stays here unless it becomes genuinely general.

## Repository map and navigation

Fast orientation belongs here rather than in a separate root catalog. This map is a projection, not source truth.

> **One canonical home per object. Many routes to it.**

Use this entry path before a repository-wide search:

```text
current main
→ README.md                         project identity + repository map
→ AGENTS.md                         operating law
→ docs/retro-vgm-compiler-roadmap.md
→ smallest canonical owner below
→ recent commits for that surface
→ exact target code, test, or document
```

Repository shelves:

```text
model/          shared musical/evidence semantics that earned sharing
components/     source-family and device-specific machinery
tests/          regressions + immutable real corpus
research/       bounded investigations and named testbeds
docs/           current architecture / reasoning contracts
tools/          reusable executable operations
imports/        preserved upstream/import provenance
patches/        maintained patch material
```

Dense shelves have their own `README.md`. Descend there before searching globally.

Canonical routes:

| Need | Route |
| --- | --- |
| project objective | this `README.md` |
| working rules | `AGENTS.md` |
| current frontier | `docs/retro-vgm-compiler-roadmap.md` |
| implemented source family | `components/README.md` → family |
| shared musical semantics | `model/README.md` → exact model header/test |
| game-music → Omniphony spatial handoff / 8.1.4.4 authority | `docs/omniphony-realtime-spatial-path.md` |
| installable private VGM/SPC decoder delivery | this README section + `AGENTS.md` delivery law |
| existing corpus / soundtrack | `tests/corpus/README.md` → `manifest.json` → set |
| research program | `research/README.md` → owning trunk/project |
| existing utility | `tools/README.md` → exact tool |
| current documentation | `docs/README.md` |
| historical project prose | `docs/history/` |
| mechanical repository inventory | `tools/repository_catalog.py` |

For exact mechanical enumeration:

```text
python tools/repository_catalog.py

→ docs/generated/repository-catalog.md
→ docs/generated/repository-catalog.json
```

Generated inventories are rebuildable projections. They do not become authority or another writable repository map.

For the Sonic 3 attribution program, enter through `research/projects/sonic3/README.md`, then the canonical policy/admission file and frozen preregistration for the exact generation. `attribution-control-admissions.jsonl` is grounded role evidence, `cube-calibration-policy.json` owns CUBE calibration/holdout rules, `role-credit-index.jsonl` is Genesis cache-routing control rather than master history, and `research/cache/` is derived creator-blind analysis. Never reconstruct creator labels from corpus tags when a canonical admission/policy already exists.

Addition rule:

```text
canonical owner exists? → extend it
otherwise               → choose the smallest existing shelf
new peer object          → only when a real new distinction needs an owner
```

A successful refactor leaves fewer places an agent must search.

Start with:

- `AGENTS.md`
- `docs/retro-vgm-compiler-roadmap.md`
- `docs/holistic-musical-understanding.md`
- `docs/composer-level-understanding.md`
- `research/README.md`
- `research/formats/genesis/genesis-driver-source-ledger.md`
- `research/formats/genesis/smps-research-pack-harvest.md`
- `research/music/composer-grammar-attribution.md`
- `docs/musical-execution-model.md`
- `docs/musical-inference-evidence.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/human-musical-discourse.md`
- `docs/source-native-enhanced-rendering.md`
- `docs/omniphony-realtime-spatial-path.md`

## Testing

Core regressions are registered through CMake and can also be exercised with `tools/run_core_tests.py`.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Important mechanisms should also be challenged by real corpus controls, negative controls, independent implementations, matched-decoy tests, revision differentials, cross-soundtrack holdouts, and confound interventions.

## Working rules

1. Holistic musical understanding is the primary objective.
2. Blind composer attribution is a capstone stress test of understanding, not a shortcut objective.
3. Learn one work across all useful representations while preserving source-native semantics.
4. Learn one composer across independent works and different soundtracks whenever possible.
5. There is no privileged score/MIDI representation for composer identity.
6. All modalities may contribute, but every contribution carries role provenance.
7. Keep composition, arrangement/programming, driver/toolchain, patch/sample, and realization attribution distinct.
8. Recover symbolic note/sequence information whenever the source supports it.
9. Preserve authoring source, compiled sequence, runtime sequence, transformed runtime, and register capture as distinct artifact roles.
10. Preserve driver family, dialect/revision, semantic scope, timing domain, and capability state when they affect interpretation.
11. Never infer command semantics from opcode bytes without the required dialect/scope context.
12. Treat executable PCM timing as part of the performed object when the driver defines playback rate implicitly.
13. Use same-composer cross-soundtrack and cross-platform controls to distinguish creator invariants from project artifacts.
14. Group related versions and derivative cues so the system cannot win by recognizing the work.
15. Actively intervene on timbre, patch/sample identity, platform, tempo, transposition, soundtrack-local, and arranger/programmer confounders.
16. Preserve encoded/source, authored, driver, device, sample, acoustic, perceptual, and listener-model distinctions where they exist.
17. Keep exact, derived, inferred, perceptual, and external claims distinct.
18. Do not call a physical channel a persistent musical part without evidence.
19. Do not jump from low-level pitch directly to harmony, creator grammar, or authorship.
20. A correct composer label without a traceable musical explanation is not sufficient evidence of understanding.
21. Composer evolution is expected; do not force all works into one static centroid.
22. Unknown is not unsupported; missing evidence is not negative evidence.
23. Corrections outrank narrative coherence.
24. Accuracy/reference behavior remains available beneath every enhancement.
25. Traceability supports understanding but does not substitute for it.

> **Understand each musical work across its representations, then understand each composer across different soundtracks deeply enough that authorship can emerge as a consequence of the music rather than the production environment.**
