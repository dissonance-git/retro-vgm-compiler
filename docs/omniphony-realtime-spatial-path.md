# Omniphony realtime spatial path

## Scope

This document owns the **handoff from VGM Compiler's causal source/musical state into Omniphony presentation**. Internal realtime causality, identity memory, observation, and control-state law are canonical in [`realtime-spatial-dsp.md`](realtime-spatial-dsp.md) and are not restated here.

VGM Compiler supplies source-native game-music objects before historical stereo collapse. Omniphony owns modern spatial realization.

> Recover the real musical sources, then present them coherently in a larger immersive format without rewriting historical source truth.

## Ownership boundary

```text
VGM Compiler
  source truth and source-quality admission
  source / generation identity
  persistent-part evidence
  authored route / send / timing evidence
  source-native shared effects
  past-only musical presentation evidence
  adaptive scene-budget state
        ↓
Omniphony
  NativeRouting / FullSphere policy
  canonical spatial world
  DERIVED geometry and source extent
  shell / binaural rendering
  distance / air / optional room
```

The compiler must not pre-render a second spatial world. Omniphony must not decide which emulator, reconstruction, or source witness is more truthful.

## Source authority

Keep these states distinct:

```text
AUTHORED  preserved from source / device / driver / format
DERIVED   musical / perceptual / presentation inference
EMPTY     no authored fact exists
```

Derived geometry never becomes authored merely because it is stable or pleasing.

Physical chip channel, persistent musical part, speaker channel, canonical Omniphony lane, and shell direction are distinct identities. Do not manufacture presentation lanes to imitate renderer topology.

For ordinary FM synthesis, one complete FM channel is the default spatial source object. Operators remain synthesis internals unless independent object identity is proven. Higher-fidelity whole-chip rendering also does not prove additive stems.

## Presentation modes

`NativeRouting` preserves source-aware routing while closing modern added rear, height, depth, and extent. `FullSphere` opens stable derived azimuth, depth, elevation, distance, and extent while keeping authored route/position evidence as constraints.

Both modes should share the same physical renderer so runtime A/B compares presentation policy rather than different binaural algorithms. Protected historical/reference stereo remains available as the control.

## Shared wet fields

When SPC/S-DSP capture proves dry voices, signed route, echo-send state, and final post-EVOL echo L/R, preserve the final echo pair as one linked historical stereo feedback field. Do not fabricate per-voice wet stems.

Historical shared wet and Omniphony's optional externalization room are separate layers.

## Adaptive presentation

Scene adaptation consumes only past-only state prepared under the causal contract. Useful pressures may include source density, energy distribution, low-band weight, transient density, historical wet share, and coarse dry-source spectral overlap.

Coarse spectral overlap is a bounded engineering statistic, not a claim of psychoacoustic masking. Its current policy is conservative: increased crowding may tighten dry extent/diffuseness and added ambience, but must not be smuggled into unrelated controls.

Tuning constants and positions remain engineering hypotheses until corpus and physical-listening validation supports them.

## Transport boundary

The Omniphony source transport must remain allocation-free on the realtime path and preserve ordered timed source evidence, authored route, strong persistent-part identity where earned, authored 3-D only when supplied, and explicit missing PCM. ABI/version requirements belong to the transport implementation and its binary-layout tests rather than duplicated prose here.

A new track, seek, or decoder restart clears the causal compiler presentation timeline and the corresponding Omniphony presentation history so one soundtrack cannot inherit another soundtrack's mix personality.

## Validation obligations

Defend these classes independently:

```text
SOURCE TRUTH        authored route / timing / identity survive
AUTHORITY           DERIVED presentation never becomes authored
MODE CONTROL        NativeRouting closes creative geometry; FullSphere opens it
EXTENT              size changes spread without moving source centre
SHARED WET          historical stereo wet remains one linked field
ADAPTATION          scene policy is past-only and resettable
SOURCE BOUNDARY     synthesis internals do not become invented objects
REFERENCE           protected historical playback remains available
```

Keep code/test success, source correctness, partition invariance, perceptual-mechanism validity, and physical listening quality as separate evidence states.
