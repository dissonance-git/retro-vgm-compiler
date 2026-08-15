# Realtime musical spatial DSP

## Purpose

Game Music Interpreter must be able to feed Omniphony while music is playing.

The spatial path is therefore a streaming DSP path, not a soundtrack-rendering pipeline and not a cache of precomputed positions. Offline analysis may be used to develop, test, or validate models, but normal playback must not depend on whole-track reverse compilation or a previously analyzed soundtrack.

The runtime shape is:

```text
source-specific execution / isolated source evidence
        ↓
neutral spatial source block
        ↓
online musical + perceptual evidence
        ↓
causal musical-scene state
        ↓
realtime spatial control spans
        ↓
Omniphony presentation policy + binaural renderer
```

The project boundary remains important: Game Music Interpreter should understand the musical object and expose trustworthy source/evidence/control state. Omniphony owns the general headphone spatial realization.

## What may persist

Realtime does not mean memoryless.

The DSP may preserve bounded state such as:

- current source identity and generation;
- persistent musical-part identity when evidence supports it;
- recent foreground/background tendency;
- foundation/bass role tendency;
- diffusion and width tendency;
- signed register/vertical affinity;
- confidence in the current musical interpretation;
- current smoothing state;
- short rolling musical/perceptual statistics required by an online analyzer.

This is causal state learned from the stream so far. It is not a hidden timeline of future instructions.

## Current neutral control contract

`model/realtime_spatial_scene_dsp.h` consumes `spatial_source_block_view` and emits bounded `realtime_spatial_control_span` records.

Each span carries:

- exact lane and frame range;
- the source evidence active over that range;
- smoothed musical/presentation values at the start of the range;
- the current target values;
- the per-sample smoothing coefficients needed to reproduce the same recurrence in Omniphony.

The target vocabulary is intentionally non-geometric:

```text
foundation
foreground
diffuse
width
vertical_affinity
confidence
```

These are musical/perceptual tendencies, not authored coordinates.

A renderer may use them to decide that an ambience can occupy more surrounding field, a stable bass foundation should remain coherent, or a foreground countermelody benefits from separation. The resulting 3-D presentation remains renderer-inferred unless the source representation itself supplied an authored position.

## Causality law

For any two executions with the same observed prefix:

```text
same source/evidence history through frame N
        ↓
same spatial control history through frame N
```

A later event may change the control target beginning at its exact frame boundary. It may not retroactively reshape an earlier span.

This is the key distinction between online interpretation and precached soundtrack direction.

Block assembly may inspect the event list to validate ordering and bounds, but later events do not participate in the state transition of earlier spans.

## Smoothing law

Musical inference is allowed to be uncertain. The audible field should not twitch every time a confidence value moves slightly.

The current control state therefore uses asymmetric exponential smoothing for unit-interval tendencies:

```text
y[n+1] = target + (y[n] - target) * coefficient
```

with a faster rise and slower fall by default. Signed vertical affinity uses one symmetric motion constant because numerical increase/decrease has no stable perceptual meaning there.

The defaults are presentation-policy starting points, not claims about auditory optimality. Listening tests must decide whether they are right.

No per-sample coordinate buffer is required. A control span contains the start value, target, and recurrence coefficient, so Omniphony can execute the same transition directly in its audio callback.

## Identity law

A physical voice/channel/lane is not a musical identity.

When a lane changes to a new bounded source identity, smoothing from the old source is reset at the identity boundary. This prevents an outgoing foreground lead, for example, from donating its accumulated presentation state to an unrelated sound merely because the driver reused the same hardware slot.

Persistent musical-part relationships may later justify continuity across source episodes, but that continuity must be explicit and evidence-backed rather than inferred from slot reuse.

## Failure law

The realtime scene controller fails closed on malformed evidence timelines:

- more lanes/events than the declared fixed capacity;
- missing required pointers;
- invalid sample rate or smoothing configuration;
- event lane outside the current block;
- event frame outside the current block;
- event time moving backward.

A failed block does not advance persistent DSP state.

This is important because a corrupt semantic sidecar must never silently steer an otherwise valid audio path.

## What this does not solve yet

The current controller is the realtime bridge from musical/presentation evidence to a stable Omniphony-ready control trajectory. It does not yet create all of that evidence itself.

The next musical frontier is the online analyzer above the neutral source bus. It should infer useful tendencies causally from combinations of:

- source-local audio and envelopes;
- exact authored/device routing where available;
- persistent source/part identity;
- pitch/register and activity trajectories;
- onset and rhythmic relationships;
- source energy relative to the ensemble;
- spectral and textural role;
- shared-effect/environment evidence;
- short- and medium-timescale repetition/continuity;
- eventually phrase/form state and soundtrack vocabulary learned during playback.

That analyzer should not ask only `where should this voice go?`.

It should ask musical questions such as:

```text
what is acting as the foundation right now?
what has become foreground?
which sources are one perceptual layer?
is this return environmental glue or a rhythmic event?
is the texture opening or contracting?
is an answering line asking for separation from the lead?
is a recurring part preserving a spatial identity?
```

Those answers can then modulate Omniphony while remaining subordinate to source evidence, musical identity, stability, and listening quality.

## Evaluation

Engineering validation and listening validation remain separate.

Code tests should protect:

- allocation-free bounded operation;
- causal prefix invariance;
- exact event boundaries;
- cross-block state carry;
- source-identity reset;
- malformed-timeline atomicity;
- no promotion of inferred tendencies into authored geometry.

Listening tests should ask whether the result improves the intended musical experience:

- larger and more externalized without flattening the scene;
- stronger separation without breaking ensemble coherence;
- stable bass and rhythmic foundation;
- spatial contrast that follows musical form rather than random motion;
- ambience that creates depth without washing out source identity;
- transformations that feel musically motivated rather than algorithmically busy.

The final arbiter for the custom listening path is not the sophistication of the control graph. It is whether deeper musical understanding makes Omniphony produce a better spatial performance of the same music.
