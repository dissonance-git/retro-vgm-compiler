# YM2151 / OPM source-family framework

Status: implementation note and evidence boundary.

## Why YM2151 is the first non-Genesis client

Genesis established four reusable laws that should not remain hidden inside one
console implementation:

1. protected reference stereo remains authoritative;
2. an enhanced realization may replace only an exact isolated source witness;
3. selected-source identity must survive render-ahead and arrive on the host
   delivery clock before presentation;
4. Spatial consumes already-selected causal sources and authored route evidence,
   never decides source quality.

YM2151 is a strong falsifier for that architecture because it is an eight-channel
four-operator FM device with different pitch/register geometry from YM2612 and a
shared FM-only family rather than the Genesis FM/DAC/PSG split.

## Research constraint

The abstraction treats one complete YM2151 channel as the source object. It does
not expose individual operators as independent mix stems.

This is grounded in FM/PM literature rather than convenience. Lazzarini and
Timoney's higher-order FM work explicitly treats operator topology and feedback
as defining parts of the synthesis system and discusses the discrete-time
implementation consequences:

- Victor Lazzarini and Joseph Timoney, **Theory and practice of higher-order
  frequency modulation synthesis**, *Journal of New Music Research* (2024),
  DOI `10.1080/09298215.2024.2312236`.

Holm's earlier implementation survey is also directly relevant: different
nominally "FM" implementations can be semantically incompatible, so patch/state
migration cannot assume that one renderer's parameter interpretation is another
renderer's:

- Frode Holm, **Understanding FM Implementations: A Call for Common Standards**,
  *Computer Music Journal* (1992), DOI `10.2307/3680493`.

The practical rule used here is therefore:

> improve the numerical realization only after preserving the authored operator
> network, feedback, pitch relationships, envelopes, modulation controls and
> their time evolution.

The register-derived `ym2151_authority_state` records the authored controls but
is deliberately not called emulator state. Phase accumulators, envelope counters,
feedback memory, timer/LFO phase and the noise generator remain renderer-owned
hidden state that must evolve causally.

## Current implementation

The shared VGM layers are now chip-neutral:

- `source_family_recomposition.h`
- `selected_source_transport.h`
- `spatial_route_transport.h`
- `spatial_source_bus.h`
- `timed_spatial_source_bus.h`
- `vgm_realtime_musical_omniphony_pipeline.h`

Genesis retains compatibility wrappers over those mechanisms. YM2151 is the
first second client via:

- `ym2151_enhanced_recomposition.h`
- `ym2151_selected_source_transport.h`
- `ym2151_spatial_source.h`
- `ym2151_spatial_route_transport.h`
- `ym2151_realtime_musical_omniphony_pipeline.h`
- `ym2151_authority_state.h`

## Exact reference capture

The canonical libvgm patch chain now applies
`patches/libvgm/apply_ym2151_source_tap.py` to the pinned MAME YM2151 core.
The tap is placed after the complete `chan_calc` / `chan7_calc` synthesis and
before ordinary stereo summation. It reports eight authored-pan channel
contributions plus the exact reference mix. This preserves algorithm/operator
coupling, feedback, mute state and channel-8 noise behavior as reference truth.

The private foobar host then uses `apply_ym2151_reference_capture.py` to:

- force the primary YM2151 reference path to MAME + linear resampling;
- capture the eight native lanes without advancing a shadow chip;
- mirror them through the exact same libvgm linear-resampler state as the
  protected mix;
- require exact native accounting and bounded host-rate reconstruction;
- expose a separate eight-lane OPM source plane without changing Genesis
  topology validity.

The startup pre-generation promotion is a separate guarded patch because linear
upsampling may produce one native sample before the first ordinary segment.

## Production register-write timing

A renderer comparison must not create a timing difference merely by choosing the
most convenient API exposed by each core.

In the pinned libvgm player, VGM command `0x54` is decoded by `Cmd_Reg8_Data8`.
That handler calls `SendYMCommand`, which uses the device's ordinary
`RWF_REGISTER | RWF_WRITE` function twice:

```text
write(port 0, register)
write(port 1, value)
```

MAME also exposes a quick direct-register helper, but that helper is not the
ordinary VGM `0x54` playback route. Nuked-OPM's ordinary writer is buffered.
Consequently all MAME/Nuked pair experiments in this project use the production
port-write contract above rather than mixing immediate MAME writes with buffered
Nuked writes.

## Why Nuked-OPM is not yet called an enhanced source renderer

The pinned libvgm tree already vendors Nuked-OPM, a cycle-oriented YM2151/YM2164
implementation derived from hardware research. That makes it a serious candidate
renderer, but not automatically an admissible eight-stem renderer.

Its implementation contains a shared serial mixer/DAC path and clamp behavior.
Consequently, eight independently muted/re-rendered outputs cannot simply be
assumed to sum to the jointly rendered output. Doing so would create convenient
"stems" whose causal relation to the actual nonlinear device output had not been
proved.

`tests/integration/libvgm-source/nukedopm_channel_additivity_falsifier.cpp`
therefore attacks the exact hypothesis:

```text
full_nuked_output == sum(single_channel_nuked_outputs)
```

The fixture keeps nine Nuked-OPM instances synchronized: one complete chip and
eight replicas with one unmuted channel each. It deliberately drives a hot
algorithm-7 patch. The executable succeeds only when it finds a concrete sample
that violates additivity. If the fixture happens to sum exactly, the test fails
and the witness must be strengthened; finite equality would still not establish
universal separability.

Current policy:

- MAME eight-channel tap = exact reference witnesses;
- Nuked-OPM = candidate higher-fidelity whole-chip realization;
- `ym2151_authority_state` = control/state contract a candidate must follow;
- family-local enhanced substitution stays disabled until a lawful causal
  per-channel candidate decomposition or equivalent exact admission proof exists.

This is an intentional incompleteness, not a missing fallback. Reference audio
continues unchanged when enhanced evidence is absent.

## Whole-chip renderer diagnostics

`tests/integration/libvgm-source/ym2151_renderer_pair_probe.cpp` compares MAME
and Nuked-OPM as whole-chip realizations under the production port-write timing
contract. It is diagnostic, not an admission gate.

The current deterministic fixture shelf covers:

```text
algorithm7_attack
feedback_chain
lfo_modulation
channel8_noise
```

For transient, late-steady and whole-render windows it reports RMS scale,
least-squares gain, gain-matched normalized RMS error, zero-lag correlation and
best correlation/lag within a bounded native-sample window. It also records the
first audible frame and requires deterministic replay after reset.

These metrics are deliberately plural and incomplete. A SciSpace literature pass
found several reasons not to collapse renderer fidelity into one waveform or
spectral loss:

- Horner, Beauchamp and So's musical-instrument discrimination work found that
  spectral-difference measures can track human discrimination, with attack/decay
  emphasis sometimes outperforming all-frame averages;
- perceptual audio-quality work by Beerends evaluates differences after an
  auditory-model transformation rather than treating raw signal distance as
  perceptual distance;
- recent differentiable-synthesis work, including Perceptual-Neural-Physical
  sound matching, explicitly notes that generic spectral loss can be a poor
  predictor of pitch relationships;
- masking-aware synthesis-matching work likewise shows that a lower ordinary
  spectral error need not imply a better perceptual match.

Therefore the evaluation ladder is:

```text
exact production timing + authority-state preservation
→ deterministic whole-chip diagnostics
→ transient / steady separation
→ pitch-sensitive + multiresolution spectral measurements
→ psychoacoustic full-reference measurements
→ controlled listening
→ external hardware / measured reference when available
→ only then consider a quality claim
```

MAME remains the protected product reference throughout. A hardware-fidelity
experiment can show that another renderer is physically closer without granting
it permission to replace source families unless the source-accounting obligation
is also independently satisfied.
