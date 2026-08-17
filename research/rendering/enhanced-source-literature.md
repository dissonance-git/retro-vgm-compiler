# Enhanced source rendering: literature + implementation quarry

This note records the August 2026 SciSpace and GitHub pass behind the next SNES and OPN2 Enhanced work. It is a research input, not a creator-intent claim.

## Question

How far can the renderer move toward a cleaner source realization while preserving the exact executable musical idea?

Two concrete targets are in scope:

1. SNES: recover or identify the closest supported pre-BRR source before the S-DSP storage/reconstruction ceiling.
2. Mega Drive: retain the six-channel, four-operator OPN2 program while moving toward a higher-quality Yamaha-family realization rather than substituting a generic modern synth.

## SciSpace pass: source restoration and source identification

### Robust fingerprinting is useful for discovery, not lineage proof

Relevant papers include:

- Lebosse, Brun, Pailles, **A Robust Audio Fingerprint's Based Identification Method** (IBPRIA 2007), DOI `10.1007/978-3-540-72847-4_25`. The reported method is robust to compression and time shifting.
- Seo, **A Robust Audio Fingerprinting Method Based on Segmentation Boundaries** (2012), DOI `10.7776/ASK.2012.31.4.260`. The evaluation includes compression, equalization and time-scale modification.
- Ouali, Dumouchel, Gupta, **A robust audio fingerprinting method for content-based copy detection** (CBMI 2014), DOI `10.1109/CBMI.2014.6849814`.
- Kim, Cho, Kim, **Robust audio fingerprinting using peak-pair-based hash of non-repeating foreground audio in a real environment** (Cluster Computing, 2016), DOI `10.1007/S10586-015-0523-Z`; the tested distortions include pitch shift, time stretch, resampling, EQ and compression.

Translation for SPC work:

```text
robust fingerprint / invariant similarity
        ↓
rank possible original library samples
        ↓
NOT automatic restoration permission
```

A fingerprint can survive transformations precisely because it discards detail. That makes it good for candidate routing and bad as proof of exact preparation lineage.

`components/spc/spc_upstream_candidate_ranking.h` follows that distinction: it supplies an amplitude/DC-insensitive correlation score for candidate discovery, while `spc_sample_restoration.h` retains the stronger lineage/preparation gate for normal Enhanced playback.

### Blind bandwidth extension is an experiment, not recovered source truth

Relevant papers include:

- Liu, Lee, Hsu, **High frequency reconstruction for band-limited audio signals** (2003), reporting subjective/objective quality improvements from reconstructed high-frequency components.
- Liu, Lee, Hsu, **High frequency reconstruction by linear extrapolation** (AES lineage, 2006), reconstructing envelope/fine detail above a cutoff by spectral extrapolation.
- Moliner, Elvander, Valimaki, **Zero-Shot Blind Audio Bandwidth Extension** (2023), arXiv `2306.01433`, using a diffusion prior to reconstruct missing high-frequency content and reporting subjective gains on historical recordings.
- Liu, Bao, **Blind bandwidth extension of audio signals based on non-linear prediction and hidden Markov model** (2014), DOI `10.1017/ATSIP.2014.7`.

Translation for SPC work:

```text
known exact upstream source available
→ use the source with proven historical preparation mapping

no exact upstream source
→ higher-rate reconstruction / bandwidth extension may be tested
→ but remains inferred or aesthetic evidence
→ never label generated HF as recovered original information
```

The preferred route is therefore source identification + forward validation, not hallucinating detail into a 32 kHz final mix.

## SciSpace pass: digital synthesis quality

The directly relevant signal-processing result is not "96 kHz is automatically better". It is that nonlinear / rich-spectrum synthesis can create above-Nyquist energy, and high internal rate or explicit antialiasing is a standard way to prevent that energy from folding back into the audible band.

Relevant papers include:

- Bilbao, Esqueda, Valimaki, **Antiderivative antialiasing, Lagrange interpolation and spectral flatness** (WASPAA 2017), DOI `10.1109/WASPAA.2017.8170011`. The paper describes oversampling, commonly 4x-8x, as the usual antialiasing approach for nonlinear audio processing, while proposing alternatives for memoryless nonlinearities.
- Pekonen, Valimaki, **Filter-based alias reduction for digital classical waveform synthesis** (ICASSP 2008), DOI `10.1109/ICASSP.2008.4517564`.
- Lazzarini, Timoney, **Higher-Order Frequency Modulation Synthesis** (2023), arXiv `2305.07909`, analyzing FM/PM operator topologies and feedback while preserving their synthesis meaning.
- Stilson, Smith, **Alias-Free Digital Synthesis of Classic Analog Waveforms** (ICMC 1996), a broader bandlimited-synthesis reference.

Translation for Enhanced OPN2:

```text
preserve programmed FM topology and modulation semantics
+ run the synthesis/reconstruction path at sufficient internal precision/rate
+ explicitly control nonlinear/aliasing artifacts
```

This literature supports oversampling and antialiasing as quality tools. It does not support changing the OPN2 patch into a different Yamaha architecture.

## GitHub quarry: the Yamaha hardware-descendant baseline

Aaron Giles' `ymfm` is especially useful because its OPN2 family expresses three closely related realizations under one register/synthesis family:

```text
ym2612
ym3438 : public ym2612
ymf276 : public ym2612
```

`ymfm` documents the YM2612/YM3438/YMF276 in the OPN2 family and keeps the same six-channel FM surface. More importantly, the implementation exposes a concrete quality ladder:

### YM2612

`ym2612::generate()` models:

- six FM channels;
- 9-bit intermediate clipping;
- the YM2612 DAC discontinuity;
- per-channel discontinuity application before final averaging.

### YM3438 / OPN2C

`ym3438::generate()` retains the OPN2 program but removes the same YM2612 DAC discontinuity while retaining 9-bit intermediate clipping.

### YMF276 / OPN2L

`ymf276::generate()` retains the YM2612-derived OPN2 interface but uses:

- **14-bit intermediate clipping** rather than 9-bit;
- **proper channel mixing** rather than the YM2612 multiplex/average approximation;
- no YM2612 DAC discontinuity;
- the same six-channel OPN2 musical surface.

This makes YMF276 unusually close to the desired user-facing analogy:

> the same six-channel composition performed on a later, cleaner Yamaha OPN2 descendant.

It is therefore the current first hardware-descendant Enhanced baseline, recorded in `components/vgm/enhancement/ym2612_enhanced_realization.h`.

A future `studio_precision_opn2` backend can go beyond that hardware ceiling through higher internal precision, higher-rate reconstruction and antialiasing, but it must first beat the YMF276 descendant under identity and listening tests. It should not be allowed to define "better" by being more different.

## SNES target, sharpened

The target hierarchy is now:

```text
A. proven original / pre-BRR sample
   + exact trim/resample/loop/amplitude preparation
   + original S-DSP pitch/envelope/articulation/routing
   = strongest Enhanced realization

B. exact game BRR only
   + source-domain high-quality reconstruction
   + higher-rate phase trajectory
   = deterministic ceiling improvement

C. plausible external sample only
   + robust fingerprint/correlation
   = discovery candidate only

D. no source candidate
   + blind bandwidth extension / generative restoration
   = reversible experiment only
```

Automatic substitution remains limited to A under the existing evidence gate.

## Mega Drive target, sharpened

The FM ladder is now:

```text
reference YM2612
→ YM3438 OPN2C comparison
→ YMF276 OPN2L hardware-descendant Enhanced baseline
→ studio-precision OPN2 experiment
```

Locked identity:

- six channels;
- four operators per channel;
- eight algorithms;
- register/write ordering;
- FNUM/BLOCK pitch trajectory;
- operator ratios/detune;
- total levels;
- ADSR/SSG-EG program;
- feedback;
- LFO/AMS/FMS;
- channel-3 special mode;
- key-on/off timing;
- authored pan;
- DAC role kept separate from FM enhancement when required.

Relaxable only as separately tested ceilings:

- YM2612 ladder-DAC discontinuity;
- 9-bit intermediate clipping;
- multiplex/output reconstruction;
- avoidable aliasing;
- arithmetic precision;
- final rate conversion and headroom.

## Next implementation discriminators

1. Add a dependency-boundary YMF276 renderer that consumes the existing `ym2612_timed_write` stream and exposes six causal FM stems before final mix.
2. Compare reference YM2612, YM3438 and YMF276 under identical command traces and exact timing.
3. Require all three to agree on note/key/algorithm/operator-state transitions even when their audio differs.
4. Measure spectral alias energy, high-frequency extension, peak/headroom behavior and null residuals.
5. Feed YMF276 stems into the same exact-subtract + enhanced-add recomposition path already used by PSG.
6. For SPC, build corpus-side candidate search using robust fingerprints/correlation, followed by a forward preparation/BRR validation before automatic admission.
7. Keep blind bandwidth extension outside normal playback until a separate listening experiment earns a narrow role.
