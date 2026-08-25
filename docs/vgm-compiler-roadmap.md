# VGM Compiler roadmap

## Purpose

VGM Compiler is a provenance-aware compiler and musical reasoning system for video game music.

Its job is not merely to decode old formats, render sound chips, or export MIDI. It should recover enough of the musical object that conversion, reconstruction, attribution, porting, explanation, re-realization, and search become different projections of the same internal understanding.

```text
SOURCE REPRESENTATIONS
VGM / VGZ • SPC • PSF-family • USF • 2SF • GSF • NSF • KSS • HES
SMPS • GEMS • SSEQ • MML • MIDI • trackers • source/disassembly • audio
                         ↓
               source-specific frontends
                         ↓
            exact execution / native semantics
                         ↓
                 musical semantic IR
                         ↓
parts • gestures • articulation • motifs • phrases • harmony • form
arrangement • orchestration • counterpoint • soundtrack relationships
                         ↓
             reasoning / verification passes
                         ↓
                    backends
MIDI • notation • synths • trackers • driver ports • analysis • annotations
```

The difficult problem is the middle, not the exporter.

> **Understand the music deeply enough that downstream transformations become compiler operations over an evidence-carrying musical model.**

## Architectural law

The compiler must never collapse source-native truth into a convenient output representation.

```text
physical channel != bounded voice episode != persistent musical part
register frequency != nominal frequency != performed pitch != heard pitch
FM patch != instrument name
sample identity != musical role
pitch movement != note rearticulation
simultaneous pitches != chord spelling != harmonic function
MIDI export != canonical score
```

A backend may deliberately lose information, but the loss must be visible.

```text
IMPORT KNOWLEDGE, NOT ASSUMPTIONS.
EXPORT INTERPRETATION, WITH LOSSES DECLARED.
```

## Current semantic ladder

The active implementation is already well above raw decoding:

```text
encoded/native source truth
        ↓
format / memory / sequence semantics
        ↓
driver and program execution
        ↓
device / synthesis / sample state
        ↓
physical voice episodes and control trajectories
        ↓
persistent musical parts
        ↓
performed pitch / articulation / role evidence
        ↓
motifs and motif transformations
        ↓
part-level phrase boundaries
        ↓
cross-part phrase consensus
        ↓
phrase regions and phrase relationships
        ↓
tonal-center / key-class / chord-degree hypotheses
        ↓
harmonic verticalities / transitions / harmonic rhythm
        ↓
voice leading / bass-harmony interaction / counterpoint
        ↓
cadential-arrival evidence
        ↓
cadence morphology + independent formal-closure evidence
        ↓
phrase syntax / longer-range harmonic-formal planning
        ↓
sections / arrangement / orchestration / texture
        ↓
whole-work and soundtrack models
        ↓
creator-blind recurring structural grammar
        ↓
cross-soundtrack creator grammar
        ↓
role-aware attribution and explanation
```

Every arrow is an inference boundary. Failure to establish one layer must remain visible rather than being patched over with a convenient guess.

## Structurally implemented now

The repository contains working machinery for:

- exact VGM/VGZ command and Genesis device-state reconstruction;
- SPC snapshot/runtime/sample/voice evidence;
- PSF-family effective-object reconstruction and platform-specific probes;
- source/driver/toolchain provenance and dialect/revision boundaries;
- persistent-part hypotheses that do not equate physical channels with musical identity;
- performed pitch motion, articulation, motif shape, and source-backed role evidence;
- motif recurrence and transformation classification;
- phrase-boundary evidence, cross-part phrase consensus, and phrase regions;
- tonal-center, key-class, pitch-class-collection, and diatonic chord-degree hypotheses;
- harmonic verticalities, transitions, harmonic rhythm, voice leading, and bass/harmony interaction;
- cadential arrivals with provenance-bound degree evidence;
- Ionian authentic, PAC/IAC, half, and leading-tone resolution morphology candidates;
- independent non-cadence-derived formal-closure evidence;
- morphology/formal-closure integration;
- V→VI continuation and deferred-authentic-resolution candidates without automatically naming deceptive cadence;
- section, counterpoint, imitation, orchestration, and creator-grammar models;
- source-native enhanced rendering and Omniphony handoff contracts;
- semantic projection/round-trip experiments and a broad real corpus.

## Active frontier: deceptive close vs deferred resolution

The most immediate unresolved bridge is now precise.

A raw Ionian V→VI progression is insufficient to name a deceptive cadence.

The code already models this path:

```text
V → VI
+ independently grounded cross-part continuation through VI
+ later integrated V → I authentic closure
→ deferred authentic resolution candidate
```

The next bridge is the complementary case:

```text
V → VI
+ independently grounded phrase completion at VI
+ no circular cadence-derived closure witness
→ deceptive cadence candidate
```

The evidence firewall matters. A cadence detector cannot supply the phrase-completion evidence that is then used to prove its own cadence label.

The immediate implementation sequence is:

1. add a bounded deceptive-cadence candidate that combines diatonic 5→6 morphology with independent phrase-completion evidence at the VI arrival;
2. preserve `cadence_class_established = false` until stronger style/formal evidence earns a final class;
3. keep continuation/deferred-resolution evidence distinct from terminal closure evidence;
4. register the entire recent cadence regression family in CMake so the normal suite actually executes it;
5. move next into phrase-role and style/grouping evidence that can separate close, continuation, return, and deferred resolution across longer contexts.

## Near-term priorities after cadence closure

### 1. Phrase syntax and longer-range harmony

Use grounded local events to model phrase roles, returns, continuation, prolongation, deferred resolution, and longer-range harmonic planning. Do not turn Roman numerals or chord sequences into a lookup table for form.

### 2. Instrumentation role and orchestration grammar

Strengthen foreground/accompaniment, bass, inner voice, counterline, pad, ostinato, percussion, doubling, role transfer, register, density, texture, and timbral-form relations.

### 3. Time-varying performance and dynamics

Continue moving beyond onset-only descriptions. Preserve glides, vibrato, bends, ornaments, modulation, dynamics, articulation, and sustained movement as trajectories where the source supports them.

### 4. Whole-work and soundtrack integration

Make local analysis cooperate at cue and soundtrack scale: thematic families, returns, contrasts, transformed reprises, cue function, pacing, exceptions, and soundtrack-scale grammar.

### 5. Real-corpus execution

Move the strongest shared relations through heterogeneous permanent corpus controls. Freeze creator-blind structural observations before attribution labels are admitted.

## Sonic 3 as integration laboratory

Sonic 3 / Sonic & Knuckles remains a strong adversarial testbed because it forces many layers to cooperate:

```text
SMPS source / disassembly
        ↕
prototype and final versions
        ↕
YM2612 / PSG / DAC execution
        ↕
VGM captures
        ↕
persistent parts / motifs / phrases / harmony / form
        ↕
cross-soundtrack creator controls
        ↕
composer / arranger / programmer attribution
        ↕
ROM and historical provenance as independent forensic evidence
```

Attribution labels are evaluation hypotheses, not feature inputs. ROM forensics is quarantined from musical-blind extraction. Driver/toolchain similarity must not become composer evidence automatically.

## Porting as a test of understanding

Cross-system porting is a future backend and a verification surface.

```text
source representation A
        ↓
semantic IR
        ↓
target realization B
        ↓
re-analyze B
        ↓
semantic IR'
        ↓
compare declared musical obligations
```

A successful port need not preserve bytes or timbre. It should preserve whichever musical properties the target contract declares important, while exposing intentional losses.

## Future applications

Downstream goals include:

- high-quality symbolic transcription;
- real-synth realization;
- cross-format and cross-platform porting;
- semantic remastering;
- musical decompilation and source reconstruction;
- representation-independent musical search;
- composer/arranger/programmer grammar;
- musical genealogy and influence analysis;
- version archaeology and lost-version reconstruction;
- adaptive-music reconstruction;
- music debugging and linting;
- semantic deduplication;
- arrangement reduction and expansion;
- hardware counterfactuals;
- an integrated VGM setmaking environment once the semantic core is mature enough.

## Validation pattern

A compiler this interpretive needs adversarial tests, not demonstrations alone.

```text
native source → execution → hide native source → recover upward → compare
```

```text
representation A → IR
representation B → IR
compare without assuming equivalence
```

```text
IR → backend B → re-analyze B → IR'
measure semantic preservation
```

```text
same creator / different soundtrack
same soundtrack / different creator
same driver / different creator
same creator / different platform
```

A disagreement is an experiment. Correction outranks a convenient narrative.

## Priority order

Unless a discriminating test requires otherwise:

1. close the cadence/phrase-syntax gap without circular evidence;
2. strengthen phrase role, orchestration, texture, dynamics, and longer-range harmony;
3. integrate cue-level analysis into whole-work and soundtrack models;
4. execute those relations over heterogeneous real corpora;
5. pressure-test representation invariance and creator invariance;
6. improve human musical explanation and attribution from the same evidence;
7. promote mature projections into reusable backends;
8. use synth realization, porting, and semantic round trips as validation surfaces;
9. build integrated end-user tooling after the compiler core is trustworthy.

The default question remains:

> **What prevents VGM Compiler from understanding this music more completely, and what experiment would remove that uncertainty?**

## Long-term shape

VGM Compiler should function less like a collection of format converters and more like compiler infrastructure for game music itself:

```text
many native musical languages
            ↓
     provenance-preserving IR
            ↓
 understanding / verification passes
            ↓
      many useful realizations
```

The output is not the project. **The recovered musical meaning is the project.**
