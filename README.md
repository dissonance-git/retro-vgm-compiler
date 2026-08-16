# Game Music Interpreter

Executable understanding, musical analysis, source-native rendering, perceptual organization, and human-readable reasoning for digital game music.

## Objective

Game Music Interpreter follows the transformations that actually produce and are heard as game music instead of flattening every source into MIDI, stems, final PCM, or a detached analytical summary.

**The primary objective is holistic musical understanding.** Exact decoding, execution tracing, synthesis reconstruction, source isolation, and provenance are supporting machinery. They matter because they can improve, constrain, explain, or validate the musical model.

The strongest composer-facing benchmark is deliberately difficult:

> If a game credits several composers but leaves some cue authorship unresolved, can the system understand the music deeply enough to learn each candidate's recurring grammar across independent works and different soundtracks, then attribute a held-out cue for defensible musical reasons?

That benchmark is downstream of understanding, not a replacement for it.

```text
native encoded truth
        ↓
format / memory / sequence semantics
        ↓
driver and program execution
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

The project is vertically end-to-end. The lowest useful fact may be a bit field, byte, address, register write, sample, or machine-state transition. The highest useful result may be an integrated explanation of what a cue is doing musically, how it relates to the soundtrack, and which creator-specific behaviors recur in other projects.

## Two generalization axes

Composer-level understanding must generalize in two different directions.

### Same work across representations

```text
MIDI / MML / tracker / native sequence
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

```text
composer A
├── soundtrack 1
├── soundtrack 2
├── soundtrack 3
└── soundtrack 4
```

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

This does not mean timbre, arrangement, synthesis, or execution are forbidden from composer attribution.

It means **every observation carries role provenance**.

A patch, articulation, orchestration, or control habit can support composer attribution when historical evidence shows that the composer authored or reliably controlled that layer. The same feature may instead belong to an arranger, programmer, shared library, driver, or platform in another soundtrack.

A legitimate result may therefore be:

```text
cross-representation composer grammar → composer A
arrangement/programming subgrammar → programmer B
patch/sample vocabulary → shared team/library
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

## Source families

Current work spans materially different representations and architectures, including:

- VGM/VGZ command streams;
- SPC snapshots and S-DSP state;
- NSF and other executable-rip formats;
- PSF1, GSF, USF, 2SF, and NCSF executable-object families;
- native music drivers and sequence formats;
- MIDI, MML, tracker source, and other symbolic music representations;
- ROM-derived samples, patches, sequences, and control data;
- rendered audio and documentary evidence.

These remain source-specific until independent systems force the same abstraction.

PSF1, GSF, USF, 2SF, and NCSF share an xSF envelope/dependency mechanism here, but their effective objects remain platform-specific executable or memory objects. A reconstructed effective object is not automatically an understood driver, sequence, part structure, or musical interpretation.

The common execution substrate lives in `model/`. Source-specific work lives under `components/` and retains its own timing, device, driver, and provenance semantics.

## Real corpus

The permanent corpus is a scientific control surface with immutable files, hashes, provenance, and manifest metadata.

It spans Genesis and SNES material plus controls for multiple Yamaha, PSG, Nintendo, Namco, Sega, Capcom, PlayStation, GBA, N64, and DS source/device families.

See `tests/CORPUS.md`, `tests/corpus/README.md`, and `tests/corpus/manifest.json`.

A source that breaks an assumption is as valuable as one that confirms it.

> **Shared abstractions should be discovered by agreement and disagreement.**

## Driver and tracker observatories

Source-available trackers, compilers, engines, and reconstructed drivers expose the semantic layer between symbolic music and hardware writes.

They help establish mechanisms such as:

```text
source note / instrument / effect
+ previous state
+ control flow
+ driver timing
        ↓
performed device trajectory
```

They do not prove that an unrelated commercial soundtrack used the same toolchain. Historical linkage must be established independently.

See `research/validation/game-music-driver-observatories.md`.

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

## Relationship to other projects

- **Helix** supplies shared research execution, provenance discipline, and project continuity.
- **Game Music Interpreter** owns game-music source, driver, device, performance, analysis, and source-native rendering semantics.
- **libaural** is the general artificial-hearing research layer.
- **Omniphony** is the general headphone spatial renderer.

Chip-specific machinery stays here unless it becomes genuinely general.

## Repository map

```text
model/          shared provenance-aware primitives
components/     source- and device-specific execution/analysis
tests/          executable regressions and real-music corpus
research/       program-organized mechanism and evidence investigations
docs/           durable architecture and analysis rules
tools/          corpus and source-specific audit utilities
```

Research is indexed by program in `research/README.md`; new investigations should extend an existing trunk before creating another peer-level file.

Start with:

- `AGENTS.md`
- `docs/holistic-musical-understanding.md`
- `docs/composer-level-understanding.md`
- `research/README.md`
- `research/music/composer-grammar-attribution.md`
- `docs/musical-execution-model.md`
- `docs/musical-inference-evidence.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/human-musical-discourse.md`
- `docs/source-native-enhanced-rendering.md`

## Testing

Core regressions are registered through CMake and can also be exercised with `tools/run_core_tests.py`.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Important mechanisms should also be challenged by real corpus controls, negative controls, independent implementations, matched-decoy tests, cross-soundtrack holdouts, and confound interventions.

## Working rules

1. Holistic musical understanding is the primary objective.
2. Blind composer attribution is a capstone stress test of understanding, not a shortcut objective.
3. Learn one work across all useful representations while preserving source-native semantics.
4. Learn one composer across independent works and different soundtracks whenever possible.
5. There is no privileged score/MIDI representation for composer identity.
6. All modalities may contribute, but every contribution carries role provenance.
7. Keep composition, arrangement/programming, driver/toolchain, patch/sample, and realization attribution distinct.
8. Recover symbolic note/sequence information whenever the source supports it.
9. Use same-composer cross-soundtrack and cross-platform controls to distinguish creator invariants from project artifacts.
10. Group related versions and derivative cues so the system cannot win by recognizing the work.
11. Actively intervene on timbre, patch/sample identity, platform, tempo, transposition, soundtrack-local, and arranger/programmer confounders.
12. Preserve encoded/source, authored, driver, device, sample, acoustic, perceptual, and listener-model distinctions where they exist.
13. Keep exact, derived, inferred, perceptual, and external claims distinct.
14. Do not call a physical channel a persistent musical part without evidence.
15. Do not jump from low-level pitch directly to harmony, creator grammar, or authorship.
16. A correct composer label without a traceable musical explanation is not sufficient evidence of understanding.
17. Composer evolution is expected; do not force all works into one static centroid.
18. Corrections outrank narrative coherence.
19. Accuracy/reference behavior remains available beneath every enhancement.
20. Traceability supports understanding but does not substitute for it.

> **Understand each musical work across its representations, then understand each composer across different soundtracks deeply enough that authorship can emerge as a consequence of the music rather than the production environment.**
