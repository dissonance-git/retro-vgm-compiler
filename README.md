# Game Music Interpreter

Executable understanding, musical analysis, source-native rendering, perceptual organization, and human-readable reasoning for digital game music.

## Objective

Game Music Interpreter follows the transformations that actually produce and are heard as a game soundtrack instead of flattening every source into MIDI, stems, final PCM, or a detached analytical summary.

**The primary objective is holistic musical understanding.** The project should ultimately understand and discuss a game soundtrack with the integrated musical awareness expected from a strong critic, musicologist, composer, arranger, producer, and audio engineer. Exact decoding, execution tracing, source isolation, synthesis reconstruction, and provenance are supporting machinery. They matter when they improve, constrain, explain, or validate the top-level understanding; they are not equal end goals merely because they are technically difficult.

The strongest composer-facing benchmark is deliberately difficult:

> If a game credits several composers but leaves some cue authorship unresolved, can the system understand the known works deeply enough to recover each composer's recurring musical grammar, then attribute a held-out cue for musical reasons and explain why?

That benchmark is downstream of understanding, not a replacement for it.

```text
physical encoding / exact bits and bytes
        ↓
format / memory / object semantics
        ↓
authored / preserved source
        ↓
program and driver execution
        ↓
device and synthesis state
        ↓
performed musical gestures
        ↓
persistent musical parts
        ↓
melody • bass • rhythm • harmony
        ↓
phrase • motif • cadence • counterpoint • form
        ↓
reusable compositional rules
        ↓
composer grammar
        ↓
blind composer-attribution stress tests
        ↓
holistic soundtrack understanding
        ↓
human musical explanation and listener response
```

The project is vertically end-to-end: the lowest useful fact may be a bit field, byte, address, command, sample word, or machine-state transition; the highest useful result may be an integrated account of what a piece or soundtrack is doing as music and why it belongs more naturally to one compositional grammar than another.

The listener musical model is distinct from listener response. `that is the returning melody`, `this bass part is answering it`, `the texture opens here`, or `this section delays the return` are claims about how the heard performance is organized as music. `surprising`, `familiar`, `moving`, `groovy`, or `I like it` are responses to that organization. Both must remain traceable to the evidence they actually use.

See `docs/holistic-musical-understanding.md` for the top-level evaluation target.

## Composer-level understanding and symbolic sequence evidence

The strongest evaluation target is not successful decoding, note extraction, MIDI export, or even a correct composer label. It is whether the system understands the composition deeply enough that attribution becomes a defensible consequence of the musical model.

`MIDI-like` information means symbolic note and sequence information broadly, not the MIDI file format alone. Useful composer-facing evidence may come from MIDI, MML, trackers, SMPS, GEMS, N-SPC, SSEQ, validated native driver bytecode, decoded sequence hex, score-like source, source code/data tables, or reconstructed note/performance events inferred upward from execution.

```text
symbolic note / sequence evidence
        ↕
driver execution
        ↕
chip / DSP / sample evidence
        ↕
rendered and heard organization
        ↓
melody • bass • rhythm • harmony
        ↓
phrase • motif • cadence • counterpoint • form
        ↓
composer-level musical model
```

Each representation is a different sensor. They should cross-check and teach one another when a real alignment exists. Explicit sequence tracks can teach the system what logical musical continuity looks like after hardware allocation. VGM/SPC execution can reveal synthesis, articulation, allocation, sample, and runtime details missing from score-like data. External MIDI or notation transcriptions can provide useful anchors while remaining explicitly external evidence.

But cross-format learning must not flatten the sources into pseudo-MIDI:

```text
MIDI track
!= MML voice
!= tracker channel
!= driver logical track
!= physical chip channel
!= physical voice episode
!= persistent musical part
```

A correspondence may be strong without becoming an equivalence. Source-native objects stay intact; common musical abstractions are added only where the evidence earns them.

> **Everything should help everything else understand the music, while each representation keeps what makes it uniquely informative.**

See `docs/composer-level-understanding.md`, `docs/music-representation-systems.md`, and `research/composer-grammar-attribution.md`.

## Composer grammar as a capstone benchmark

A composer grammar is not a flat fingerprint of easy-to-count traits. It is a model of recurring musical decisions and relationships across securely attributed independent works.

Useful coordinates include:

```text
MELODY
interval and contour behavior
cell repetition and mutation
sequence construction
phrase peaks and recoveries

BASS / HARMONY
bass motion
harmonic rhythm
voice-leading pressure
cadential motion
modal/chromatic behavior

RHYTHM
groove cells
syncopation
anticipation/delay
phrase-level density

PHRASE / FORM
phrase length and extension
transition logic
loop-boundary behavior
return versus development

MOTIF DEVELOPMENT
fragmentation
transposition
reharmonization
sequencing
truncation / extension

COUNTERPOINT
inner-voice behavior
imitation
contrary/parallel motion
common-tone and dissonance treatment
```

The strongest traits are relational. `uses syncopation` is weaker than `repeatedly delays the same melodic arrival and resolves the displacement at formal return points`.

For a multi-composer soundtrack, securely attributed tracks can build candidate grammars. Unknown tracks remain held out. The system then asks which grammar best explains the held-out piece and which musical dimensions support or contradict that answer.

A correct name without such an explanation is suspicious rather than impressive.

## Composition and realization are different attribution axes

Game music often distributes creative work across multiple people.

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= final rendering
```

A patch bank, FM modulation habit, PSG deployment pattern, BRR sample family, macro style, or channel-allocation habit may strongly identify an arranger/programmer while providing weak composer evidence.

The system should therefore preserve separate hypotheses such as:

```text
composition grammar → composer A
arrangement/programming grammar → programmer B
patch/sample vocabulary → shared team/library
```

These may legitimately disagree.

Cross-platform same-composer controls are especially valuable because composition-facing habits should survive more readily than low-level implementation details. A melodic, phrase, bass, harmonic, or motif-development pattern that follows the same composer from Genesis VGM to SNES SPC is much more interesting than a hardware-specific patch fingerprint.

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
!= frequency displacement
!= performed pitch
!= heard pitch
!= note spelling
```

```text
simultaneous pitches
!= chord spelling
!= harmonic function
!= key
!= progression
!= cadence
!= form
!= composer grammar
!= authorship
```

A VGM trace can be exact downstream execution evidence without exposing the tracker or sequence language that existed before it. An NSF can preserve executable music code without preserving source notation. A tracker note or effect can be exact source evidence while its realized performance still depends on prior state and tick-by-tick driver semantics.

> **Higher analysis may summarize lower evidence, but it may not erase or silently repair the uncertainty underneath it.**

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

PSF1, GSF, USF, 2SF, and NCSF share only an xSF envelope/dependency mechanism here. Their effective objects remain PS-X EXE memory, GBA uploads, Nintendo 64 ROM/save-state patches, Nintendo DS ROM/save maps, and selected SDAT structures respectively. A reconstructed effective object is not yet a running machine, understood driver/sequence, recovered voice/part structure, or validated playback path.

The common execution substrate lives in `model/`. Source-specific work lives under `components/` and retains its own timing, device, driver, and provenance semantics.

## Real corpus

The permanent corpus is a scientific control surface with immutable files, hashes, provenance, and manifest metadata.

It currently covers Genesis and SNES material plus controls for Yamaha OPN/OPM/OPL/OPLL families, AY-family PSG, SCC/K051649, HuC6280, NES APU and NSF, Game Boy audio, Namco WSG/C140/C352, SegaPCM, MultiPCM, RF5C164, OKIM6295, QSound, PlayStation PSF1, Game Boy Advance GSF, Nintendo 64 USF, and Nintendo DS 2SF/NCSF.

See `tests/CORPUS.md`, `tests/corpus/README.md`, and `tests/corpus/manifest.json`.

A device that breaks an assumption is as valuable as one that confirms it.

> **Shared abstractions should be discovered by agreement and disagreement.**

## Driver and tracker observatories

Source-available trackers, compilers, engines, and reconstructed drivers expose the semantic layer between symbolic music and hardware writes.

Current observatories include NES tracker/NSF engines, hUGEDriver, HuSIC/MML material, SCC sequence reconstruction, and reconstructed Namco driver semantics above C140/C352.

They help establish mechanisms such as:

```text
source note / instrument / effect
+ previous channel state
+ control flow
+ driver tick semantics
        ↓
performed device trajectory
```

They do not prove that an unrelated commercial soundtrack used the same toolchain. Historical linkage must be established independently.

See `research/game-music-driver-observatories.md`.

## Musical understanding

Higher musical claims are dependency-aware rather than vocabulary-first.

```text
performed pitches
↓
persistent parts
↓
melody / bass / accompaniment / inner voices
↓
harmonic segmentation
↓
chord tones vs figuration
↓
harmonic function and voice leading
↓
progression and harmonic rhythm
↓
cadence / phrase
↓
motivic and formal relations
↓
reusable compositional rules
↓
composer grammar
```

A listener-level song model may organize the same lower evidence into foreground and accompaniment, melody and bass roles, repeated sections, arrivals, departures, builds, returns, transitions, tension and release, or other musically meaningful relations.

The goal is not to concatenate separate analyses of harmony, rhythm, timbre, form, and space. The goal is to understand how those dimensions cooperate to make a passage, track, and soundtrack behave as a musical whole.

Alternative readings may coexist over unchanged lower evidence. Analytical systems such as Western functional harmony are used only where their assumptions fit.

## Attribution evaluation discipline

Composer attribution should be treated as an adversarial evaluation surface.

For a game with several plausible credited composers:

1. freeze the candidate set before looking at disputed cues;
2. keep disputed cues completely out of feature/model construction;
3. group prototypes, ports, reprises, arrangements and derivative cues by work family so related material cannot leak across train/test;
4. build composer grammars from independent securely attributed works;
5. use matched controls such as same driver/different composer and same composer/different platform;
6. rerun attribution with patch, timbre, platform, tempo, transposition, arranger/programmer, and related-work cues masked or normalized;
7. require supporting and contradicting musical explanations;
8. allow abstention when the recovered grammar does not converge.

The desired output is not simply:

```text
composer = A
```

It is closer to:

```text
composer A probable
  melodic grammar: strong support
  phrase/form grammar: strong support
  bass/harmony: medium support
  cadence behavior: counterevidence
  survives patch masking and platform holdout

arranger/programmer B probable
  FM/PSG realization grammar: strong support
```

See `research/composer-grammar-attribution.md` and `research/musicological-authorship-attribution.md`.

## Human musical discourse

Human-facing language is a projection over evidence, not another truth layer. The same musical object may be discussed naturally as a listener, critic, composer, theorist, producer, engineer, or forensic analyst.

The aspirational standard is composer-grade structural understanding without invented intent: the system should understand how musical choices support one another well enough to discuss why a passage works, how material develops, how an arrangement creates contrast, how a track participates in the identity of the larger soundtrack, and why a held-out cue resembles one composer's established grammar more than another's.

Descriptions such as `it opens up here`, `the bass starts pushing harder`, or `the phrase keeps delaying the return to tonic` are useful when they summarize integrated evidence rather than one-feature phrase rules.

See `docs/human-musical-discourse.md`, `docs/composer-level-understanding.md`, and `docs/holistic-musical-understanding.md`.

## Source-native enhanced rendering

The accurate/reference renderer is the scientific control, not the quality ceiling.

Enhanced rendering asks whether a specific implementation ceiling can be relaxed while preserving the same piece, parts, gestures, recognizable instruments, timing relationships, and arrangement.

```text
same musical object
        ↓
relax one unwanted implementation ceiling
        ↓
higher-quality realization
```

For FM this means preserving algorithm, operator relationships, envelopes, modulation, feedback, timing, articulation, and patch identity. For sample-based systems it means preserving source identity and intentional preparation while distinguishing technical degradation from changes the instrument adopted as part of itself.

See `docs/source-native-enhanced-rendering.md`.

## Evidence roles

Different sources answer different questions.

```text
format specification → file semantics
official chip/platform docs → documented hardware behavior
mature emulators/device cores → implementation and undocumented behavior
identified driver/source → software usage and authoring semantics
real corpus → observed preserved execution
literature → pressure-test general analytical claims
```

These sources cross-check one another rather than collapsing into one anonymous authority.

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
research/       bounded mechanism and evidence investigations
docs/           durable architecture and analysis rules
tools/          corpus and source-specific audit utilities
```

Start with:

- `AGENTS.md`
- `docs/holistic-musical-understanding.md`
- `docs/composer-level-understanding.md`
- `research/composer-grammar-attribution.md`
- `docs/musical-execution-model.md`
- `docs/musical-inference-evidence.md`
- `docs/musical-understanding-dependencies.md`
- `docs/music-representation-systems.md`
- `docs/persistent-musical-identity.md`
- `docs/human-musical-discourse.md`
- `docs/audio-programming-languages.md`
- `docs/source-native-enhanced-rendering.md`
- `docs/upstreams.md`
- `docs/vgm-frontier.md`

## Testing

Core regressions are registered through CMake and can also be exercised with `tools/run_core_tests.py`.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Important mechanisms should also be challenged by real corpus controls, negative controls, independent implementations, literature, matched-decoy tests, and confound interventions.

## Working rules

1. Holistic musical understanding is the primary objective; descend into lower layers when doing so materially improves that understanding.
2. Composer-level structural understanding is the strongest evaluation surface; decoding, reconstruction, transcription, export, and classification are means rather than endpoints.
3. Treat blind composer attribution among plausible credited composers as a capstone stress test of understanding, not as a shortcut objective.
4. Build composer hypotheses from composition-facing grammar such as melody, bass/harmony, rhythm, phrase, motif development, counterpoint, cadence, and form.
5. Keep composition attribution separate from arrangement/programming, driver/toolchain, and patch/sample attribution.
6. Recover symbolic note/sequence information whenever the source supports it, whether MIDI, MML, tracker data, native driver commands, decoded bytecode, or another representation.
7. Let source families cross-supervise one another through explicit evidence-bearing correspondences, but never collapse native semantics into a lowest-common-denominator pseudo-MIDI.
8. Use same-composer cross-platform controls and matched decoys whenever possible to distinguish compositional invariants from implementation fingerprints.
9. Group related versions and derivative cues during evaluation so the system cannot win by recognizing the work instead of the composer.
10. Actively intervene on confounders such as timbre, patch/sample identity, platform, tempo, transposition, and arranger/programmer features.
11. Source-domain first when a lower-level question is actually needed, beginning at exact encoded data when available.
12. Preserve encoded/source, authored, driver, device, sample, acoustic, perceptual, and listener-model clocks or alignments separately where the distinction exists.
13. Keep exact, derived, inferred, perceptual, listener-model, and external claims distinct.
14. Do not infer a commercial toolchain from output similarity alone.
15. Do not call a physical channel a persistent musical part without evidence.
16. Do not jump from nominal frequency directly to note spelling, harmony, style, composer grammar, or authorship.
17. A correct composer label without a musically traceable explanation is not sufficient evidence of understanding.
18. Corrections outrank narrative coherence.
19. Accuracy/reference behavior remains available beneath every enhancement.
20. Traceability is supporting infrastructure: preserve it where useful, but never confuse a perfect lower-level explanation with a complete understanding of the music.

> **Understand the soundtrack deeply enough that authorship can become a consequence of musical understanding, while preserving the distinct evidence that belongs to composition, realization, and source history.**
