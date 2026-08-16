# Persistent musical identity

## Purpose

This document records the current Game Music Interpreter frontier for recovering persistent musical voices/parts from executable game-music evidence.

The problem exists because implementation resources are not musical identities:

```text
authored / logical part
        ↓ may be allocated through
running synthesis voice
        ↓ occupies
physical hardware slot
```

The mapping may move, split, fuse, restart, or be temporarily unobservable. Therefore:

```text
physical slot
!= physical voice episode
!= persistent musical part
!= auditory stream
```

This distinction is supported independently by game-driver behavior, current VGM/SPC adapters, OpenMusic's voice-separation libraries, and symbolic voice-separation literature.

Persistent-part recovery is one of the central bridges between low-level execution and composer-level understanding. Without it, a system may know exactly what every chip voice did while still failing to recognize melody, bass, countermelody, accompaniment, voice leading, phrase continuity, or thematic transformation.

## Current evidence hierarchy

Persistent identity should use the strongest source-supported evidence available. A lower-level heuristic must not override a stronger upper-layer identity merely because the heuristic is easier to compute.

A practical ordering is:

1. **explicit authored part identity** when the source language/score proves it;
2. **validated driver/logical-track identity** when driver grammar and execution prove it;
3. **persistent synthesis/instrument/sample identity** when exact and semantically relevant;
4. **control continuity** such as pitch/modulation/envelope trajectories;
5. **temporal and contour continuity** across recovered performance events;
6. **pitch/register proximity and overlap constraints**;
7. **perceptual stream/grouping evidence** when the question is how a listener organizes the result.

This is not a universal scalar weighting. Evidence can conflict, be unavailable, or have different semantics in different source families. The graph should preserve the alternatives and their provenance rather than reducing all identity evidence to one opaque score.

## Part identity versus auditory-stream identity

Some symbolic voice-separation literature defines a musical voice in perceptual terms, close to an auditory stream. That is useful research but is not identical to Game Music Interpreter's `part` object.

Game Music Interpreter already has separate semantic layers and node kinds for:

- musical-performance `part`;
- synthesis `voice_instance`;
- `physical_slot`;
- `auditory_stream`.

A listener may fuse several authored/physical sources into one stream or split one source into several perceptual components. Therefore a voice-separation algorithm can be:

- a candidate **part** inference when applied to source-supported performance events;
- a candidate **auditory stream** inference when applied as a perceptual model;
- neither when stronger source identity contradicts its grouping.

The analysis must say which question it is answering.

## Symbolic supervision and execution inference

Persistent identity is a natural place for source families to teach one another.

When a symbolic or sequence representation exposes an explicit logical track, it can provide a strong supervisory example of how one musical part manifests downstream:

```text
known logical track
        ↓
notes / rests / articulation / instrument changes
        ↓
driver allocation
        ↓
physical voice episodes and device events
```

That paired evidence can teach the common model which execution patterns tend to preserve musical identity through channel reassignment, note changes, patch reuse, sample reuse, rests, retriggers, or control changes.

When only VGM/SPC or another execution-side source survives, the inverse analysis can then ask whether similar evidence supports a persistent-part hypothesis:

```text
physical episode A
+ compatible timing
+ compatible pitch trajectory
+ compatible patch/sample identity
+ articulation/control continuity
        ↓
possible persistent musical part
        ↓
physical episode B
```

This is not a claim that the hidden historical source was MIDI, SMPS, SSEQ, MML, or any other known format. It is a transfer of musical-identity knowledge across representations.

The reverse direction matters too. Execution evidence can reveal that a symbolic transcription or export has flattened meaningful distinctions, for example:

- one apparent symbolic instrument corresponds to several materially different FM patches or sample articulations;
- one logical track is physically interrupted by voice stealing without losing musical identity;
- one note-like symbolic event realizes as a longer envelope/release gesture;
- a sample or patch change carries articulation or orchestration meaning not represented by note pitch alone.

Therefore:

> Symbolic data can supervise execution-only inference, and execution evidence can correct symbolic simplification. Neither becomes the canonical truth.

See `docs/composer-level-understanding.md`.

## GitHub and literature pressure tests

### OpenMusic Streamsep

`openmusic-project/Streamsep` performs symbolic stream/voice separation with single-link agglomerative clustering. Its implementation uses temporal distance, perceptually scaled pitch distance, non-overlap constraints, tunable weights, and segmentation. It can follow lines that cross registers.

Useful lesson:

> Persistent grouping is a separate analysis over events, not a property of the hardware channel that produced them.

Its cubic implementation and score-centric feature set make it a research/reference algorithm rather than a realtime Game Music Interpreter dependency.

### Voice separation as link prediction

Karystinaios, Foscarin, and Widmer model symbolic voice separation as a multi-trajectory tracking problem: notes are graph nodes and a predicted link means two notes are consecutive in the same voice. Their formulation handles inversions and overlaps and constrains trajectories without requiring one fixed register ordering.

The accompanying `manoskary/vocsep_ijcai2023` implementation uses Partitura note structures and a family of note, pitch, metrical, density, harmonic, and voice-leading descriptors.

Useful lesson for Game Music Interpreter:

```text
performance event
        ↓ candidate successor relation
performance event
        ↓
trajectory / persistent-part hypothesis
```

The important transfer is the **relation-first** formulation. Game Music Interpreter does not need to import the graph neural network or its training stack to benefit from that decomposition.

### Other symbolic voice-separation work

The literature also includes:

- same-voice predicates learned from symbolic features;
- greedy note-to-voice assignment with perceptually informed context;
- contig/fragment methods that connect locally stable voice fragments;
- information-theoretic voice assignment;
- note-to-note affinity graphs followed by clustering;
- methods that explicitly allow the number of active streams to change over time.

These approaches reinforce three durable constraints:

1. local pitch proximity is insufficient by itself;
2. longer temporal/contextual fragments can be stronger than one adjacent-note comparison;
3. competing assignments are normal and should not be erased prematurely.

## Why Game Music Interpreter can do better than score-only separation

Generic symbolic systems usually begin with note-like events because that is the highest layer they possess. Game Music Interpreter can often retain deeper causal evidence.

Examples:

- a validated GEMS/SMPS/N-SPC track may prove logical identity across hardware allocation changes;
- an exact FM patch or tracker instrument may support continuity across physical slots;
- an SPC event may retain exact runtime source index, BRR address/version, pitch rate, envelope state, and physical voice lifecycle;
- a VGM Genesis event may retain device family, exact register-transition support, pitch code/block, physical episode, and control history;
- authored MML may directly prove a part before compilation and allocation occur.

A source-aware analysis should use these facts before falling back to generic score heuristics.

The long-term advantage comes from combining both directions:

```text
explicit symbolic identity teaches execution mapping
+
execution truth teaches symbolic limitations
        ↓
stronger persistent-part recovery across all formats
```

## Source-relative feature availability

A common identity analysis must not require every source to expose the same features.

### Authored symbolic / MIDI / MML

Potentially available:

- explicit part/track identity;
- exact note/rest/tie structure;
- authored pitch and duration;
- instrument selection;
- articulation/modulation;
- tempo/meter where represented;
- macros/loops/control flow where represented.

When the source semantics are validated, these can be stronger than any inverse voice-separation heuristic.

### Driver / sequence formats

Potentially available:

- persistent logical tracks;
- track-local event order;
- instrument/sample/patch identity;
- modulation state;
- allocation policy into hardware resources;
- loop, call, branch, variable, random, and other control-flow semantics.

Driver identity can remain stable while physical voices are rotated or stolen.

### VGM / Genesis execution

Currently represented by the project:

- exact source command order/timing where captured;
- decoded device transitions;
- bounded physical YM2612/PSG voice episodes;
- device family and physical channel;
- device-native YM pitch code/block or PSG period;
- device-native pitch-control transition history;
- capture gaps and resynchronization boundaries.

Not proved merely by the VGM register trace:

- original driver track;
- authored part identity;
- MIDI note/bend semantics;
- normalized absolute musical pitch independent of device context.

The existing Genesis pitch adapter intentionally stops at device-native pitch control.

### SPC / S-DSP execution

Currently represented by the project:

- exact SPC snapshot state;
- exact/derived runtime DSP events through controlled instrumentation;
- bounded physical S-DSP voice episodes;
- exact runtime source index and BRR address observations;
- exact event-time BRR sample versions when RAM continuity can be proved;
- device-native pitch rate;
- envelope, key-on-delay, noise, release/inactive phase boundaries;
- explicit continuity loss.

Not automatically proved from S-DSP state alone:

- authored note name;
- absolute sample root tuning;
- original driver part;
- persistent part identity;
- perceptual stream identity.

A change in runtime `mSrc` does not automatically begin a new persistent part, and one physical voice episode can outlive a KOFF boundary through release.

### Trackers/modules

Potentially available:

- order/pattern/row structure;
- explicit notes;
- instrument/sample identity;
- channel/effect state;
- source-specific channel semantics.

Tracker channel identity should be interpreted according to the source format rather than universally equated with a persistent musical part.

## Relative pitch without absolute note identity

Voice-separation heuristics often use absolute or normalized note pitch. Executable game-music sources do not always provide that directly.

However, some useful pitch relations can be derived without claiming an absolute authored note:

- YM2612 frequency ratios can be derived from F-number/block relationships within a known device clock context;
- PSG relative pitch movement can be derived from tone-period ratios;
- S-DSP pitch-rate ratios can describe relative playback-rate movement, but only become musical interval evidence when sample/tuning continuity supports that interpretation.

Therefore analysis should distinguish:

```text
device-native pitch state
relative pitch relation
normalized / authored pitch hypothesis
```

rather than demanding one universal pitch number at ingestion.

A later alignment to known symbolic data may strengthen a pitch interpretation, but it must not rewrite the original device-native observation.

## Current executable regression

`tests/model/voice_separation_hypothesis_test.cpp` protects the current representation boundary.

It establishes that:

- several persistent-part hypotheses may coexist for the same recovered event;
- a candidate may survive a register crossing and physical-channel move;
- hardware episodes do not need `same_identity_as` links for a higher part hypothesis to span them;
- grouping features and grouping confidence remain explicit hypothesis evidence;
- lower source/performance/synthesis evidence remains unchanged when hypotheses are added;
- validated driver-track identity may support a stronger persistent-part interpretation than a proximity-only voice-separation candidate;
- the weaker candidate remains inspectable after stronger evidence appears.

This uses the existing `part`, `groups_into`, provenance, and evidence-state vocabulary. No new graph primitive was required.

## Immediate implementation direction

Do **not** implement a universal weighted voice score merely to make all source families look alike.

The project-owned analysis should make feature availability explicit and test source-specific evidence extraction from real adapters. It should be capable of producing candidate successor/grouping evidence even when some common symbolic features are unavailable.

High-value candidate evidence includes:

- exact authored/driver-track identity;
- exact/derived instrument or sample identity;
- physical-episode continuity;
- control-trajectory continuity;
- event time gap and overlap;
- relative pitch movement when valid;
- absolute/normalized pitch when valid;
- timbre/synthesis similarity;
- source allocation change as a neutral fact rather than an identity break;
- capture completeness and provenance quality.

High-value cross-representation controls include paired or aligned cases where symbolic sequence identity and downstream execution are both known. Those cases should be used to calibrate and falsify execution-only grouping rules without treating one format's semantics as universal.

Only after features are extracted from materially different source families should the project choose deterministic, probabilistic, learned, or hybrid grouping policies. Source-specific policies may remain necessary even when they feed a shared persistent-part representation.

That policy should remain analysis-side initially. Normal realtime playback must not depend on whole-song voice separation.

## Related research

See:

- `docs/composer-level-understanding.md`
- `docs/openmusic-libraries.md`
- `docs/music-representation-systems.md`
- `docs/musical-execution-model.md`
- `research/cases/openmusic-libraries.md`
- `components/vgm/enhancement/genesis_performance_adapter.h`
- `components/vgm/enhancement/genesis_pitch_control_adapter.h`
- `components/spc/spc_runtime_voice_adapter.h`
- `components/spc/spc_runtime_sample_adapter.h`
- `tests/model/voice_separation_hypothesis_test.cpp`
