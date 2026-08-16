# Composer-level understanding

## North star

Game Music Interpreter is ultimately trying to understand a game soundtrack the way a strong composer can understand another composer's music.

That means recovering more than playable audio, chip state, note names, MIDI, chords, or isolated features. The system should build an integrated model of how a piece is constructed, how its parts cooperate, how musical ideas transform across time, why important moments work, and how each cue participates in the grammar of the larger score.

The target is structural understanding without invented creator intent.

```text
exact source / execution evidence
        +
symbolic note and sequence evidence
        +
heard musical organization
        +
cross-track and documentary context
        ↓
composer-level model of the music
```

A successful answer should increasingly be able to say not merely `these notes occur`, but things such as:

- this is the returning melodic idea, transformed rather than simply repeated;
- the bass is changing the harmonic meaning of otherwise similar upper material;
- this inner line is functioning as counterpoint rather than filler;
- the arrangement creates a structural arrival by widening register, changing density, and reassigning timbral roles;
- the phrase delays an expected resolution and uses that delay to connect two sections;
- this cue inherits a soundtrack-wide harmonic, rhythmic, thematic, or orchestration habit while deliberately breaking another one.

## Symbolic music is broader than MIDI

In this project, symbolic note/sequence evidence means any representation that exposes authored or executable musical events above raw device state. MIDI is one member of that family, not its definition and not its canonical form.

Potential symbolic sources include:

- MIDI notes, tracks, programs, controllers, bends, tempo and meter;
- MML and other text music languages;
- tracker patterns, rows, effects and instrument commands;
- SMPS, GEMS, N-SPC and other native driver sequence languages;
- Nintendo DS SSEQ and related sequence structures;
- decoded sequence bytecode or hex whose command semantics are validated;
- score-like source or notation;
- source code or data tables that prove note, duration, instrument, loop or logical-track structure;
- reconstructed note/performance events inferred from execution when no authored sequence survives;
- external transcriptions used as explicitly external evidence.

The phrase `recover the MIDI` is therefore too narrow for the project goal. The real task is:

> recover as much of the latent musical program and score-like structure as the evidence supports, without pretending that every source originally contained MIDI or any other universal notation.

## Cooperative representations law

Every representation is a sensor looking at a different aspect of the same musical object.

```text
MIDI / notation / MML / tracker / native sequence
        ↕
logical tracks • notes • rests • durations • control flow
        ↕
driver execution
        ↕
patches • samples • synthesis state • physical voice episodes
        ↕
VGM / SPC / executable-rip evidence
        ↕
rendered audio / auditory organization
        ↓
shared musical interpretation
```

Information should flow in both directions where evidence permits.

A symbolic sequence can teach the system what known logical parts, notes, articulations and transformations look like after driver allocation and chip execution. Low-level execution can teach the system where a symbolic representation is incomplete, quantized, mistranscribed, or blind to synthesis and allocation behavior. Audio and perceptual evidence can pressure-test whether an exact implementation distinction is actually heard as a musical distinction.

The system should exploit these correspondences for cross-supervision, calibration, testing and inference.

## Preserve uniqueness while sharing knowledge

Cross-representation learning must not flatten source families into a lowest-common-denominator pseudo-MIDI.

These are not interchangeable:

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
MIDI program
!= FM patch
!= BRR sample
!= tracker instrument
!= driver instrument object
```

and:

```text
MIDI note
!= sequence note command
!= device pitch state
!= sample playback rate
!= performed pitch
!= heard pitch
!= notation spelling
```

A correspondence can be strong without becoming an equivalence.

The common model should therefore express relations such as `realizes`, `derived_from`, `corresponds_to`, `groups_into`, `same_identity_as`, transformation hypotheses, time mappings and confidence-bearing alignments while leaving native objects intact.

> Everything may teach everything else, but nothing is allowed to erase what makes a representation uniquely informative.

## Forward and inverse understanding should meet

The project has two complementary directions.

### Forward

When authored or sequence semantics are available:

```text
note / instrument / effect / logical track
+ prior state
+ control flow
+ driver timing
        ↓
physical execution
        ↓
heard result
```

This teaches exact causal relationships between compositional instructions and realization.

### Inverse

When only execution or audio survives:

```text
register / DSP / sample / voice behavior
        ↓
performance-event hypotheses
        ↓
persistent musical parts
        ↓
pitch / rhythm / articulation / instrumentation
        ↓
phrase / harmony / motif / form
        ↓
composer-level interpretation
```

The inverse model should use lessons learned from forward-observable formats without asserting that the hidden historical source was the same format or toolchain.

For example, a known SMPS or SSEQ track can teach what stable logical-part identity looks like under allocation changes. That lesson may improve VGM or SPC part recovery. It does not prove that a VGM or SPC came from SMPS or SSEQ.

## Cross-format teaching examples

### MIDI and native sequence data

MIDI or validated sequence data can provide relatively explicit note boundaries, duration, part order, tempo, controller state, instrument changes and phrase-scale organization. These are useful supervisory signals for learning how musical structure appears downstream.

### VGM

VGM can provide exact or near-exact device-command timing, register trajectories, patch state, DAC behavior and physical voice activity. It can reveal performance details and synthesis decisions that a score-like transcription may flatten.

A Genesis VGM may support continuity through a stable YM2612 patch fingerprint, timing, relative pitch motion and articulation even when physical-channel identity changes.

### SPC

SPC runtime analysis can expose S-DSP voice lifecycle, source indices, event-time BRR sample versions, pitch rates, envelopes and RAM generations. These can support persistent-part and articulation inference even when no original sequence language is known.

When the same proven sample continues across voice reassignment, compatible timing and relative pitch can support the hypothesis that a musical part persisted despite hardware migration.

### Driver and tracker sources

SMPS, GEMS, N-SPC, trackers, MML and similar sources can expose logical tracks, loops, macros, effects, allocation rules and explicit authored structure. They are especially valuable bridges between compositional decisions and hardware execution.

## The latent musical model

The shared musical model should accumulate only abstractions that genuinely survive comparison across source families.

Useful targets include:

```text
musical events
    pitch relations • onset • duration • articulation • dynamics

persistent parts
    melody • bass • countermelody • accompaniment • percussion • texture

phrase structure
    cells • motifs • repetition • variation • call/response • cadence

harmonic organization
    simultaneity • voice leading • harmonic rhythm • tonal/modal function

arrangement
    register • voicing • density • orchestration • doubling • role migration

form
    section • transition • buildup • return • interruption • release

soundtrack relationships
    thematic transformation • cue families • shared grammar • exceptions

ludic function
    state signaling • traversal/place identity • tension management
    continuity • narrative framing • player-action coupling
```

These are not simply fields to fill. They are hypotheses and structures whose support may come from several representations at once.

## Composer questions are the evaluation surface

The project should increasingly be tested with questions a composer would ask of another piece:

- What is the governing musical idea here?
- Which material is structural and which is figuration?
- What does the bass do to the harmony and phrase direction?
- Which voices are independent, doubled, decorative, or contrapuntal?
- Where does tension come from, and what actually releases it?
- How is repetition made non-redundant?
- What changes when a section returns?
- Which transformations preserve identity and which create a new idea?
- What is the orchestration doing beyond assigning instruments to notes?
- How does register participate in form?
- Which rhythmic cells organize the groove?
- How do articulation and synthesis change the perceived role of identical pitch material?
- Which habits recur across the soundtrack strongly enough to constitute an internal compositional grammar?
- Which cue deliberately violates that grammar, and what does the contrast accomplish?
- Given the established grammar, which continuations or variations are plausible and which would feel foreign?

A system that cannot answer these questions has not reached the goal merely because it can decode the source perfectly.

## Counterfactual understanding

Composer-level understanding should support bounded counterfactual reasoning.

If the system claims to understand why a passage works, it should be able to predict some consequences of changing it:

```text
change bass motion
→ harmonic function / voice-leading pressure may change

remove countermelody
→ texture and phrase dialogue may collapse

preserve notes but change register/orchestration
→ formal weight or foreground hierarchy may change

change articulation while keeping pitch
→ groove, emphasis or perceived role may change

replace a transformed motif with a literal repeat
→ developmental relationship may weaken
```

These are analysis hypotheses, not licenses to invent historical intent. Their value is that they test whether the model has learned relationships among musical decisions rather than memorized labels.

## Implementation implications

1. Preserve source-native objects and semantics before normalization.
2. Recover symbolic note/sequence information whenever available, regardless of whether it is MIDI, MML, tracker data, native bytecode or another form.
3. Treat reconstructed score-like data as an evidence-bearing projection, not canonical truth.
4. Align representations explicitly in time rather than forcing them onto one clock.
5. Allow one musical part to migrate across physical resources.
6. Allow one source object to realize several audible gestures and several source objects to cooperate in one musical role.
7. Use stronger authored/driver identity to supervise weaker execution-only heuristics when the relationship is independently established.
8. Use execution evidence to detect where symbolic simplifications lose meaningful behavior.
9. Learn cross-format regularities only after preserving source-specific exceptions and counterexamples.
10. Evaluate progress by improvement in musical interpretation, not by parser count or decoded-state volume.

## Evidence discipline

Deep interpretation must remain traceable.

Keep separate:

```text
OBSERVED / EXACT
source bytes, commands, runtime state, documented sequence semantics

DERIVED
validated transformations of exact evidence

INFERRED
parts, note identities, harmony, phrase, motif, role, form, style

EXTERNAL
transcriptions, interviews, scores, documentation, scholarship
```

A MIDI transcription can be extraordinarily useful while still being external evidence. A driver track can be exact without proving how a listener groups it. A VGM register trace can be exact without revealing the original authored note spelling.

Confidence should rise when independent representations converge and fall when they disagree.

Disagreement is not noise to be normalized away. It is often the most informative result.

## Completion criterion

The project is not finished with a source family when it can merely parse or replay it.

For a mature source family, the desired path is:

```text
native truth
→ sequence / performance semantics where recoverable
→ persistent musical identity
→ pitch / rhythm / articulation / instrumentation
→ composition and arrangement
→ phrase / harmony / motif / form
→ soundtrack relationships and game function
→ natural composer-level explanation
```

Different formats will expose different rungs directly. The common system should use those differences as leverage.

> **The destination is not MIDI. The destination is understanding the composition. Symbolic sequence data is one of the strongest bridges available, and every source family should help the others cross it without losing its own identity.**
