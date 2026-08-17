# SNESAPU dependency patches

These guarded patches consolidate the mature causal-source work that previously lived in `dissonance-git/vgmspc` into Retro VGM Compiler's dependency boundary.

They target the editable SPCPlay / SNESAPU source tree. They are intentionally not part of the dependency-free model build.

## Source capture

Apply in order:

```text
python patches/snesapu/apply_source_capture.py <spcplay-root>
python patches/snesapu/upgrade_source_capture_v2.py <spcplay-root>
```

Both scripts use exact singular source replacements. If the pinned upstream layout changes, patching fails instead of guessing a new hot-loop insertion point.

### SRCE v2 contract

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

## Independent Enhanced playback

Apply the foobar preference plus the first audible Enhanced policy with:

```text
python patches/snesapu/apply_enhanced_component.py \
  <foo_snesapu/foobar2000/foo_snesapu>
```

This composes:

```text
apply_enhanced_ui.py
apply_enhanced_runtime.py
```

`Enhanced` and `Spatial` are independent saved controls. Enhanced defaults off, so the protected existing synthesis path remains the default.

### Existing SNESAPU already contains the right first enhancement mechanism

The pinned SPCPlay/SNESAPU renderer has a useful distinction that is easy to miss from the preferences dialog.

Its interpolation choices include `INT_SINC`, documented by the source as an **8-point sinc** interpolator. More importantly, `SetDSPOpt` normally sets the DSP execution rate to the configured output rate and recalculates source pitch for that rate. Only the separate `DSP_ECHOFIR` compatibility/actual-emulation mode forces the DSP back to 32 kHz and then activates the final sampling-rate converter when the requested output rate is higher.

Therefore, with for example:

```text
configured output rate = 96 kHz
Enhanced = on
```

our first SNES Enhanced path selects:

```text
DSP execution / voice reconstruction at 96 kHz
+ 8-point sinc source interpolation
+ the same sequence / BRR / pitch / envelope / routing state
```

rather than:

```text
historical 32 kHz final stereo
-> generic 96 kHz upsampling
```

`apply_enhanced_runtime.py` expresses exactly that bounded intervention. It leaves the user's configured output-rate field intact, forces `INT_SINC` only for the active Enhanced playback path, and explicitly clears `DSP_ECHOFIR` so Enhanced cannot accidentally collapse back to a 32 kHz completed-bus resample.

This also means a user who already selected `96 kHz + Sinc` was manually exercising much of this same source-domain quality path even before the dedicated checkbox existed. The checkbox turns that mechanism into an explicit, reversible synthesis mode rather than an accidental combination of quality settings.

The ordinary interpolation and output-rate preferences remain available for the reference/control path.

## Deeper enhanced reconstruction

The existing high-rate sinc path is only the first rung. Retro VGM Compiler also models an earlier and more explicit source seam:

```text
components/spc/spc_enhanced_reconstruction.h
components/spc/spc_enhanced_native_interval.h
components/spc/spc_sample_restoration.h
components/spc/spc_upstream_sample_reconstruction.h
components/spc/snes_spc_enhanced_source_hook_bridge.h
```

That machinery can evaluate the exact decoded-BRR source trajectory at sub-32-kHz phases, and can substitute a proven higher-quality upstream sample only when lineage, preparation mapping, and same-instrument validation permit it.

This is the route for eventually going beyond the already-good SNESAPU sinc renderer while keeping historical edits, loops, articulation, pitch motion, envelopes, and sample identity under evidence control.

## Separation of concerns

```text
reference source capture != enhanced source reconstruction
spatial presentation      != synthesis enhancement
```

The foobar shell may independently select synthesis quality and Omniphony presentation:

```text
Enhanced OFF + Spatial OFF -> protected reference stereo
Enhanced OFF + Spatial ON  -> protected synthesis + Omniphony
Enhanced ON  + Spatial OFF -> enhanced source-native stereo
Enhanced ON  + Spatial ON  -> enhanced source-native audio + Omniphony
```
