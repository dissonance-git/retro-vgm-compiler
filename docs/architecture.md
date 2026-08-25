# VGM Compiler architecture

This is the canonical contract for VGM Compiler's shared semantics, evidence discipline, and cross-source abstraction rules.

## Objective

VGM Compiler exists to understand digital game music deeply enough that analysis, reconstruction, explanation, attribution, rendering, conversion, and future porting can be treated as projections over one evidence-carrying musical model.

The difficult part is the middle:

```text
native source / executable state / captured execution / audio
        ↓
source-specific semantics and execution
        ↓
physical synthesis + performed gestures
        ↓
persistent musical parts
        ↓
melody • bass • rhythm • harmony • timbre • articulation
        ↓
motif • phrase • cadence • counterpoint • form • arrangement
        ↓
whole-work / soundtrack model
        ↓
creator grammar • attribution • explanation • transformation
```

No output representation is canonical merely because it is convenient. MIDI, notation, chord labels, stems, prose, rendered PCM, or reconstructed source can all be useful while remaining lossy or hypothetical projections.

## Linked representations, not one universal format

A soundtrack may survive as or be observable through:

```text
MML / score / tracker / MIDI / native sequence
VGM / VGZ register execution
SPC machine snapshot and runtime continuation
NSF / KSS / HES / xSF executable objects
ROM / disassembly / driver source
patches / samples / device state
rendered audio
external transcription or documentary evidence
```

These are complementary sensors over one musical phenomenon. Their distinctions are evidence.

```text
MIDI track
!= authored voice
!= driver logical track
!= hardware channel
!= physical voice episode
!= persistent musical part
!= auditory stream
```

Likewise:

```text
MIDI note
!= sequence command
!= device frequency state
!= performed pitch
!= heard pitch
!= notation spelling
```

and:

```text
MIDI program
!= tracker instrument
!= FM patch
!= sample object
!= audible instrument family
!= orchestration role
```

Cross-representation alignment may be strong without becoming equivalence. A correspondence can be many-to-many, time-varying, and uncertain.

## Source-specific semantics come first

A common model begins after the strongest source-native semantics available to that family have been preserved.

### Authored symbolic/programming sources

MML, score-like source, trackers, MIDI, or validated native sequence data may expose notes, rests, ties, durations, meter, tempo, logical parts, loops, macros, instruments, articulation, modulation, and control flow directly.

Authored truth does not by itself prove later acoustic realization. Compiler, driver, allocation, synthesis, routing, and playback state may transform it.

### Logged execution traces

VGM/VGZ strongly establish the commands and timing present in the log, device clocks, register state, embedded PCM/ROM blocks, and source-local ordering.

They generally do not prove the original driver track, original sequence opcode, composer-facing instrument name, authored notation, or persistent musical identity across dynamic hardware allocation.

### Executable rips and machine snapshots

SPC, NSF/NSFe, HES, KSS/SGC, PSF-family objects and related formats may preserve machine/program state, driver code/data, samples, saved device state, or enough executable context to recover higher semantics through controlled execution.

A valid container or memory image is not automatically a validated running machine, decoded driver, recovered score, or correct playback result.

### Driver and sequence formats

Validated SMPS, GEMS, N-SPC, SSEQ and other engines can expose stronger logical-track and program semantics than a register log while still requiring a target device model for the realized sound.

### Audio and perceptual evidence

Rendered audio can expose grouping, salience, masking, acoustic overlap, timbral relations, groove, and other heard properties unavailable from symbolic or register state alone.

When exact source state exists, an audio-derived estimate does not replace it. It observes a different projection.

## Semantic layer, evidence state, provenance, and availability are orthogonal

Every material claim should make four independent coordinates recoverable.

### Semantic layer

Useful layers include:

```text
source representation
authored program
driver execution
synthesis / routing
musical performance
musical structure
acoustic realization
auditory interpretation
listener response
musicological context
```

The ordering is explanatory, not a ladder of increasing truth. `musicological context` cross-cuts artifacts, versions, structures, performances, renders, and external evidence.

### Evidence state

```text
exact       directly represented or deterministically recovered from validated state
derived     deterministic or tightly constrained transformation of exact evidence
hypothesis  interpretation with plausible alternatives
```

Examples:

- exact VGM register write: exact source/execution evidence;
- device pitch relation from validated registers: derived;
- persistent part assignment without driver identity: hypothesis;
- phrase boundary: usually hypothesis, perhaps strongly supported;
- external documented credit: exact relative to that documentary source, not automatically proof of the role one wishes to infer.

A hypothesis never overwrites its support.

### Provenance

A claim should retain enough support to answer:

- which artifact, executor, observer, model, theory, or documentary witness produced it;
- which lower claims it depends on;
- which transformation connected them;
- which limitations and alternatives remain.

### Availability and capture quality

```text
unknown != false
unavailable != absent
not applicable != unavailable
exact relative to an artifact != complete preservation of the historical event
```

Missing evidence must remain missing. Do not fill gaps with zeroes, false booleans, empty labels, or convenient inferred source history.

## Identity is always scoped

The word `identity` is unsafe without a noun.

```text
artifact identity
!= source-object identity
!= patch/sample identity
!= physical voice episode
!= persistent musical-part identity
!= auditory-stream identity
!= work/version identity
!= authorship identity
```

One work may survive in many byte-distinct artifacts. One part may move across hardware slots. Several physical sources may fuse perceptually. A shared patch may be reused across unrelated works. None of these implies the others.

## Persistent musical identity

Persistent-part recovery is a central bridge from execution to musical understanding because implementation resources are not stable musical entities.

```text
authored / logical part
        ↓ may be allocated through
running synthesis voice
        ↓ occupies
physical hardware slot
```

The mapping may move, split, fuse, restart, be stolen, or become temporarily unobservable.

Evidence should prefer the strongest available source-supported relation:

1. explicit authored part identity;
2. validated driver/logical-track identity;
3. persistent exact synthesis/instrument/sample identity when semantically relevant;
4. control continuity such as pitch, modulation, or envelope trajectories;
5. temporal, contour, and articulation continuity;
6. pitch/register proximity and overlap constraints;
7. perceptual stream/grouping evidence when the question concerns hearing.

This is not one universal scalar score. Evidence can conflict and can mean different things in different source families.

### Relation-first recovery

When explicit identity is unavailable, model candidate successor relations among bounded performance events or voice episodes, then build trajectories from those relations.

```text
performance event A
+ compatible timing
+ compatible pitch/control trajectory
+ compatible patch/sample evidence
+ continuity constraints
        ↓
candidate successor relation
        ↓
persistent-part hypothesis
```

Symbolic sources can supervise what stable part identity looks like downstream. Execution sources can reveal where symbolic transcriptions flattened meaningful articulation, timbre, allocation, or release behavior. Neither representation becomes canonical truth.

## Programmed expression is musical evidence

Exact control is not implementation residue merely because it lives below notation.

```text
exact programmed control
!= derived musical gesture
!= higher expressive interpretation
```

Pitch envelopes, gate behavior, vibrato, detune, duty-cycle changes, FM operator changes, sample retriggers, rhythmic echo, modulation, and dynamic trajectories can be exact execution facts that support musical interpretations.

Calling an exact pitch envelope a `scooped attack` is an interpretation supported by that control history. The interpretation does not replace the history.

## Time is plural

Authored, score, driver, device, sample, acoustic, and perceptual time are related but not interchangeable.

Mappings may be piecewise because of tempo changes, loops, scheduler behavior, expressive timing, resampling, latency, capture gaps, or alignment uncertainty.

Time-domain mappings are evidence objects. Do not hide them in convenience conversions when a later inference depends on the distinction.

## Musical understanding is a dependency graph

Higher analysis can summarize lower evidence, but it may not silently repair or skip uncertainty.

A useful orientation is:

```text
performance evidence
        ↓
pitch / rhythm / part relations
        ↓
harmony / tonality / voice leading
        ↓
cadence / phrase / motivic relations
        ↓
form / hierarchy / arrangement
        ↓
whole-work / soundtrack relations
        ↓
creator grammar / attribution / discourse
```

The real topology is a graph. Authored source may bypass inverse pitch recovery. Documentary evidence may constrain attribution directly. Auditory grouping may affect harmonic interpretation when note ownership is ambiguous.

Therefore:

```text
analysis order != truth order != time order
```

## Harmonic dependencies

Do not collapse `sounding pitches` into `chord`.

```text
performed pitch activity
        ↓
harmonic segmentation
        ↓
chord-tone / figuration hypotheses
        ↓
root / quality / inversion candidates
        ↓
local tonal-center / key candidates
        ↓
harmonic function
        ↓
progression / harmonic rhythm
        ↓
preparation / prolongation / resolution relations
```

Executable music contains passing and neighbor tones, arpeggiation, suspensions, anticipations, pedals, staggered attacks, held tones, portamento, echo, ornamentation, and allocation artifacts.

```text
pitch active at t != chord member at t
pitch-class set != chord spelling != root != inversion != function
local tonal center != global key
chord list != progression
```

Several analyses can coexist when the evidence underdetermines theory.

## Phrase, cadence, and form

A boundary is not yet a phrase role. A sonority is not yet a cadence. A section boundary is not yet form.

Cadence classification must combine local morphology with independent grouping/formal evidence and longer-range syntax. Never allow a cadence label to provide the phrase completion used to prove that same cadence.

For ambiguous arrivals such as Ionian `V → VI`, preserve simultaneous candidate interpretations when supported:

```text
local closure evidence at VI
+
continuation through VI
+
later grounded V → I
→ local close + larger-scale continuation can coexist
```

Final class establishment belongs downstream of phrase-role evidence, not chord morphology alone.

Form similarly depends on recurrence, contrast, transformation, phrase function, tonal/harmonic planning, orchestration, texture, and temporal hierarchy rather than a flat list of detected sections.

## Timbre, synthesis, and instrument identity

```text
synthesis-object identity
!= acoustic descriptor
!= perceptual instrument-family label
!= historical acoustic-instrument identity
```

An authored label such as `strings` may be exact at the authored layer. A classifier output such as `violin_family` is a model/perceptual hypothesis. A higher-quality reconstruction is an acoustic-realization candidate, not recovered historical source truth.

Timbre can also carry structural musical information. Patch/sample choice, envelope, register, articulation, modulation, and routing may participate in phrase, orchestration, role, and form.

## Auditory interpretation and listener response

Auditory organization is distinct from emotional or evaluative response.

Candidate auditory claims include stream grouping, fusion/segregation, continuity, foreground/background, masking, salience, perceived beat/meter, and spatial organization.

Listener response includes expectation, surprise, familiarity, memory, emotion, pleasure, groove, attention, and aesthetic judgment under an explicit listener/model context.

The same source and render can support different listener-response hypotheses under different cultural, learning, memory, or task contexts.

Human-facing language is a discourse projection over the evidence, not a new truth layer. See `human-musical-discourse.md`.

## Attribution and musicological context

Keep creative roles distinct:

```text
composition
!= arrangement
!= sequence / sound-data programming
!= driver / engine programming
!= patch / sample design
!= final realization
```

All modalities may contribute to attribution, but role provenance determines what the contribution means. A patch, control, or arrangement habit may support composer attribution only when historical evidence shows the composer controlled that layer.

Core law:

```text
artifact identity
!= musical similarity
!= work/version identity
!= authorship
```

Similarity can rank or strengthen hypotheses. It cannot establish historical authorship by itself.

## Shared-model admission rule

A mechanism becomes shared only when materially different source families need the same semantic relation without losing useful native evidence.

```text
shared implementation convenience != shared semantic law
```

A shared xSF envelope does not imply one platform runtime. Yamaha chip-family resemblance does not imply one register ontology. Two analyzers wanting the same helper does not make the helper a musical universal.

Promote abstractions by agreement **and disagreement** across independent sources.

## Rendering and transformation

The accurate/reference path remains the scientific control. Source-native enhancement may relax implementation ceilings only when it preserves the adopted musical/instrument identity and keeps the intervention explicit and reversible.

A backend may intentionally lose information, but the loss must be declared.

```text
source A → semantic model → backend B
                     ↓
                re-analyze B
                     ↓
                semantic model'
```

Semantic round trips are verification surfaces. Byte identity is not required unless the contract says so; preservation obligations must be named.

See `source-native-enhanced-rendering.md` for the enhancement contract and `omniphony-realtime-spatial-path.md` for the spatial handoff.

## Cross-project boundary

VGM Compiler owns executable game-music source semantics, source-native execution/reconstruction, musical analysis, rendering, and playback bridges.

libaural may contribute general auditory evidence. Omniphony may consume source-aware spatial objects. Helix may own broader research continuity, library identity, and cross-project historical evidence.

Evidence can cross boundaries without copying another project's ontology or database into this repository.

## Architectural test

For every new abstraction ask:

1. What exact problem does it solve?
2. Which source families independently require it?
3. What native distinctions would it erase?
4. What evidence state and provenance does it carry?
5. What happens when the required evidence is unavailable?
6. Can a higher claim descend back to its support?
7. Is this a durable semantic contract, or merely an implementation convenience?

If the answer is unclear, keep the mechanism source-local or research-local until a discriminating test earns promotion.
