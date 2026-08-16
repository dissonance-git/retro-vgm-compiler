# QSound native-to-consumer time boundary

## Question

How should the 19 native QSound source lanes be placed on a consumer/output timeline without creating nineteen independent clocks or falsely claiming bit-identical reproduction of libvgm's historical resampler state?

## Pinned upstream resampler

The current VGM playback foundation uses the libvgm resampler from the same pinned upstream revision as the QSound core:

```text
ValleyBell/libvgm
61fc6725644886abc3168e240e4e51588d74bdf7

emu/Resampler.h
emu/Resampler.c
player/vgmplayer.cpp
```

`RESMPL_STATE` carries persistent interpolation state including:

```text
smpRateSrc
smpRateDst
resampleMode
smpP
smpLast
smpNext
lSmpl
nSmpl
```

VGMPlayer configures each device resampler from the device option, falling back to `RSMODE_LINEAR`.

For the ordinary QSound case the core native rate is approximately 24 kHz while consumer playback is commonly 44.1 or 48 kHz, so the default linear path is the libvgm linear upsampler.

## The initialization trap

The linear upsampler does more than apply a rate ratio.

During `Resmpl_Init`, libvgm pre-generates one source sample because its upsampler is one source sample behind. That sample is stored as `nSmpl` before ordinary destination blocks are rendered.

The current GMI QSound native-source observer is intentionally scoped to normal decode blocks. It is not attached during player/resampler initialization.

Therefore:

```text
captured native source frames
+
nominal source/destination rate ratio
```

is **not sufficient evidence** to claim exact reconstruction of libvgm's interpolation history from output sample zero.

The same warning applies at arbitrary state transitions and rate changes: persistent `RESMPL_STATE` history is part of historical interpolation behavior.

## Separate two goals

Two different engineering goals must remain separate.

### Historical reference parity

If the claim is:

> reproduce libvgm's exact resampled QSound stereo trajectory

then the relevant `RESMPL_STATE` history, mode and boundary behavior must be part of the ruler. A project-owned rate converter cannot simply declare itself equivalent.

### Source-bus time coherence

If the claim is:

> place all 19 exact native source lanes on one explicit consumer timeline for a modern source-aware renderer

then GMI can define and validate its own time map, provided the map is explicit and all lanes share it.

These are not contradictory. The historical renderer remains the control while the source bus uses a better-specified presentation-time contract.

## Implemented shared time map

`components/vgm/enhancement/qsound_native_time_map.h` defines the first project-owned time ruler.

For output sample index `d`, native rate `Rs` and consumer rate `Rd`:

```text
native position = d * Rs / Rd
```

The implementation returns this coordinate without floating-point phase drift as:

```text
native_floor
fraction_numerator / fraction_denominator
```

Every one of the 19 source lanes must consume the same returned coordinate.

There is no lane-local phase accumulator.

## Why absolute time

The mapping is computed from the absolute output sample index rather than by repeatedly adding a floating-point step.

That gives two useful invariants:

1. splitting a song into different decode-block sizes cannot change the intended consumer-time coordinate;
2. all 19 lanes remain locked to one rational phase even after long playback.

The time map is also independent of whether a particular decode block happened to make libvgm pull zero, one or several new native QSound frames.

## Current rate boundary

The first ruler admits:

```text
consumer rate >= native QSound rate
```

including exact-rate operation.

It rejects downsampling.

That is deliberate. Downsampling needs an explicit anti-alias filter policy; silently applying point sampling or the upsampling interpolation rule would turn an untested rendering choice into infrastructure.

This is not a hardware limitation. It is the current evidence boundary of the consumer converter.

## Missing native brackets are evidence gaps

The time map says which native coordinate a consumer frame needs. It does not manufacture the native samples needed to evaluate that coordinate.

Because the current decode-scoped observer can miss libvgm's initialization pre-sample, early consumer coordinates may request a native bracket that the sidecar does not contain.

The correct response is:

```text
required native bracket absent
→ consumer source frame unavailable
```

not:

```text
repeat zero / repeat first observed sample / guess history
→ claim valid source frame
```

A later observer snapshot or explicit source-history contract may close that startup gap. Until then, availability must remain visible.

## Relation to the QSound environment

The shared time map is for time-varying native audio evidence, not for smearing discrete controls.

The following remain discrete events/state:

- source pan writes;
- PCM echo-contribution writes;
- shared echo feedback and delay controls;
- filter-table selection;
- output-delay controls;
- state-transition controls;
- mix-accounting freshness.

A future source bus may associate those controls with the consumer timeline, but must not interpolate an enum, validity bit or command word as if it were PCM.

The native mix-accounting observer can use the same native/consumer correspondence as a witness, but its wet/dry terms remain historical branch accounting rather than nineteen source wet stems.

## Source-bus target

The current safest consumer target is:

```text
19 coherent mono float source lanes
+ exact source evidence / authored QSound route state
+ discrete shared-environment evidence
+ protected reference mix as control
```

A shared effect-return lane should be added only if a later causal decomposition earns it.

This matches the general source-bus principle already used by SPC: shared fields remain shared objects instead of being copied into every source.

## Tests

`tests/vgm/qsound_native_time_map_test.cpp` currently protects:

- unsupported unconfigured state;
- rejection of unvalidated downsampling;
- 24038 Hz to 48000 Hz rational projection;
- exact one-second source/destination agreement;
- exact rational phase on consecutive consumer samples;
- no phase drift from block-relative accumulation;
- exact-rate zero-fraction operation.

The ruler has also been compiled locally with C++17, `-Wall -Wextra -Wpedantic -Werror` and passed.

The manual `core-tests` workflow owns the test. Hosted workflow execution remains blocked before job start by the repository/account runner billing/spending condition, so no green-CI claim is made.

## Current claim

The earned statement is:

> GMI has an explicit, drift-free rational mapping from the consumer sample timeline to one shared native QSound timeline, suitable as the phase authority for all 19 source lanes when consumer rate is at least the native rate.

The stronger statements remain unearned:

> The project-owned map reproduces libvgm's hidden resampler state exactly.

and:

> A complete 19-lane consumer-rate QSound audio bus is already available.

The next implementation step is a bounded source-window/interpolation layer that consumes this one time map and marks missing native brackets unavailable rather than guessing them.
