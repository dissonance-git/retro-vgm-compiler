# libvgm dependency patches

This directory contains two kinds of integration material:

- numbered observational patches used by the current VGM execution/analysis path;
- guarded source-render patches used by the foobar source-isolation / Enhanced path.

The source-render sequence is:

```text
python patches/libvgm/apply_source_capture.py <libvgm-checkout-root>
```

It applies:

```text
apply_source_resampler_hooks.py
apply_sn76496_source_tap.py
apply_playera_gain_view.py
```

The scripts use exact singular source replacements. They must fail when the pinned upstream implementation drifts rather than guess a new audio-hot-loop insertion point.

## Why these hooks exist

Enhanced playback must not work by layering a second synthesizer on top of the protected libvgm mix.

The safe replacement algebra is:

```text
protected reference mix
+ enhanced exact source contribution
- exact historical contribution of that same source
= source-replaced enhanced mix
```

`components/vgm/enhancement/genesis_enhanced_recomposition.h` implements that fail-closed operation. Unsupported chips and unknown residuals remain in the protected reference mix.

The VGMPlayer hooks expose the exact resampler boundaries needed to place a source sidecar in the same output-rate domain as the reference. The SN76496 tap exposes the four exact MAME-core source contributions and its reference sum. The PlayerA view exposes the exact later song/fade gain so source and reference arithmetic can be aligned before replacement.

## YM2612 / OPN2

The mature historical quarry also used a sidecar over the exact Nuked OPN2 24-cycle output bus to recover six FM channels plus DAC before their ordinary stereo sum. That mechanism is being consolidated separately because it is tied tightly to the pinned `ym3438_int.h` implementation contract.

Source isolation is not itself higher-quality synthesis. It is the accounting boundary that lets an independently validated enhanced FM/PSG/DAC renderer become audible without doubling the source or replacing unrelated devices.

## UI independence

`patches/foo_input_vgm/apply_enhanced_ui.py` creates an independent `Enhanced` preference. It is intentionally orthogonal to `Spatial`, the VGM output resampler, and the chip sample-rate preference.

```text
Enhanced OFF + Spatial OFF -> protected reference stereo
Enhanced OFF + Spatial ON  -> protected source-aware Omniphony presentation
Enhanced ON  + Spatial OFF -> source-replaced enhanced stereo
Enhanced ON  + Spatial ON  -> source-replaced enhanced Omniphony presentation
```

Until an exact source contribution and a validated enhanced replacement are both available for a device, `Enhanced` must leave that device on the protected reference path.
