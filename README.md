# VGM Tooling

Executable understanding, analysis, source-native rendering, and human-readable reasoning for digital game music.

`VGM` in the project name is historical shorthand for video game music tooling. The project is not limited to the `.vgm` format, and the foobar2000 components are important realtime frontends rather than the definition of the project.

## Objective

VGM Tooling should understand a supported game-music object deeply enough to do three things at once:

1. **explain and render the machinery that makes the music happen**, from authored/programmed source through driver execution, synthesis, routing, effects, and final audio;
2. **reason about the song a listener actually hears as a coherent musical object**, including parts, gestures, texture, phrases, sections, motifs, form, timbral organization, repetition, and larger musical behavior;
3. **discuss that music naturally**, using the kind of language appropriate to a listener, reviewer, composer, producer, mixing/mastering engineer, or technical analyst without losing the evidence underneath the wording.

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
auditory organization + musical structure
        ↓
        THE SONG AS HEARD
        coherent, time-aligned, queryable
        ↓
human discourse projection
listener • critic • creator • producer • engineer • forensic
        ↕
every material claim can descend back
into source / execution evidence
```

A supported object should therefore not collapse immediately into MIDI, stems, or stereo PCM. Those are useful projections. The internal representation must retain the route among source truth, execution truth, performed musical behavior, acoustic realization, listening-level structure, and human explanation.

The long-term target is not merely to answer:

> What register changed at this time?

It is also to answer naturally:

> What happens here?

and then, if asked:

> Why does it feel different, what changed musically, and what exact programmed or acoustic behavior creates that effect?

## One phenomenon, many observatories

VGM Tooling deliberately studies mature systems that expose different strata of digital music:

- VGM/VGZ and register-log tooling;
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

See:

- `docs/musical-execution-model.md`
- `docs/musical-inference-evidence.md`
- `docs/music-representation-systems.md`
- `docs/human-musical-discourse.md`
- `docs/audio-programming-languages.md`
- `docs/persistent-musical-identity.md`
- `docs/vgm-frontier.md`
- `docs/upstreams.md`
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
note-like events, pitch/dynamic/articulation trajectories, programmed gestures,
persistent parts when supported, instrument relationships, authored controls

MUSICAL STRUCTURE
meter, beat hierarchy, harmony, motif, phrase, section, form, repetition,
transformation, texture and higher musical relations

ACOUSTIC REALIZATION
reference hardware behavior or source-native enhanced realization

AUDITORY INTERPRETATION
what a listener/model hears as events, streams, foreground/background, masking,
salience, perceived rhythm, timbral grouping and other perceptual organization

LISTENER RESPONSE
expectation, familiarity, memory, emotion, groove, pleasure, attention and other
listener/model responses under an explicit context

MUSICOLOGICAL CONTEXT
work/version relations, documentary history, credits, attribution hypotheses,
transmission, ports/adaptations and external evidence
```

Human discourse is **not** another truth layer. It is a projection over claims from these layers.

Later abstraction is not automatically greater truth. `semantic layer`, `evidence status`, `provenance`, `capture quality`, and `discourse register` answer different questions.

A physical chip channel is not automatically a persistent musical voice. A register log is not automatically the original score. A perceptual stream is not automatically one physical source. A stylistic match is not authorship proof. A perceptually similar reconstruction is not recovered historical source truth. A natural metaphor is not an exact source fact.

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
9. device-native pitch/control history is represented before MIDI interpretation;
10. authored, driver, device, sample and acoustic clocks remain separate and are joined by explicit mappings;
11. analysis features preserve claim layer, availability, evidence strength, confidence and provenance;
12. competing musical, perceptual, listener and historical hypotheses may coexist over unchanged lower evidence;
13. timbre similarity, instrument identity and organological identity remain separate claims;
14. technical realization fingerprints do not silently become composer attribution;
15. human-facing wording does not alter the evidence status of the claims it summarizes.

The graph is not finished architecture by declaration. New abstractions must be earned by real source adapters or validation failures.

## The song-level reasoning target

The common graph is the evidence substrate. It is not the final user-facing level of musical understanding.

A higher analysis should be able to construct a synchronized song-level view over the existing evidence without inventing a second canonical ontology.

At one time span, reasoning should be able to inspect together:

```text
source bytes / commands
+ driver state / control flow
+ synthesis objects / samples / patches
+ physical voice episodes
+ programmed modulation and articulation
+ realized musical events
+ persistent-part hypotheses
+ rendered audio / acoustic measurements
+ auditory streams / grouping
+ melody, bass, percussion and other role hypotheses
+ texture / density / register / timbral organization
+ motif / phrase / section / form relations
+ loop / repetition behavior
+ game-context annotations when externally known
+ attribution / historical evidence
```

This enables questions such as:

- What enters or leaves the texture here?
- Which physical voices realize the same persistent musical part?
- Where does the melody migrate between timbres or channels?
- Which programmed envelope or modulation creates this gesture?
- Why does the return feel bigger when the note material is similar?
- How do bass, percussion and upper voices interlock?
- Which changes are structural and which are ornamental?
- How does the loop close musically, not only byte-wise?
- What source evidence supports this phrase/section interpretation?
- What would another analytical or listener model plausibly hear differently?

The safe rule is:

> **reason holistically, claim locally.**

The song-level account may combine many layers, but every assertion retains its own evidence scope.

## Human musical discourse

A technically correct musical analysis can still sound unlike anything a listener, critic, composer, producer, or engineer would naturally say.

Human musical discourse is strongly metaphorical, relational, embodied and purpose-dependent. People routinely talk about music through motion, space, force, weight, material, light, colour, temperature, breath, architecture, conversation, narrative and energy.

Examples include:

```text
it opens up here
the bass starts digging in
the groove lurches forward
the chorus finally lifts
the synth sneaks in behind the melody
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
shape, intention, pacing, contrast, gesture, interaction among parts

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
```

Human descriptors are many-to-many.

```text
one technical change
→ several possible human descriptions

one human description
← several possible technical causes
```

`It opens up here` could arise from added parts, increased upper-register activity, less masking, wider spatial spread, more ambience, lower density, longer sustain, a timbral change, or a combination.

Conceptually:

```text
claim / comparison
+ support bundle
+ confidence
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
What changed?

VGM TOOLING
A higher part comes in, the bass gets more pointed, and the sound spreads out.

USER
What exactly makes the bass more pointed?

VGM TOOLING
[driver / envelope / articulation explanation with exact provenance]
```

The target is:

> **Speak like people speak about music; know exactly why you are saying it.**

## Retro authorship is role-relative

Practitioner interviews and surviving source code show that there was no universal pipeline of “composer writes notes, programmer implements them.” In some traditions composition, arrangement, sound programming, synthesis design and driver work were deeply entangled; in others they were distributed among different people and fed back into one another.

For attribution and style analysis, use parallel evidence coordinates rather than one giant `composer fingerprint`.

```text
COMPOSITION
melody • rhythm • harmony • form • motivic habits

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
```

Inverse:

```text
VGM / SPC / executable state / ROM / audio
→ recover exact execution where possible
→ recover program / driver structure where evidence permits
→ recover synthesis objects and physical voice episodes
→ recover conservative performance events and controls
→ recover persistent identities and musical structure when supported
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

Enhanced rendering may exceed historical storage, interpolation, synthesis precision, DAC, bandwidth, mixing, and effects limitations when the result remains traceable to the encoded musical work.

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

A hardware limitation is not automatically artistic intent. A hardware artifact or limitation that materially defines the authored/programmed instrument may be.

The long-term playback target is:

> **Every supported soundtrack should aim to sound like the highest-quality realization its surviving musical data can support.**

## Realtime playback and offline analysis

Normal playback remains realtime. Whole-song preprocessing, stem export or reverse compilation must not become a prerequisite for hearing a track.

The broader project may still perform offline/forensic/song-level analysis when the question requires it. Those analyses should consume captured/source evidence and remain optional to realtime playback.

```text
realtime source / execution
        ↓
bounded allocation-free capture
        ↓
non-realtime graph materialization / analysis
        ↓
synchronized technical + musical + perceptual reasoning
        ↓
human discourse projection when requested
```

## Current engineering centers

### Mega Drive / Genesis

The current VGM vertical slice reaches conservative performance truth from exact command capture:

```text
exact VGM command
→ decoded device transition
→ bounded physical voice episode
→ conservative pitched-activity observation
→ device-native pitch control
→ persistent musical identity remains source/evidence dependent
```

The repository also contains source-state and enhanced-rendering work for SN76489-family PSG and YM2612 DAC/PCM paths. The next audible FM milestone remains a mature six-channel YM2612 renderer with isolated channel output and exact patch/control semantics before selected hardware constraints are experimentally relaxed.

See `docs/vgm-frontier.md` for the engineering frontier rather than inferring audible status from model/research commits.

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
          foobar2000     libaural      attribution
          playback       testground     / forensics
              │             │
              ▼             ▼
          Omniphony     artificial hearing
```

### Helix

Helix owns project orientation, research questions, evidence continuity, negative results, cross-project transfer, and re-entry state. This repository owns executable game-music implementation and local tests/history.

### libaural

libaural owns general artificial hearing. VGM Tooling can provide unusually strong answer keys by pairing exact hidden source/execution state with a controlled acoustic render and comparing what a hearing model infers from audio alone.

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

Song-level reasoning adds another paired validation:

```text
HUMAN-FACING
Does the description sound like something a musically knowledgeable person would naturally say about what is heard?

EVIDENCE-FACING
Can each material claim descend into the strongest available musical, acoustic, historical, or executable evidence?
```

A result fails if it is technically correct but linguistically alien, or natural-sounding but unsupported.

Current model regressions protect boundaries around program versus trace, capture completeness, device versus musical events, physical voice episodes versus persistent parts, device-native pitch controls, time mappings, source-relative analysis availability, competing theoretical/perceptual interpretations, listener-response context, work/version identity, timbre/instrument identity, and role-relative attribution.

Human discourse currently remains a reasoning/projection rule rather than a new graph primitive.

Measurements should catch structural regressions, but listening remains decisive for perceptual quality.

## Historical lineage

This repository supersedes the earlier private `dissonance-git/vgmspc` implementation line. Useful state/provenance ideas survive; premature semantic-role heuristics and older spatial-rendering architecture remain historical evidence rather than current truth.

`docs/first-pass.md` is a historical bootstrap handoff, not the current roadmap. `docs/history.md` preserves repository lineage. Current orientation belongs here, current engineering status in source-family frontier docs, and detailed research evidence in `research/cases/`.

## Final target

VGM Tooling should become a comprehensive, provenance-preserving implementation for understanding digital game music across authored source, executable state, driver behavior, synthesis, musical performance, musical structure, acoustic realization, perception, historical context, and human discourse.

But the project should not stop at a stack of layers.

> **The layers exist so the system can understand the music as music without losing the machine that made it.**

For Helix-facing reasoning, the ideal endpoint is a supported soundtrack that can be inspected both as an executable musical system and as a coherent heard song, discussed in natural musical language with a reversible path from that conversation back into the exact evidence.
