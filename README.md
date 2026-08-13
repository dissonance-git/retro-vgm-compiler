# VGM Tooling

Executable understanding, analysis, source-native rendering, and human-readable reasoning for digital game music.

`VGM` in the project name is historical shorthand for video game music tooling. The project is not limited to the `.vgm` format, and the foobar2000 components are important realtime frontends rather than the definition of the project.

## Objective

VGM Tooling should understand a supported game-music object deeply enough to do three things at once:

1. **explain and render the machinery that makes the music happen**, from authored/programmed source through driver execution, synthesis, routing, effects, and final audio;
2. **reason about the song as music**, including performed pitch/rhythm, persistent parts, auditory organization, harmony, key/tonal center, chord function, progression, harmonic rhythm, voice leading, cadence, phrase, motif, form, texture, timbre, repetition, and larger musical behavior;
3. **discuss that music naturally**, using the kind of language appropriate to a listener, reviewer/critic, composer/musician, musicologist/theorist, producer, mixing/mastering engineer, or forensic/technical analyst without losing the evidence underneath the wording.

The higher goals do not replace the lower ones. Every musical, perceptual, historical, or natural-language claim should remain descendable into the strongest evidence underneath it.

```text
VGM / SPC / MML / driver sequence / tracker / MIDI / executable rip
        ↓
source-specific parsing / execution
        ↓
exact program, driver, device and synthesis state
        ↓
realized performance events and control trajectories
        ↓
persistent musical identities when supported
        ↓
routing / effects / acoustic realization
        ↓
auditory organization / heard objects
        ↓
pitch • rhythm • part relations
        ↓
harmony • tonality • progression • voice leading
        ↓
cadence • phrase • motif • formal hierarchy
        ↓
style • work/version • attribution hypotheses
        ↓
        THE SONG AS UNDERSTOOD
        coherent, time-aligned, queryable
        ↓
human discourse projection
listener • critic • composer • musicologist • producer • engineer • forensic
        ↕
every material claim can descend back
into the strongest available evidence
```

This is an orientation, not a mandatory serial pipeline. Authored score, validated driver state, VGM/SPC execution, rendered audio, libaural observations, archival sources, or external annotations may expose different layers directly. The real structure is a dependency graph: an inferred higher claim must retain the evidence, assumptions, competing interpretations, and uncertainty that allowed it to exist.

A supported object should therefore not collapse immediately into MIDI, stems, or stereo PCM. Those are useful projections. The internal representation must retain the route among source truth, execution truth, performed musical behavior, acoustic realization, auditory interpretation, musicological analysis, and human explanation.

The long-term target is not merely to answer:

> What register changed at this time?

It is also to answer naturally:

> What happens here musically?

and then, if asked:

> What key or harmony is active, how does the progression function, why does the phrase feel different, and what exact programmed or acoustic behavior creates that effect?

## One phenomenon, many observatories

VGM Tooling deliberately studies mature systems that expose different strata of digital music:

- VGM/VGZ and register-log tooling;
- the VGM specification as a format-level authority;
- official chip, console, and development documentation where preserved;
- SPC, NSF/NSFe, HES, KSS and PSF-family executable/snapshot formats;
- native drivers such as SMPS, GEMS, N-SPC, MDX and PMD;
- MML dialects and compilers;
- MIDI and hardware/module synthesis;
- trackers and module formats;
- MAME, Hoot, Game Music Emu, Modizer and other broad execution systems;
- VGMTrans and VGM/SPC-to-MIDI recovery tools;
- DAWs and session/routing systems;
- music21, Partitura, MEI, Humdrum and related symbolic systems;
- OpenMusic and other computer-assisted composition systems;
- SuperCollider, Csound, ChucK, Faust, Cmajor, Max/MSP/Pure Data and related audio languages;
- MIR, score/audio alignment, source-separation, timbre, expressive-performance, music-cognition and ludomusicology literature;
- harmonic-analysis, tonal-hierarchy, voice-leading, cadence, form, computational musicology, and authorship-attribution research;
- practitioner interviews and surviving composer/programmer source code;
- natural-language music-caption corpora, music criticism, studio/production discourse, and research on musical metaphor.

These are **research observatories**, not an architecture to copy wholesale and not a dependency shopping list.

```text
one source exposes authored notation
one exposes driver execution
one exposes device state
one exposes synthesis graphs
one exposes routing and automation
one exposes musical structure
one exposes rendered audio
one exposes perceptual organization
one exposes historical authoring practice
one exposes musicological analysis
one exposes how humans naturally describe the result
        ↓
compare the strata
        ↓
identify distinctions that survive
        ↓
implement only the useful common mechanisms
        ↓
keep source-specific truth attached underneath
```

A source also has an evidence role. The VGM specification is stronger than an emulator for deciding what a VGM command byte means. A chip manual can be stronger than a wiki summary for documented register behavior. A mature emulator or die-analysis implementation can reveal behavior the original manual omitted. A surviving driver can reveal how a game actually used the hardware. These sources should cross-check one another rather than being flattened into one anonymous authority.

See:

- `docs/musical-execution-model.md`
- `docs/musical-inference-evidence.md`
- `docs/musical-understanding-dependencies.md`
- `docs/music-representation-systems.md`
- `docs/human-musical-discourse.md`
- `docs/audio-programming-languages.md`
- `docs/persistent-musical-identity.md`
- `docs/source-native-enhanced-rendering.md`
- `docs/vgm-frontier.md`
- `docs/upstreams.md`
- `research/cases/vgm-cross-chip-controls.md`
- `research/cases/harmonic-formal-analysis.md`
- `research/cases/musicological-authorship-attribution.md`
- `research/cases/sonic-smps-pitch-recovery.md`
- `research/cases/retro-composition-programming-listening.md`
- `research/cases/human-musical-discourse.md`

## Levels of truth

Do not collapse these claim types:

```text
SOURCE / AUTHORED REPRESENTATION
bytes, commands, MML, score/pattern source, MIDI, native sequence/container,
ROM-derived data, VGM, SPC and other preserved objects

PROGRAM / CONTROL FLOW
patterns, loops, branches, macros, scheduler structure, driver program points,
legal transitions and adaptive/generative future behavior

DRIVER / SCHEDULER EXECUTION
realized control flow, logical tracks, execution traces, allocation, modulation,
loops, runtime program state

DEVICE / SYNTHESIS
registers, operators, oscillators, partials, BRR/PCM/ADPCM samples, envelopes,
physical voice episodes, effects, routing and device state

MUSICAL PERFORMANCE
note-like events, programmed/transposed pitch coordinates, pitch/dynamic/
articulation trajectories, programmed gestures, persistent parts when supported,
instrument relationships and authored controls

AUDITORY INTERPRETATION
what a listener/model hears as events, streams, foreground/background, masking,
salience, perceived pitch/rhythm, timbral grouping and other perceptual organization

MUSICAL STRUCTURE / THEORY
meter, beat hierarchy, harmonic segmentation, chord/figuration hypotheses,
local/global tonality, harmonic function, progression, harmonic rhythm,
voice leading, cadence, motif, phrase, form, repetition and transformation

ACOUSTIC REALIZATION
reference hardware behavior or source-native enhanced realization

LISTENER RESPONSE
expectation, familiarity, memory, emotion, groove, pleasure, attention and other
listener/model responses under an explicit context

MUSICOLOGICAL CONTEXT
work/version relations, documentary history, credits, attribution hypotheses,
transmission, ports/adaptations and external evidence
```

Human discourse is **not** another truth layer. It is a projection over claims from these layers.

Later abstraction is not automatically greater truth. `semantic layer`, `evidence status`, `provenance`, `capture quality`, and `discourse register` answer different questions.

A physical chip channel is not automatically a persistent musical voice. A register log is not automatically the original score. A nominal chip frequency is not automatically a heard pitch. A compiled chromatic coordinate is not automatically a written note spelling. A sounding pitch is not automatically a chord tone. A pitch-class set is not automatically a chord function. A local tonicization is not automatically the global key. A section boundary is not automatically a formal analysis. A perceptual stream is not automatically one physical source. A stylistic match is not authorship proof. A perceptually similar reconstruction is not recovered historical source truth. A natural metaphor is not an exact source fact.

## The current common model

The repository contains a small provenance-aware musical execution graph in `model/musical_execution_graph.h` plus source-relative analysis features in `model/analysis_feature.h`.

The graph distinguishes events, persistent values, controls, streams, graph topology, source objects, logical processes, program points, execution traces, synthesis objects, physical voice episodes, parts, auditory objects, and provenance-bearing relations.

Current executable representation rules include:

1. static program/control-flow structure is distinct from one realized traversal;
2. execution traces preserve repeated visits to the same program point;
3. timestamp and source/execution order are separate when order can affect state;
4. capture windows do not create fake musical-execution identities;
5. overflow and partial payload capture remain explicit evidence defects;
6. observation gaps invalidate stateful semantic continuation until exact resynchronization;
7. device transitions remain distinct from musical-performance observations;
8. bounded physical voice episodes remain distinct from persistent musical parts;
9. device-native pitch/control history is represented before MIDI or note-name interpretation;
10. programmed/transposed pitch, nominal device frequency, performed pitch, heard pitch, note spelling, pitch class, and harmonic role remain separately earnable claims;
11. authored, driver, device, sample, acoustic and analytical clocks remain separate and are joined by explicit mappings;
12. analysis features preserve claim layer, availability, evidence strength, confidence and provenance;
13. competing musical, perceptual, listener and historical hypotheses may coexist over unchanged lower evidence;
14. timbre similarity, instrument identity and organological identity remain separate claims;
15. technical realization fingerprints do not silently become composer attribution;
16. higher inferred analysis retains its dependency route rather than erasing uncertainty underneath it;
17. human-facing wording does not alter the evidence status of the claims it summarizes;
18. file-format semantics and chip/device semantics remain separate evidence layers when a format specification exists;
19. a cross-chip abstraction is earned by agreement across independent device families and may be rejected by a negative control.

The graph is not finished architecture by declaration. New abstractions must be earned by real source adapters, cross-representation controls, or validation failures.

## Musical understanding is a dependency graph

The useful orientation is:

```text
SOURCE / EXECUTION / SYNTHESIS
        ↓
MUSICAL PERFORMANCE
        ↓
AUDITORY ORGANIZATION
        ↓
PITCH / RHYTHM / PART RELATIONS
        ↓
HARMONY / TONALITY
        ↓
PROGRESSION / VOICE LEADING / HARMONIC RHYTHM
        ↓
CADENCE / PHRASE / MOTIVIC RELATION
        ↓
FORM / HIERARCHY
        ↓
STYLE / WORK / VERSION / AUTHORSHIP ANALYSIS
        ↓
HUMAN DISCOURSE
```

But analysis order is not truth order. Strong direct evidence may enter at any layer it genuinely supports. A validated sequence can establish a programmed pitch before an audio model has inferred it. A surviving score may establish written spelling more strongly than device execution can. A libaural observation may establish a perceptual grouping that no register trace contains. An archival document may constrain authorship without passing through harmonic analysis.

The central rule is:

> **Higher analysis may summarize lower evidence, but it may not erase, silently repair, or skip the uncertainty of the evidence it depends on.**

This is how the system is meant to learn the distinctions rather than merely memorize music-theory vocabulary.

## The song-level reasoning target

The common graph is the evidence substrate. It is not the final user-facing level of musical understanding.

A higher analysis should be able to construct a synchronized song-level view over the existing evidence without inventing a second canonical ontology.

At one time span, reasoning should be able to inspect together:

```text
source bytes / commands
+ driver state / control flow
+ synthesis objects / samples / patches
+ physical voice episodes
+ programmed pitch / transposition / detune / modulation
+ realized musical events
+ persistent-part hypotheses
+ rendered audio / acoustic measurements
+ auditory streams / grouping
+ melody, bass, percussion and other role hypotheses
+ harmonic segmentation
+ chord-tone / figuration hypotheses
+ chord / root / inversion candidates
+ local / global tonal-center candidates
+ harmonic function / progression / harmonic rhythm
+ voice-leading / counterpoint relations
+ cadence / phrase / motif / formal hierarchy
+ texture / density / register / timbral organization
+ loop / repetition behavior
+ game-context annotations when externally known
+ attribution / historical evidence
```

This enables questions such as:

- What enters or leaves the texture here?
- Which physical voices realize the same persistent musical part?
- Where does the melody migrate between timbres or channels?
- Which programmed envelope, transposition, detune, or modulation creates this gesture?
- What key or tonal center is locally active, and how certain is that claim?
- Which sounding pitches are structural chord tones versus figuration?
- What chord/function candidates are supported here?
- What progression is unfolding, and at what harmonic rhythm?
- How do bass motion and upper voices create the voice leading?
- What cadence is being prepared or evaded?
- Why does the return feel bigger or more settled when some note material is similar?
- How do bass, percussion and upper voices interlock?
- Which changes are structural and which are ornamental?
- How does the loop close musically, not only byte-wise?
- What source evidence supports this phrase/form interpretation?
- What would another analytical or listener model plausibly hear differently?

The safe rule is:

> **reason holistically, claim locally.**

The song-level account may combine many layers, but every assertion retains its own evidence scope.

## Human musical discourse

A technically correct musical analysis can still sound unlike anything a listener, critic, composer, musicologist, producer, or engineer would naturally say.

Human musical discourse is strongly metaphorical, relational, embodied and purpose-dependent. People routinely talk about music through motion, space, force, weight, material, light, colour, temperature, breath, architecture, conversation, narrative and energy. Musicologists and theorists additionally move among keys, chords, progressions, counterpoint, cadence, motive, form, style, sources, transmission, and attribution.

Examples include:

```text
it opens up here
the bass starts digging in
the groove lurches forward
the chorus finally lifts
the synth sneaks in behind the melody
this dominant area keeps delaying the return to tonic
the cadence is weakened by the upper voice
the return keeps the motive but reharmonizes it
the mix feels boxed-in
the snare needs more bite
let the verse breathe
this section never quite settles
```

These are not automatically vague or incorrect. They become grounded when the system can identify the musical/acoustic observations supporting them.

### Discourse modes

```text
ORDINARY LISTENER
what changed, what stands out, what it feels like

REVIEWER / CRITIC
description + evaluation + metaphor + comparison + cultural framing

COMPOSER / MUSICIAN
shape, intention, pacing, contrast, gesture, thematic/harmonic construction,
interaction among parts

MUSICOLOGIST / THEORIST
pitch organization, key/tonality, chord, function, progression, counterpoint,
voice leading, cadence, motif, phrase, form, style, work/version, source,
transmission and attribution

PRODUCER
what the song needs: lift, energy, impact, space, contrast, hook, momentum

MIXING / MASTERING ENGINEER
position, width, spectral balance, body, punch, cohesion, dynamics, clarity

FORENSIC / TECHNICAL
exact source, driver, synthesis, timing, routing and measurement evidence
```

These are discourse modes, not immutable people. One person may switch registers in the same conversation.

### Discourse acts

The system should separately understand whether the speaker is:

```text
describing
comparing
interpreting
evaluating
diagnosing
directing a change
explaining a mechanism
reporting documented intent
```

Evaluation is not source truth. Creator intent requires documentary evidence. A production or engineering diagnosis remains a hypothesis or judgment unless its causal route is independently proven.

### Many-to-many language law

Do not build a phrase dictionary such as:

```text
higher spectral centroid = bright
more voices = bigger
more stereo width = open
pitch set {C,E,G} = tonic function
```

Human descriptors and music-theory interpretations are many-to-many and context-dependent.

```text
one technical change
→ several possible human descriptions

one human description
← several possible technical causes

one pitch collection
→ several possible harmonic functions

one harmonic function
← several possible voicings / surface sonorities
```

`It opens up here` could arise from added parts, increased upper-register activity, less masking, wider spatial spread, more ambience, lower density, longer sustain, a timbral change, or a combination.

Conceptually:

```text
claim / comparison
+ support bundle
+ confidence
+ analytical assumptions where applicable
+ competing interpretations
+ discourse mode
+ discourse act
+ requested detail
→ natural description
```

The wording is a projection. The support bundle carries the evidence.

### Progressive disclosure

Default discussion should sound like a knowledgeable person listening to music, not a telemetry dump.

```text
USER
What happens here?

VGM TOOLING
It opens up and starts pushing harder.

USER
What changed musically?

VGM TOOLING
The upper part returns over a changed harmonic support, while the bass starts driving the phrase toward a stronger arrival.

USER
What is the harmony doing?

VGM TOOLING
[local key / chord-function / progression / cadence explanation with explicit uncertainty]

USER
What exactly creates that?

VGM TOOLING
[performed pitch / part / driver / envelope / articulation explanation with exact provenance]
```

The target is:

> **Speak like people speak about music; know exactly why you are saying it.**

## Retro authorship is role-relative

Practitioner interviews and surviving source code show that there was no universal pipeline of “composer writes notes, programmer implements them.” In some traditions composition, arrangement, sound programming, synthesis design and driver work were deeply entangled; in others they were distributed among different people and fed back into one another.

For attribution and style analysis, use parallel evidence coordinates rather than one giant `composer fingerprint`.

```text
COMPOSITION
melody • rhythm • harmony • counterpoint • cadence • form • motivic habits
large-scale tonal planning

ARRANGEMENT / SOUND PROGRAMMING
register • voicing • texture • channel roles • modulation idioms • effects
articulation • envelopes • control tricks • machine-specific realization choices

DRIVER / TOOLCHAIN
command grammar • allocation behavior • scheduler idioms • data layout
compiler / driver artifacts

PATCH / SAMPLE DESIGN
FM topology • waveforms • patch parameters • sample preparation • loop strategy

RENDERING
levels • echo / reverb strategy • mixing • hardware-specific acoustic realization
```

`ARRANGEMENT / SOUND PROGRAMMING` is intentionally one coordinate. In retro executable music, decisions about voicing, channel assignment, texture, modulation, articulation and effects often form one continuous realization practice rather than two clean historical roles.

The same person can occupy several coordinates. Several people can contribute to one finished cue. Evidence may point to different people on different coordinates.

```text
strong driver fingerprint
!= composer proof

strong arrangement / sound-programming fingerprint
!= composition proof

shared patch/sample habits
!= shared authorship
```

Attribution begins with historical possibility and candidate generation, not nearest-style classification. Source/transmission evidence, chronology, instrumentation, documentary context, compositional structure, and computational fingerprints are independent coordinates that may converge or disagree. The valid result space includes candidate, shared/collaborative, school/circle/team, unknown, and none-of-the-current-candidates.

Attribution claims remain hypotheses until documentary or otherwise independent evidence justifies stronger status.

## Programmed expression is music, not residue

Expressive-performance research and practitioner evidence both reject a simple “notes first, expression later” model.

For executable game music, exact musical instructions may include:

- gate length;
- attack/release shaping;
- volume trajectories;
- vibrato and delayed vibrato;
- portamento and detune;
- pitch envelopes and arpeggiation;
- pulse-width/duty changes;
- FM operator/envelope changes;
- waveform/sample changes;
- sample start/loop behavior;
- retrigger and note-stealing policy;
- rhythmic echo/delay;
- exact machine-specific timing relationships.

These may be part of composition/realization rather than implementation debris.

```text
exact programmed control
        ↓ supports
derived musical gesture
        ↓ may support
higher expressive / structural interpretation
```

The interpretation never replaces the exact control history.

## Forward and inverse problems

VGM Tooling operates in both directions.

Forward:

```text
authored musical program
→ control flow / scheduler / driver
→ realized execution
→ synthesis / routing
→ acoustic realization
→ auditory organization
```

Inverse:

```text
VGM / SPC / executable state / ROM / audio
→ recover exact execution where possible
→ recover program / driver structure where evidence permits
→ recover synthesis objects and physical voice episodes
→ recover conservative performance events and controls
→ recover programmed/transposed pitch coordinates when source semantics support them
→ recover persistent identities and musical structure when supported
→ infer harmony / tonality / phrase / form without erasing competing readings
→ construct a coherent listening-level account without erasing uncertainty
→ express that account naturally without strengthening the claims
```

Where both directions can be built independently, they form a strong validation pair.

## Capability is source-relative

Different source families expose different semantic depths.

```text
not exposed
≠ absent

unknown
≠ false

not applicable
≠ unavailable
```

A tracker may expose patterns and instruments. A VGM log may expose exact register writes but not the original score. An SPC may preserve executable state without making its driver grammar explicit. A broad replay library may expose isolated voices for one engine and only final PCM for another.

Do not fabricate parity merely because a caller prefers one uniform shape.

## Accuracy is the foundation, not the ceiling

The accurate renderer is the scientific reference. It is not the quality ceiling.

The enhanced renderer is not a generic "HD remaster," not a MIDI/SoundFont conversion path, and not permission to replace historical instruments with modern substitutes. Its target is a **counterfactual source-native realization** of the same executable musical idea.

```text
same piece
same parts
same musical gestures
same recognizable instruments
same arrangement
        ↓
remove only technical ceilings that are not identity-bearing
        ↓
let the intended sound breathe farther
```

Historical constraints are not presumed to be defects. Practitioner evidence shows both sides: storage, tooling and fidelity limits sometimes forced unwanted compromise, while other limitations were deliberately exploited until the artifact became part of the instrument itself.

The durable rule is:

> **Recover intention where evidence exists, relax unwanted implementation ceilings where identity survives, and preserve the constraints that the music actually adopted as part of itself.**

Preserve:

- notes and exact timing;
- rhythm, groove and phrasing;
- instrument/patch/sample relationships;
- programmed articulation, modulation and automation;
- musical hierarchy and texture;
- deliberate effects;
- meaningful hardware behavior that became part of the instrument or realization.

Enhance where evidence supports it:

- source reconstruction;
- bandwidth/interpolation;
- transient fidelity and low-frequency body;
- synthesis quality/precision;
- masking and separation;
- mixing precision/headroom;
- source extent and environmental rendering;
- stereo presentation.

For sample systems, an upstream uncompressed sample is evidence, not an automatic replacement. Historical editing, filtering, truncation, tuning, loop preparation, encoding, envelopes and effect interaction may have become part of the shipped instrument.

For FM systems, keep the programmed synthesis identity intact:

```text
same algorithm
same operator relationships
same envelopes
same feedback / modulation
same timing / articulation
        ↓
higher-quality source-native realization
```

The target is the best plausible descendant of the same FM instrument, not `YM2612 → unrelated DX preset`.

Every relaxed ceiling is a separate reversible experiment against the accurate renderer. Do not change synthesis precision, reconstruction, mixing, spatial presentation and effects at once and then call the result an improvement.

SoundFont, DLS, MIDI, VST and modern sampler/synth ecosystems are observatories for rendering practice and interoperability. They are not the canonical representation or planned foobar playback backend.

See `docs/source-native-enhanced-rendering.md` for the evidence ladder and validation law.

The long-term playback target is:

> **Every supported soundtrack should aim to sound like the highest-quality realization its surviving musical data and preserved instrument identity can support.**

## Realtime playback and offline analysis

Normal playback remains realtime. Whole-song preprocessing, stem export or reverse compilation must not become a prerequisite for hearing a track.

The broader project may still perform offline/forensic/song-level analysis when the question requires it. Those analyses should consume captured/source evidence and remain optional to realtime playback.

```text
realtime source / execution
        ↓
bounded allocation-free capture
        ↓
continuous cheap auditory / performance state where applicable
        ↓
non-realtime or slower-cadence graph materialization / analysis
        ↓
synchronized technical + musical + perceptual + musicological reasoning
        ↓
human discourse projection when requested
```

Source/audio time, auditory-state update time, musical-analysis time, and LLM reasoning time are not assumed to be the same clock.

## Current engineering centers

### Cross-chip VGM / Yamaha families

The current VGM work is no longer allowed to equate “VGM semantics” with “YM2612 semantics.”

The format layer now decodes versioned clocks, dual/variant flags, the Yamaha register-write transport, generic DAC Stream Control, and structural timing/loop information before handing writes to family-specific state models.

The first Yamaha comparison already produced both positive and negative results:

```text
OPN
YM2203 • YM2608 • YM2610/B • YM2612/3438
shared OPN register geometry where proven

OPM
YM2151/2164
separate key-code/key-fraction pitch model

OPL
YM3526 • Y8950 • YM3812 • YMF262
separate two-op / dynamic-four-op connection model

OPLL
YM2413 family
preset/user instrument provenance + narrower FNUM model
```

OPN and OPM independently earned only a narrow shared four-operator invariant: algorithm/feedback packing and the physical `1,3,2,4` operator-register order. OPL explicitly falsifies that packing as universal Yamaha behavior, so the common helper remains narrow.

Pitch is converging one layer higher. A regression shows YM3812, YMF262, and YM2413 reaching the same approximately `439.990595 Hz` nominal channel basis through different FNUM widths and native clock divisors. The shared coordinate is earned after device semantics, not imposed by first converting everything to MIDI notes.

`tools/vgm_corpus_audit.py` is now the format-level real-corpus admission test. On the immutable Sonic set it validates all 58 files, all declared total-sample counts, and all 57 declared loops including command-boundary and loop-duration consistency.

Additional VGMRips families should be admitted as a small orthogonal control matrix rather than as a giant undifferentiated archive. See `research/cases/vgm-cross-chip-controls.md`.

Official Sega/Yamaha development and hardware manuals preserved through archives such as Sega Retro are useful primary-source referees when reverse-engineered implementations disagree or when documented versus undocumented behavior matters.

This work also makes future state-aware assistance with VGM loop validation and preservation-set preparation plausible. The current tooling validates existing loops; it does not yet discover or rewrite them.

### Mega Drive / Genesis

The current VGM vertical slice reaches conservative performance truth from exact command capture:

```text
exact VGM command
→ decoded device transition
→ bounded physical voice episode
→ conservative pitched-activity observation
→ device-native pitch control
→ source-clock-normalized nominal pitch
→ source-compatible programmed-pitch hypotheses where driver semantics permit
→ persistent musical identity remains source/evidence dependent
```

The repository also contains source-state and enhanced-rendering work for SN76489-family PSG and YM2612 DAC/PCM paths. The YM2612 FM boundary preserves captured register timing in absolute 44.1 kHz VGM ticks and passes the whole ordered timed-write block to the synthesis backend. The backend declares source chip clock and output sample rate, owns tick → native synthesis clock → output-rate conversion, and reports algorithmic latency. VGM tick numbers are not silently treated as output frames.

The real 58-track Sonic 3 & Knuckles VGZ corpus is now an immutable pressure surface rather than a hypothetical fixture. Driver/source comparison has earned several pitch distinctions without collapsing them:

```text
YM2612 FNUM/BLOCK
→ nominal source-clock frequency
!= performed/heard pitch

SMPS note token + transposition
→ programmed chromatic pitch coordinate
!= original written spelling

observed VGM frequency
→ source-compatible table-pitch + displacement hypotheses
!= recovered source note by nearest-frequency rounding
```

A corpus audit currently shows that isolated FM key-ons are genuinely ambiguous under the legal SMPS displacement model, while temporal continuity collapses much of that ambiguity. Distinctive recovered displacement states have been cross-checked against surviving S&K sequence sources such as IceCap and Knuckles. These results are a bridge toward time-bearing pitch/part trajectories, not permission to call the downstream VGM an exact reconstruction of the original SMPS sequence.

The next musical-analysis milestone is stable time-bearing pitch/part evidence suitable for harmonic segmentation. The next audible FM milestone remains a mature coherent six-channel YM2612 renderer with isolated channel output, exact patch/control semantics, and high-quality clock-correct rate conversion before selected hardware constraints are experimentally relaxed.

See `docs/vgm-frontier.md`, `research/cases/vgm-cross-chip-controls.md`, and `research/cases/sonic-smps-pitch-recovery.md` for the engineering/research frontier rather than inferring audible status from model commits.

### SPC / Super NES

The SPC path preserves snapshot/runtime S-DSP state, bounded voice episodes, BRR sample/version evidence, pitch rate, envelopes, key/release state and runtime sample-source observations. Higher driver/sequence identity remains source-relative and should be recovered only when evidence permits.

The editable SNESAPU lineage is the implementation foundation; the supplied SPCPlay/SNESAPU 2.21.3.9130 package remains a newer behavior/version reference.

## Project relationships

```text
                         Helix
             project state / research / tests
                           │
                           ▼
                     VGM Tooling
      executable game-music understanding + rendering
              │             │             │
              ▼             ▼             ▼
          foobar2000     libaural      Sonic 3
          playback       hearing       attribution
              │          testground     testbed
              ▼             ▼             │
          Omniphony     artificial         │
                       hearing             │
                          ▲                │
                          └────────────────┘
```

### Helix

Helix owns project orientation, research questions, evidence continuity, negative results, cross-project transfer, and re-entry state. This repository owns executable game-music implementation and local tests/history.

### libaural

libaural owns general artificial hearing. VGM Tooling can provide unusually strong answer keys by pairing exact hidden source/execution state with a controlled acoustic render and comparing what a hearing model infers from audio alone.

The transfer is bidirectional: libaural contributes auditory-object, grouping, continuity, masking, pitch, timbre, spatial and memory evidence when a musical question depends on what is heard; VGM Tooling contributes source-authoritative controls that can measure where perception agrees with, fuses, splits, or reinterprets physical execution.

### Sonic 3 Music Attribution

The Sonic 3 Helix project is the eventual adversarial testbed for the upper stack because it combines unusually rich executable evidence with difficult real historical attribution. It is **not** permission to rush unfinished musical inference into authorship claims.

VGM Tooling may eventually contribute composition-facing coordinates such as melody, rhythm, harmony, counterpoint, cadence, form and motivic behavior alongside realization-facing coordinates such as voicing, texture, patches, modulation and driver fingerprints. The attribution project independently decides what those coordinates can support.

```text
composer
!= arranger / implementation author

technical fingerprint
!= composition proof

one version
!= every version of the work
```

The testbed becomes valuable precisely because lower layers can be validated before the attribution layer is allowed to consume them.

### Omniphony

Omniphony owns general headphone spatial presentation. VGM Tooling supplies excellent source-aware PCM and may later expose compact source-supported side information. Chip-specific machinery stays here.

## Validation law

Every audible enhancement must remain reversible and be compared with the accurate/reference render.

Semantic recovery should use paired-direction controls where possible:

```text
authored source
→ known compiler / driver / synth
→ execution trace
→ recovered common model
→ compare with source truth
```

Cross-chip validation adds another pressure test:

```text
family A exact machine semantics
+ family B exact machine semantics
        ↓
shared abstraction candidate
        ↓
family C negative/positive control
        ↓
retain, narrow, or reject the abstraction
```

Musical analysis should additionally test whether uncertainty contracts only when new evidence actually earns it:

```text
one instant
→ competing pitch / part / harmony hypotheses

+ continuity / meter / source semantics / auditory grouping
→ fewer surviving hypotheses

+ known symbolic or driver source
→ independent validation or correction
```

Song-level reasoning adds another paired validation:

```text
HUMAN-FACING
Does the description sound like something a musically knowledgeable person would naturally say about what is heard?

MUSICOLOGY-FACING
Can key, chord, progression, cadence, form, style, or attribution claims expose their analytical assumptions and competing readings?

EVIDENCE-FACING
Can each material claim descend into the strongest available musical, acoustic, historical, or executable evidence?
```

A result fails if it is technically correct but linguistically alien, natural-sounding but unsupported, or musicologically confident only because a lower ambiguity was silently discarded.

Current model regressions protect boundaries around program versus trace, capture completeness, device versus musical events, physical voice episodes versus persistent parts, device-native pitch controls, nominal/programmed pitch semantics, time mappings, source-relative analysis availability, analysis dependency routes, competing theoretical/perceptual interpretations, listener-response context, work/version identity, timbre/instrument identity, role-relative attribution, VGM format/version semantics, Yamaha family boundaries, and cross-family nominal pitch normalization.

Human discourse currently remains a reasoning/projection rule rather than a new graph primitive.

Measurements should catch structural regressions, but listening remains decisive for perceptual quality.

## Historical lineage

This repository supersedes the earlier private `dissonance-git/vgmspc` implementation line. Useful state/provenance ideas survive; premature semantic-role heuristics and older spatial-rendering architecture remain historical evidence rather than current truth.

`docs/first-pass.md` is a historical bootstrap handoff, not the current roadmap. `docs/history.md` preserves repository lineage. Current orientation belongs here, current engineering status in source-family frontier docs, and detailed research evidence in `research/cases/`.

## Final target

VGM Tooling should become a comprehensive, provenance-preserving implementation for understanding digital game music across authored source, executable state, driver behavior, synthesis, musical performance, auditory organization, harmony/tonality, voice leading, cadence, form, acoustic realization, perception, historical/musicological context, and human discourse.

But the project should not stop at a stack of layers.

> **The layers exist so the system can understand the music as music without losing the machine that made it.**

For Helix-facing reasoning, the ideal endpoint is a supported soundtrack that can be inspected both as an executable musical system and as a coherent heard/composed work: queried for key, harmony, progression, form, texture, performance, realization, and historical hypotheses; discussed naturally from listener through musicologist to forensic depth; and always able to descend back into the exact evidence that supports each claim.
