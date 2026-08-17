# SNESAPU dependency patches

These guarded patches consolidate the mature causal-source work that previously lived in `dissonance-git/vgmspc` into Retro VGM Compiler's dependency boundary.

They target the editable SPCPlay / SNESAPU source tree. They are intentionally not part of the dependency-free model build.

Apply in order:

```text
python patches/snesapu/apply_source_capture.py <spcplay-root>
python patches/snesapu/upgrade_source_capture_v2.py <spcplay-root>
```

Both scripts use exact singular source replacements. If the pinned upstream layout changes, patching fails instead of guessing a new hot-loop insertion point.

## SRCE v2 contract

The resulting SNESAPU producer exposes one source/control block beside each protected reference PCM block:

```text
0..7    dry voice audio
8..15   effective per-sample left coefficient for voices 0..7
16..23  effective per-sample right coefficient for voices 0..7
24      final shared wet contribution, left
25      final shared wet contribution, right
```

The dry voice tap is after interpolation/noise selection and envelope processing, before voice-local stereo gain. The wet pair is one shared S-DSP echo field and is captured after the current EVOL arithmetic. It must never be cloned into eight fictional per-voice wet stems.

`components/spc/snesapu_source_transport_v2.h` is the dependency-free consumer-side contract for these 26 planes.

The gain planes are control truth, not extra audio objects. The wet planes already contain their EVOL gain trajectory, so downstream evidence must mark that route arithmetic as preapplied.

## Enhanced reconstruction is a different seam

SRCE v2 observes the reference SNESAPU synthesis path. It does not itself replace interpolation.

The independently switchable `Enhanced` path begins earlier, from decoded BRR neighborhood + source fractional phase, and is modeled by:

```text
components/spc/spc_enhanced_reconstruction.h
components/spc/snes_spc_enhanced_source_hook_bridge.h
```

That separation is deliberate:

```text
reference source capture != enhanced source reconstruction
spatial presentation      != synthesis enhancement
```

The foobar shell may select either synthesis path and independently select reference stereo versus Omniphony presentation.
