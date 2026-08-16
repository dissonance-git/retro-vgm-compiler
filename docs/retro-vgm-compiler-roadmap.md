# Retro VGM Compiler roadmap

## Purpose

Retro VGM Compiler is a provenance-aware compiler and musical reasoning system for video game music.

Its job is not merely to decode old formats, render sound chips, or export MIDI. It should recover enough of the musical object that many downstream tasks become different projections of the same internal understanding.

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

> **Understand the music deeply enough that conversion, reconstruction, attribution, porting, explanation, and re-realization become downstream compiler operations.**

This document is the durable roadmap. Future applications belong here until the semantic core is strong enough to justify building them.

---

## Compiler model

The compiler analogy is literal enough to guide the architecture.

```text
format parser / emulator / sequence decoder    → frontend
loops / calls / branches                       → control flow
patch / sample / controller state              → data flow
physical-channel reuse                         → identity / alias analysis
execution → persistent musical object          → semantic lifting
musical reasoning                              → analysis passes
MIDI / synth / tracker / target driver         → backend
A → IR → B → IR'                               → semantic round-trip test
```

Unlike a conventional compiler, source representations may expose different semantic altitudes. An SMPS source file can contain authored sequence decisions that a VGM register log only reveals indirectly. An SPC snapshot may preserve live driver memory and sample state without exposing a clean authored score. A MIDI file may expose explicit notes while losing the synthesis program that gave them their original meaning.

The common representation therefore cannot be a lowest-common-denominator note list. Shared semantics are earned only when different sources support the same musical claim.

---

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

Examples:

```text
pitch_quantized_to_12tet
timing_quantized
meter_imposed_for_export
timbre_collapsed_to_program
fm_operator_structure_lost
sample_identity_lost
physical_channel_identity_lost
persistent_part_unknown
percussion_event_uncertain
software_modulation_simplified
loop_or_branch_flattened
```

The governing interoperability rule is:

```text
IMPORT KNOWLEDGE, NOT ASSUMPTIONS.
EXPORT INTERPRETATION, WITH LOSSES DECLARED.
```

---

## Current semantic ladder

The active implementation is building upward through these layers:

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
relative and absolute pitch evidence
        ↓
motifs and motif transformations
        ↓
part-level phrase boundaries
        ↓
cross-part phrase consensus
        ↓
phrase regions and phrase relationships
        ↓
harmonic verticalities and bounded chord candidates
        ↓
harmonic transitions and harmonic rhythm
        ↓
voice leading / bass-harmony interaction / counterpoint / imitation
        ↓
cadential-arrival evidence
        ↓
sections and section relationships
        ↓
arrangement / orchestration / texture / tension-release
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

---

## What is already structurally implemented

The project already contains meaningful machinery for:

- exact VGM/VGZ command and Genesis device-state reconstruction;
- SPC snapshot/runtime/sample/voice evidence;
- PSF-family effective-object reconstruction and platform-specific probes;
- persistent-part hypotheses that do not equate physical channels with musical identity;
- transposition- and tempo-scale-tolerant motif profiles;
- motif recurrence and transformation classification;
- phrase-boundary evidence and cross-part phrase consensus;
- phrase regions and phrase-to-phrase relationships;
- absolute-pitch admission gates;
- harmonic verticalities and explicitly tuned triad hypotheses;
- harmonic transitions and tempo-scale-invariant harmonic rhythm;
- persistent-part-aware voice leading;
- bass/harmony interaction;
- evidence-first cadential arrivals without premature Roman-numeral/function claims;
- multi-phrase section hypotheses and section recurrence/variation;
- counterpoint motion and imitation/call-response;
- creator-blind structural grammar observations;
- cross-work/cross-soundtrack composer-grammar confidence rules;
- role-aware composer/arranger/programmer/toolchain attribution boundaries;
- loss-declared symbolic projection rather than MIDI-as-truth;
- Sonic 3 as the primary adversarial integration testbed.

The Genesis path additionally distinguishes channel FNUM from operator-network periodicity and performed-pitch hypotheses. FM algorithm topology, operator multipliers, detune, phase modulation, partial key state, CH3 special mode, and DAC mode can strengthen, weaken, or block pitch claims instead of being flattened into nearest MIDI notes.

---

## Near-term build frontier

The current priority is to make the semantic IR more musically complete before building product backends.

### 1. Instrumentation role and orchestration grammar

Infer relationships such as:

- foreground melody versus accompaniment;
- bass, inner voice, counterline, pad, ostinato, percussion and doubling roles;
- role transfer between persistent parts or timbres;
- registral support and spacing;
- density and textural changes;
- timbral contrast that marks phrase, section, return, climax or release;
- recurring orchestration strategies across works.

A patch/sample identity is evidence about realization. It is not itself a musical role.

### 2. Time-varying performed pitch and articulation

Move beyond onset-only pitch where source evidence permits. Preserve glides, vibrato, ornaments, bends, modulation and sustained pitch movement as trajectories within one episode unless rearticulation evidence exists.

### 3. Stronger harmony and tonality

Extend from safe verticalities/triads toward:

- non-tertian and incomplete sonorities;
- tonal-center and key hypotheses;
- non-chord tones;
- harmonic function only when context earns it;
- cadence classification after function is grounded;
- longer-range harmonic planning.

### 4. Texture, density and tension/release

Model how register, activity, dissonance, harmonic rhythm, timbre, dynamics, articulation, silence and sectional expectations cooperate over time.

### 5. Percussion-event reconstruction

Recover musically meaningful percussion events from PCM/DAC/sample execution rather than treating individual sample writes as notes.

### 6. Whole-work and soundtrack integration

Make local analysis cooperate at the cue and soundtrack level: thematic families, returns, contrasts, transformed reprises, cue function, pacing, exceptions and soundtrack-scale grammar.

### 7. Real-corpus execution

Move the strongest shared C++ relations through the permanent corpus and Sonic 3 controls. Freeze creator-blind structural observations before attribution labels are admitted.

---

## Sonic 3 as integration laboratory

Sonic 3 / Sonic & Knuckles remains the primary adversarial testbed because it forces many layers to cooperate:

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

The attribution labels are evaluation hypotheses, not feature inputs. ROM forensics is quarantined from musical-blind extraction. Driver/toolchain similarity must not become composer evidence automatically.

The strongest test is not whether the system guesses a name. It is whether the same musical explanation survives representation changes, soundtrack changes, platform changes and confound interventions.

---

## Porting as a test of understanding

Cross-system porting is a future backend, but it is also a powerful validation principle.

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
compare musical meaning
```

A successful port need not preserve bytes or timbre. It should preserve the properties declared important for that target, for example:

```text
persistent parts         preserved
motif structure          preserved
phrase structure         preserved
harmonic movement        preserved
articulation             preserved where target allows
formal relationships     preserved
timbre                    intentionally transformed
hardware-specific state  intentionally lost
```

Different backends can declare different permissible losses. A General MIDI export may lose FM topology. A compatible FM synth backend may preserve much more. A target game driver may preserve sequence/control structure but require voice-allocation changes.

Porting should therefore become compiler verification, not merely novelty conversion.

---

## Future backends and applications

These are downstream goals, not current implementation branches.

### High-quality symbolic transcription

Project to MIDI, MusicXML-like notation, piano roll, tracker rows or DAW tracks with part identity, articulation, timing, controllers, uncertainty and explicit losses.

### Real-synth realization

Play recovered musical parts on hardware or software synthesizers while preserving articulation, modulation, phrase relationships and orchestration function rather than merely assigning General MIDI programs.

### Cross-format / cross-platform porting

Examples include Genesis → SNES, SNES → Genesis, PS1 sequence → modern synth, tracker → SMPS, or one driver family → another. Target constraints should be explicit compiler constraints.

### Semantic remastering

Relax an implementation ceiling while preserving the musical object. Distinguish authored musical decisions from technical limitations before changing either.

### Musical decompilation and source reconstruction

Recover increasingly high-level sequence/source semantics from execution-only evidence and validate against hidden authored/source representations where available.

### Representation-independent musical search

Search for motifs, bass/harmony relations, phrase strategies, harmonic rhythms, counterpoint patterns or formal transformations across heterogeneous formats.

### Composer, arranger and programmer grammar

Learn recurring musical and realization decisions across independent works, with role provenance and confound controls. Attribution remains a consequence of the grammar, not an opaque classifier.

### Musical genealogy and influence analysis

Identify which structural ideas migrate between works: melodic cells, bass strategies, rhythmic schemas, orchestration behavior, phrase architecture or transformation rules, while separating generic style vocabulary from stronger lineage evidence.

### Version archaeology and lost-version reconstruction

Compare prototype/final, arcade/console, regional versions, ports, remixes and surviving related representations. Recover what changed musically versus technically and infer missing arrangement/source information only where evidence supports it.

### Adaptive-music reconstruction

Preserve branches, variables, layer activation and conditional sequence paths as a musical state graph rather than flattening an adaptive score into one playback.

### Music debugging and linting

Detect malformed loops, stuck envelopes, impossible pitch states, suspicious clocks, broken samples, accidental SFX, conversion artifacts, likely corrupted rips and semantic inconsistencies.

### Semantic deduplication

Distinguish byte duplicate, same execution, same arrangement, same composition, transformed version, work-family relative and merely similar material.

### Arrangement reduction and expansion

Produce melody/bass reductions, lead sheets or piano reductions, and eventually realize reduced material under target orchestration/hardware constraints.

### Hardware counterfactuals

Ask how a musical object could be realized under another platform's voice/sample/synthesis constraints without claiming the counterfactual was historically intended.

### External-tool feedback

Existing converters, trackers, emulators, disassemblies and setmaking utilities are research observatories first. Later, Retro VGM Compiler should feed improved semantics back into those ecosystems through libraries, annotations, converters or patches.

### All-in-one VGM setmaking environment

A future application can replace fragmented command-line workflows with one coherent setmaking tool backed by the semantic engine. It can eventually use compiler knowledge for rip validation, semantic deduplication, loop verification, metadata/provenance checks, version comparison and conversion. **Do not build this application until the semantic core is sufficiently mature.**

---

## Round-trip and differential validation

A compiler this interpretive needs adversarial tests, not demonstrations alone.

Useful patterns include:

```text
native source → execution → hide native source → recover upward → compare
```

```text
representation A → IR
representation B → IR
compare the same work without assuming equivalence
```

```text
IR → backend B → re-analyze B → IR'
measure semantic preservation
```

```text
our projection vs vgm2mid / smps2mid / tracker converter
→ inspect disagreements
→ use source/driver truth to decide which interpretation is better
```

```text
same creator / different soundtrack
same soundtrack / different creator
same driver / different creator
same creator / different platform
```

A disagreement is an experiment. Correction outranks a convenient narrative.

---

## Roadmap priority

Unless a discriminating test requires otherwise, build in this order:

1. deepen persistent musical identity and time-varying performance evidence;
2. finish instrumentation-role, orchestration, texture and dynamics models;
3. strengthen harmony, tonality, cadence and long-range form;
4. integrate cue-level analysis into whole-work and soundtrack models;
5. execute those relations over heterogeneous real corpora;
6. pressure-test representation invariance and creator invariance;
7. improve human musical explanation and attribution from the same evidence;
8. only then promote mature projections into reusable backends;
9. use synth realization, porting and semantic round trips as validation surfaces;
10. build integrated end-user tooling after the compiler core is trustworthy.

The default question remains:

> **What prevents Retro VGM Compiler from understanding this music more completely, and what experiment would remove that uncertainty?**

---

## Long-term shape

Retro VGM Compiler should eventually function less like a collection of format converters and more like compiler infrastructure for game music itself:

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
