# Causal musical-spatial runtime: literature and implementation pass

Date: 2026-08-15

This pass asks a narrow engineering question:

> What external evidence should change the realtime Game Music Interpreter → Omniphony path, without turning playback into offline pre-analysis or replacing source-authoritative game-music evidence with generic audio inference?

The answer is not a new monolithic model. The evidence supports the architecture already emerging in GMI: exact source truth where available, small causal observations, persistent uncertainty-aware state, and a renderer-facing perceptual control layer. The main additions are clearer fallback rules for opaque audio, stronger temporal-hypothesis discipline, and an implementation pattern for source/object continuity.

## Decision summary

```text
source-native game format
    ↓
exact emulator / driver / source planes when available
    ↓
causal acoustic + musical observations
    ↓
persistent hypothesis state
    ↓
perceptually meaningful presentation controls
    ↓
Omniphony

opaque mixed audio only
    ↓
low-latency causal demixer fallback
    ↓
same downstream hypothesis + presentation pipeline
```

A neural separator is therefore a fallback source-recovery mechanism, not the preferred path for VGM, SPC, PSF-family execution, or any other format where the source machine can expose stronger evidence directly.

## Literature findings

### Low-latency music source separation

Venkatesh, Benilov, Coleman, and Roskam, **“Real-Time Low-Latency Music Source Separation Using Hybrid Spectrogram-TasNet”**, ICASSP 2024, DOI `10.1109/ICASSP48485.2024.10448381`.

Reported result: HS-TasNet reaches 23 ms latency with 4.65 dB overall SDR on MUSDB, increasing to 5.55 dB with additional training data. The useful architectural lesson is hybrid short-window waveform/spectral inference under a strict latency budget.

Wu et al., **“Towards Practical Real-Time Low-Latency Music Source Separation”**, 2025, DOI `10.1109/ICME59968.2025.11208966`.

Useful result: the RT-STT work explicitly optimizes a lightweight single-path architecture and quantization for realtime inference. This reinforces that a realtime fallback should be purpose-built for bounded latency rather than adapting a large offline separator unchanged.

Yang et al., **“Band-SCNet: A Causal, Lightweight Model for High-Performance Real-Time Music Source Separation”**, Interspeech 2025, DOI `10.21437/Interspeech.2025-448`.

Reported result: 2.59 M parameters, 7.79 dB SDR, 92 ms latency on MUSDB18-HQ. This is a useful quality/latency frontier point, but its latency is much larger than the source-native path should need.

**Adoption rule:** preserve these models as fallback teachers/benchmarks. Do not demix a source that the emulator already knows exactly.

### Online musical timing state

Heydari, Cwitkowitz, and Duan, **“BeatNet: CRNN and Particle Filtering for Online Joint Beat, Downbeat and Meter Tracking”**, 2021/2022, arXiv `2108.03576`.

The important pattern is:

```text
causal frame evidence
→ persistent probabilistic timing hypotheses
→ online beat / downbeat / meter state
```

The system uses causal convolutional/recurrent layers followed by sequential Monte Carlo particle filters. Its information-gating strategy reduces particle-filter cost.

**Adoption rule:** phrase, pulse, meter, and section state should not be treated as a smoothed scalar derived independently from each block. GMI should expose noisy causal observations and maintain bounded competing hypotheses through time. This matches the existing ambiguity-preserving identity and role machinery.

### Auditory continuity

Cao, Parks, and Goldwyn, **“Dynamics of the Auditory Continuity Illusion”**, Frontiers in Computational Neuroscience 15 (2021), DOI `10.3389/FNCOM.2021.676637`.

Their dynamical account demonstrates two relevant mechanisms: hysteresis and bistability. Continuity can persist through an interruption when there is sufficient evidence for continuation and no sufficiently strong evidence of discontinuity.

**Adoption rule:** absence or masking is not automatically identity death. Realtime object/part memory should persist conservatively through weak evidence and break on positive discontinuity evidence, with confidence kept separate from identity assertions.

This agrees with the deepSTRF/libaural continuation work already imported into GMI: freeze durable state under weak evidence, preserve ambiguity, and re-adjudicate on re-entry.

### Realtime multi-source tracking

McCormack, Politis, Särkkä, and Pulkki, **“Real-Time Tracking of Multiple Acoustical Sources Utilising Rao-Blackwellised Particle Filtering”**, EUSIPCO 2021, DOI `10.23919/EUSIPCO54536.2021.9616095`.

The system combines direct-path-dominance testing, grid-less localization, and Rao-Blackwellised particle filtering, with realtime multi-source tracking and changing source counts.

The directly reusable idea is not literal 3-D localization. It is the state discipline:

- explicit measurement uncertainty;
- persistent target IDs;
- birth/death as hypotheses rather than instantaneous decisions;
- tracker time advances even when no measurement arrives;
- clutter/noise is modeled as such rather than forced into an object.

**Adoption rule:** GMI musical-object continuity should use the same conceptual separation between observation and persistent object state. Do not import a 3-D acoustic tracker merely because Omniphony renders in 3-D.

## GitHub observatories

Repository observations are evidence about implementation patterns, not automatic dependencies.

### `sweetspotsoundsystem/stemgen-rt`

Observed 2026-08-15.

This is the strongest practical fallback reference found in the pass. It implements an MIT-licensed JUCE/ONNX realtime four-stem plugin using HS-TasNet and reports 11.6 ms latency. Its realtime engineering is more valuable to GMI than its particular learned separator:

- audio callback writes to a ring buffer;
- inference runs on a separate thread;
- processed stems are consumed without blocking the callback;
- deadline failure crossfades to dry audio rather than glitching;
- chunk boundaries are crossfaded;
- low-frequency content is stabilized/reinjected from constrained dry evidence;
- silence gating suppresses hallucinated output;
- rejected vocal energy is reassigned instead of silently losing mix energy.

**Adoption rule:** if GMI later supports opaque stereo through learned source separation, use this class of fail-closed realtime transport. Do not put neural inference directly on the audio callback and do not let missed inference corrupt continuity.

### `cwitkowitz/BeatNet`

Observed `main` on 2026-08-15.

The repository exposes the causal CRNN and `particle_filtering_cascade.py`. The implementation makes beat/downbeat likelihoods observations over a persistent state space rather than treating each activation independently.

**Adoption rule:** copy the architecture, not the Python runtime. GMI should implement a small fixed-capacity C++ timing-hypothesis layer if/when rhythm state begins steering presentation.

### `leomccormack/Spatial_Audio_Framework`

Observed 2026-08-15.

SAF contains a realtime Rao-Blackwellised particle-filter multi-target tracker and mature spatial-audio primitives. The tracker explicitly models target birth/death, measurement noise, clutter likelihood, persistent IDs, and advancement through steps with no new observations.

License boundary matters: the optional tracker module is GPLv2. Its architecture is an observatory unless the target project intentionally accepts that license boundary.

**Adoption rule:** use the state-model lessons, not copied tracker code.

### `SoundScapeRenderer/ssr`

Observed 2026-08-15.

SSR remains a useful mature object/source spatial-rendering observatory. Its existence reinforces the architectural boundary between source objects and a renderer capable of multiple reproduction strategies.

**Adoption rule:** GMI describes what the musical/perceptual source is doing; Omniphony owns the headphone rendering geometry.

### `NMLAB8/Mamba-S-Net`

Observed 2026-08-15.

The repository is useful as a teacher for efficient long-context state-space modeling in music separation. It is not evidence that a large state-space network belongs in the playback callback.

## Immediate implication for the VGM/SPC Omniphony path

SPC already has a source-authoritative causal pipe. The current VGM foobar path still renders a stereo block and applies chip-state-informed stereo enhancement. Its `VgmRegShadow` provides strong register/timing evidence but not isolated source waveforms.

Inspection of the vendored libvgm Gens YM2612 core found a much stronger seam: each of six FM channels is rendered sequentially before being summed, while DAC is added separately. Therefore the source plane can be captured at synthesis time rather than reconstructed from the final mix or guessed from registers.

This suggests the next bounded implementation:

```text
libvgm reference YM2612 render
├─ FM1 contribution
├─ FM2 contribution
├─ FM3 contribution
├─ FM4 contribution
├─ FM5 contribution
├─ FM6 contribution
└─ DAC contribution
        +
SN76489 source contributions
        ↓
request-aligned neutral VGM source block
        ↓
GMI causal musical frontend
        ↓
Omniphony ABI 0.3 source objects/events
```

The ordinary libvgm stereo output remains the scientific control and fail-closed fallback.

Because libvgm offers multiple YM2612 cores (GPGX, Gens, Nuked), source capture must advertise capability rather than pretending every core exposes the same internal decomposition. The first implementation may be a validated Gens capture path, but the product must either preserve the user-selected reference core or explicitly separate the source-analysis core from audible reference authority. A source-analysis core must never silently replace the reference renderer.

## New runtime invariants from this pass

1. **Native-source precedence**
   - exact emulator/driver source evidence outranks learned demixing when available.

2. **Observation/state separation**
   - a frame/block observation never becomes a persistent object or musical role merely because it is locally strong.

3. **No-observation advancement**
   - persistent state advances in time even when a source is masked, silent, or temporarily unobserved.

4. **Positive-discontinuity rule**
   - weak evidence may lower confidence without killing identity; positive discontinuity evidence may break identity.

5. **Ambiguity preservation**
   - competing meter/object/part hypotheses may coexist until evidence separates them.

6. **Callback safety**
   - expensive inference never blocks the audio callback; deadline misses degrade to a defined reference path.

7. **Energy/accounting discipline**
   - fallback separation should expose conservation/residual accounting so a cleaner stem does not quietly mean a damaged mix.

8. **Perceptual-control boundary**
   - musical meaning should steer non-geometric controls such as foreground, foundation, diffusion, width, and vertical affinity; the binaural renderer owns final geometry.

## Next discriminating tests

The next useful tests are not another broad literature sweep. They are executable:

1. prove that a tapped YM2612 source plane recomposes to the Gens reference output sample-for-sample for FM-only fixtures;
2. prove the same with DAC active;
3. prove source capture does not alter reference output when enabled;
4. prove source identity and authored route changes can be emitted at exact intra-block sample offsets;
5. run source swaps, masking, dropout, and re-entry fixtures through the GMI identity layer and verify conservative hold vs positive discontinuity;
6. only after native formats are stable, benchmark an opaque-stereo fallback against HS-TasNet/StemgenRT-class latency and failure behavior.

The research conclusion is therefore conservative but useful: **do not replace the present causal architecture. Complete it.** The missing work is better source evidence and stronger persistent musical hypotheses, not a larger all-purpose model.