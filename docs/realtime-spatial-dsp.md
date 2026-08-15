# Realtime musical spatial DSP

## Purpose

Game Music Interpreter must be able to feed Omniphony while music is playing.

The spatial path is therefore a streaming DSP path, not a soundtrack-rendering pipeline and not a cache of precomputed positions. Offline analysis may be used to develop, test, or validate models, but normal playback must not depend on whole-track reverse compilation or a previously analyzed soundtrack.

The current runtime shape is:

```text
source-specific execution / isolated source evidence
        ↓
neutral spatial source block
        ↓
prepare current block from past-only musical state
        ↓
Omniphony / spatial presentation of current audio
        ↓
complete current block
        ↓
online acoustic observation
        ↓
weak musical-role hypotheses
        ↓
identity-aware musical-role memory
        ↓
state available to the next block
```

The project boundary remains important: Game Music Interpreter should understand the musical object and expose trustworthy source/evidence/role state. Omniphony owns the general headphone spatial realization.

## Realtime does not mean memoryless

The DSP may preserve bounded state learned from audio that has already happened, including:

- current source identity and generation;
- persistent musical-part identity when evidence supports it;
- recent foreground/foundation/environment hypotheses;
- hypothesis confidence and cue provenance;
- short rolling acoustic statistics;
- role-state smoothing and expiry;
- spatial-control smoothing state.

This is causal memory. It is not a hidden timeline of future instructions.

## Implemented layers

### Neutral source bus

`model/spatial_source.h` is the format-neutral source boundary. A physical voice/channel/lane is not automatically a persistent part, auditory stream, musical role, or authored 3-D position.

`spatial_source_block_view` carries:

- isolated mono source lanes when available;
- source identity and generation;
- exact source/device stereo-route evidence when available;
- effect-send evidence;
- persistent-part evidence when available;
- higher presentation evidence without relabeling it as authored geometry;
- exact within-block evidence events.

### Online acoustic observer

`model/realtime_musical_spatial_observer.h` measures bounded, causal source/scene facts from the current source block.

Current source observations include:

- observed-frame availability;
- RMS and peak;
- activity;
- energy share relative to the observed ensemble;
- low-band energy ratio;
- edge/transient proxy;
- source-identity age.

Current scene observations include:

- observed/active lane counts;
- mean activity;
- energy concentration across lanes;
- low-band energy ratio;
- edge proxy;
- shared-effect-return energy share.

These are observations, not musical verdicts. `low_band_energy_ratio` does not mean `bass`, and `relative_energy` does not mean `lead`.

Unknown samples are excluded rather than treated as zero. An unavailable interval also breaks the observer's local filter continuity rather than advancing an analysis filter through imaginary silence.

The block-summary observer refuses to silently merge two source identities if a physical lane is reused inside the block. The caller must split that acoustic observation at the identity boundary instead.

### Conservative role hypotheses

`model/realtime_musical_role_hypothesis.h` converts observations into explicitly weak hypotheses:

```text
foundation
foreground
transient_accent
environmental_layer
```

Each role carries separate `score`, `confidence`, and cue-provenance bits.

Raw acoustic inference is intentionally confidence-capped. In particular:

- low-frequency body may nominate a foundation candidate but cannot by itself prove a bass part;
- high energy/salience may nominate foreground but cannot by itself prove melodic leadership;
- edge density may nominate a transient accent but cannot establish rhythmic function;
- an explicitly exposed shared wet return is strong evidence of an environmental layer, but still says nothing about an authored 3-D position.

Stronger musical/presentation evidence already attached to the source is admitted as a prior. Weak acoustic evidence cannot silently outrank it.

### Identity-aware role memory

`model/realtime_musical_role_tracker.h` carries musical-role state across blocks without equating hardware slots with musical identity.

The continuity key is:

1. a persistent-part ID when that evidence clears the configured confidence threshold; otherwise
2. bounded `source_id + generation`.

No physical lane number is used as the fallback musical identity.

Musical time advances once per audio block, no matter whether the block contains one source or nineteen. Multiple sources observed in one QSound block therefore cannot make role memory age nineteen times too fast.

The tracker is fixed-capacity and allocation-free. Stale state expires. Capacity eviction deterministically reinitializes the selected slot rather than blending unrelated musical identities.

### Past-only realtime handoff

`model/realtime_musical_spatial_frontend.h` makes the anti-precache rule structural through two explicit phases.

#### `prepare_block()`

Called before the current audio block is spatially rendered.

It may inspect:

- current source identity/evidence at the block boundary;
- exact timed source-evidence events already known from the source representation;
- role memory learned from completed earlier audio.

It does **not** inspect current-block PCM.

The returned handoff is a bounded sidecar containing past-only role state for each lane plus timed role-memory lookups at source-evidence event boundaries. Audio ownership remains in `spatial_source_block_view`.

#### `complete_block()`

Called only after that audio block has passed the renderer.

It may then:

- analyze the completed source PCM;
- build weak role hypotheses;
- update identity-aware role memory for later blocks.

This gives an executable causality rule:

```text
same completed history before block N
        ↓
same prepared musical state for block N
```

Changing the PCM inside block N cannot change the state that was prepared for that same block. Its effect begins only after completion, in later blocks.

The control latency is therefore bounded by the chosen analysis block size. A host may use smaller sub-blocks when lower semantic reaction latency is worth the additional control traffic.

## Spatial control smoothing

`model/realtime_spatial_scene_dsp.h` remains the bounded smoothing/control layer for presentation evidence.

It emits exact frame-bounded `realtime_spatial_control_span` records carrying:

- lane and frame range;
- active source evidence;
- smoothed start values;
- current target values;
- per-sample exponential recurrence coefficients.

The current target vocabulary is:

```text
foundation
foreground
diffuse
width
vertical_affinity
confidence
```

These are musical/perceptual presentation tendencies, not authored coordinates.

No per-sample coordinate cache is needed. Omniphony can execute the supplied recurrence in its audio callback.

## Causality law

There are two complementary causal guarantees.

### Within one control block

For any two executions with the same evidence prefix through frame N:

```text
same source/evidence prefix
        ↓
same spatial-control prefix
```

A later event may change a target beginning at its exact frame boundary, but may not reshape an earlier span.

### Across analysis blocks

For any two executions with the same completed history before block N:

```text
same completed past
        ↓
same role-memory handoff for block N
```

The current block is learned only after it has played.

Together these rules distinguish an adaptive DSP from precached soundtrack direction.

## Identity law

A physical voice/channel/lane is transport, not musical identity.

When a lane changes to a new bounded source identity, spatial smoothing and block-level acoustic observation must not donate the old source's state to the replacement.

A stronger persistent-part relationship may deliberately carry role memory across source episodes, but only when the evidence explicitly supports that continuity. Slot reuse alone never earns it.

## Failure law

Different state layers fail according to what actually happened.

The spatial scene controller fails closed on malformed evidence and does not advance its control state.

The acoustic observer also fails closed rather than inventing a repaired timeline.

The two-phase frontend has one deliberate distinction: if `complete_block()` is reached with a valid audio clock but analysis becomes unusable, **stream time still advances** because the audio has already happened. The block loses learning instead of freezing musical time. Invalid sample-rate input cannot advance the clock.

This prevents a bad semantic sidecar from steering valid audio while also preventing analysis failure from making role memory believe time stopped.

## Current frontier

We now have the causal nervous system from source audio to past-only musical-role memory. The major missing information is deeper music rather than more plumbing.

The next online evidence should add, without collapsing evidence layers:

- source-native pitch/register trajectories where a format genuinely exposes them;
- performed/auditory pitch estimates separately from device nominal frequency;
- onset timing and cross-source rhythmic relationships;
- note/phrase continuity;
- call-and-response and countermelodic relationships;
- texture opening/contraction;
- recurring part vocabulary;
- phrase and section state;
- eventually soundtrack-level vocabulary learned during playback.

That richer layer should answer questions such as:

```text
what is functioning as the foundation right now?
what is actually leading rather than merely loud?
which sources move as one perceptual layer?
is this transient metrically structural or ornamental?
is this wet return environmental glue or a rhythmic event?
is the texture opening, contracting, or redistributing foreground?
is an answering line asking for separation from the lead?
is a recurring part preserving a spatial identity?
```

Only then should renderer policy become progressively more ambitious.

## Evaluation

Engineering validation and listening validation remain separate.

Code tests should protect:

- allocation-free bounded operation;
- past-only prepare/complete causality;
- exact event boundaries;
- time advancing once per block rather than once per lane;
- cross-block role-state carry;
- source-generation boundaries;
- evidence-backed persistent-part continuity;
- fixed-capacity eviction without identity contamination;
- malformed-timeline failure behavior;
- no promotion of inferred tendencies into authored geometry.

Listening tests should ask whether deeper musical state improves the intended result:

- larger and more externalized without flattening the scene;
- stronger separation without breaking ensemble coherence;
- stable bass and rhythmic foundation;
- spatial contrast that follows musical form rather than random motion;
- ambience that creates depth without washing out source identity;
- transformations that feel musically motivated rather than algorithmically busy.

The final arbiter is not how elaborate the state graph becomes. It is whether deeper realtime musical understanding makes Omniphony produce a better spatial performance of the same music.
