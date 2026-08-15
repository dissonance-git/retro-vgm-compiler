# Game Music Interpreter

Executable understanding, musical analysis, source-native rendering, perceptual organization, and human-readable reasoning for digital game music.

## Objective

Game Music Interpreter follows the transformations that actually produce and are heard as a game soundtrack instead of flattening every source into MIDI, stems, final PCM, or a detached analytical summary.

**The primary objective is holistic musical understanding.** The project should ultimately understand and discuss a game soundtrack with the integrated musical awareness expected from a strong critic, musicologist, composer, arranger, producer, and audio engineer. Exact decoding, execution tracing, source isolation, synthesis reconstruction, and provenance are supporting machinery. They matter when they improve, constrain, explain, or validate the top-level understanding; they are not equal end goals merely because they are technically difficult.

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
acoustic realization
        ↓
auditory organization
        ↓
listener musical model
        ↓
parts • pitch • rhythm
        ↓
harmony • tonality • progression • voice leading
        ↓
cadence • phrase • motif • form
        ↓
style • work/version • attribution hypotheses
        ↓
holistic soundtrack understanding
        ↓
listener response and human musical discourse
```

The project is vertically end-to-end: the lowest useful fact may be a bit field, byte, address, command, sample word, or machine-state transition; the highest useful result may be an integrated account of what a piece or soundtrack is doing as music. Lower layers exist to serve that understanding. Descend as far as the musical question requires, but do not mistake exactness at a lower layer for completion of the task.

The listener musical model is distinct from listener response. `that is the returning melody`, `this bass part is answering it`, `the texture opens here`, or `this section delays the return` are claims about how the heard performance is organized as music. `surprising`, `familiar`, `moving`, `groovy`, or `I like it` are responses to that organization. Both must remain traceable to the evidence they actually use.

See `docs/holistic-musical-understanding.md` for the top-level evaluation target.

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
!= style
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
- MML and tracker source;
- ROM-derived samples, patches, sequences, and control data;
- rendered audio and documentary evidence.

These remain source-specific until independent systems force the same abstraction.

PSF1, GSF, USF, 2SF, and NCSF share only an xSF envelope/dependency mechanism here.
Their effective objects remain PS-X EXE memory, GBA uploads, Nintendo 64
ROM/save-state patches, Nintendo DS ROM/save maps, and selected SDAT structures
respectively. A reconstructed effective
object is not yet a running machine, understood driver/sequence, recovered
voice/part structure, or validated playback path.

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
harmonic segmentation
↓
chord tones vs figuration
↓
chord / root / inversion candidates
↓
local and global tonal center
↓
tonicization / modulation
↓
harmonic function
↓
progression and harmonic rhythm
↓
voice leading / counterpoint
↓
cadence / phrase
↓
motivic and formal relations
↓
listener model of the piece as a song
↓
holistic interpretation of the track and soundtrack
```

A listener-level song model may organize the same lower evidence into foreground and accompaniment, melody and bass roles, repeated sections, arrivals, departures, builds, returns, transitions, tension and release, or other musically meaningful relations. The holistic layer then integrates these with timbre, orchestration, production, spatial design, stylistic language, game function, cross-track relationships, and larger expressive trajectory.

The goal is not to concatenate separate analyses of harmony, rhythm, timbre, form, and space. The goal is to understand how those dimensions cooperate to make a passage, track, and soundtrack behave as a musical whole.

Alternative readings may coexist over unchanged lower evidence. Analytical systems such as Western functional harmony are used only where their assumptions fit.

## Human musical discourse

Human-facing language is a projection over evidence, not another truth layer. The same musical object may be discussed naturally as a listener, critic, composer, theorist, producer, engineer, or forensic analyst.

The aspirational standard is composer-grade structural understanding without invented intent: the system should understand how musical choices support one another well enough to discuss why a passage works, how material develops, how an arrangement creates contrast, and how a track participates in the identity of the larger soundtrack. Documentary claims about what a creator consciously intended remain separate from structural musical inference.

Descriptions such as `it opens up here`, `the bass starts pushing harder`, or `the phrase keeps delaying the return to tonic` are useful when they summarize integrated evidence rather than one-feature phrase rules.

See `docs/human-musical-discourse.md` and `docs/holistic-musical-understanding.md`.

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

Important mechanisms should also be challenged by real corpus controls, negative controls, independent implementations, or primary documentation.

## Working rules

1. Holistic musical understanding is the primary objective; descend into lower layers when doing so materially improves that understanding.
2. Source-domain first when a lower-level question is actually needed, beginning at exact encoded data when available.
3. Preserve encoded/source, authored, driver, device, sample, acoustic, perceptual, and listener-model clocks or alignments separately where the distinction exists.
4. Keep exact, derived, inferred, perceptual, listener-model, and external claims distinct.
5. Do not infer a commercial toolchain from output similarity alone.
6. Do not call a physical channel a persistent musical part without evidence.
7. Do not jump from nominal frequency directly to note spelling, harmony, style, authorship, or listener understanding.
8. Mature repositories and literature are observatories, not automatic dependencies.
9. Corrections outrank narrative coherence.
10. Accuracy/reference behavior remains available beneath every enhancement.
11. Traceability is supporting infrastructure: preserve it where useful, but never confuse a perfect lower-level explanation with a complete understanding of the music.

> **Understand the soundtrack as a musical whole; use every lower layer that helps you get there.**
