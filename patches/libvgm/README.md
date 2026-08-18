# libvgm dependency patches

This directory contains two kinds of integration material:

- numbered observational patches used by the current VGM execution/analysis path;
- guarded source-render patches used by the foobar source-isolation / enhanced path.

The canonical source-aware sequence is:

```text
python patches/libvgm/apply_source_capture.py <libvgm-checkout-root>
```

It applies the numbered realtime/DAC observer patches followed by:

```text
apply_source_resampler_hooks.py
apply_sn76496_source_tap.py
apply_ym2151_source_tap.py
apply_playera_gain_view.py
apply_playera_postrender_hook.py
apply_playera_deferred_postrender.py
```

The scripts use exact singular source replacements. They must fail when the pinned upstream implementation drifts rather than guess a new audio-hot-loop insertion point.

## Why these hooks exist

enhanced playback must not work by layering a second synthesizer on top of the protected libvgm mix.

The safe replacement algebra is:

```text
protected reference mix
+ enhanced exact source contribution
- exact historical contribution of that same source
= source-replaced enhanced mix
```

`components/vgm/enhancement/source_family_recomposition.h` owns the shared fail-closed transaction law. Device clients define their exact source identities and evidence families. Unsupported chips and unknown residuals remain in the protected reference mix.

The VGMPlayer hooks expose the resampler and post-render boundaries needed to place source sidecars in the same output-rate and gain domain as the reference. The SN76496 tap exposes four exact MAME-core source contributions and their reference sum. The YM2151 tap exposes eight complete MAME OPM channel contributions after channel synthesis and authored pan but before ordinary summation. The PlayerA hooks expose later song/fade gain and a deferred replacement boundary so source/reference arithmetic can remain aligned.

## YM2612 / OPN2

The Genesis path uses a sidecar over the exact Nuked OPN2 24-cycle output bus to recover six FM channels plus DAC before ordinary stereo summation. Higher-quality FM realization is admitted only through the protected source-replacement contract; source isolation itself is not a quality claim.

## YM2151 / OPM

YM2151 is the first non-Genesis client of the shared source-family framework. The exact reference source plane currently comes from the default MAME OPM core through `apply_ym2151_source_tap.py` and the guarded foobar host capture patches.

libvgm also contains Nuked-OPM, which is a serious whole-chip fidelity candidate, but its downstream serial mixer/DAC is shared across channels. The repository therefore does not treat eight independently muted Nuked renders as exact enhanced source lanes unless additivity is proved. `tests/integration/libvgm-source/nukedopm_channel_additivity_falsifier.cpp` attacks that assumption directly.

Until a lawful causal per-channel candidate decomposition or equivalent exact admission proof exists, YM2151 family-local enhanced substitution remains disabled and the protected reference audio stays authoritative.

## UI independence

The `enhanced` preference is independent of `Spatial`, the VGM output resampler, and the chip sample-rate preference.

```text
enhanced OFF + Spatial OFF -> protected reference stereo
enhanced OFF + Spatial ON  -> protected source-aware Omniphony presentation
enhanced ON  + Spatial OFF -> source-replaced enhanced stereo where admitted
enhanced ON  + Spatial ON  -> source-replaced enhanced Omniphony presentation where admitted
```

Until an exact source contribution and a validated enhanced replacement are both available for a device, `enhanced` must leave that device on the protected reference path.
