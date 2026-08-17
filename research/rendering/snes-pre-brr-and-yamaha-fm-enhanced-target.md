# SNES pre-BRR and high-end Yamaha FM enhanced target

Date: 2026-08-17

This note freezes the product target behind the independent `Enhanced` playback option for the two current source families.

## 1. SNES: the preferred source is before BRR when it can be proven

The ideal SNES path is not merely a cleaner reconstruction of the shipped 32 kHz S-DSP bus.

It is:

```text
original production sample / source-library waveform
        ↓
known game preparation
(trim / resample / gain / loop / intentional filtering)
        ↓
BRR encoding
        ↓
shipped S-DSP instrument
```

When the exact upstream source and the preparation chain are recoverable, Enhanced should render from the upstream side of the BRR loss while replaying the game's exact musical control trajectory:

```text
proven pre-BRR waveform
+ exact game preparation that carried musical identity
+ exact SRCN / pitch / PMON / envelope / KON-KOFF / loop history
+ exact authored stereo route
+ independent shared echo field
        ↓
high-rate source reconstruction
```

If that source cannot be proven, Enhanced falls back one rung:

```text
exact BRR decode
+ sub-native source-coordinate reconstruction
+ higher-quality interpolation / arithmetic
```

It does **not** invent a hypothetical master.

### Why the evidence rule matters

The SciSpace literature pass separates two very different operations that are often called audio super-resolution.

Classical reconstruction can recover additional detail only when the signal has exploitable structure or additional constraints. Modern music super-resolution systems such as phase-aware GAN methods and AudioSR explicitly **predict** missing high-frequency components. Those systems can improve perceived quality, but their extra bandwidth is generated content rather than historical evidence.

For this project that yields a hard boundary:

```text
generative bandwidth extension
    = reversible listening experiment only

verified upstream source retrieval
    = eligible source-native restoration
```

Relevant literature:

- Markovich et al., *Benchmarking Compressed Sensing, Super-Resolution, and Filter Diagonalization* (2015), arXiv:1502.06579.
- Hu et al., *Phase-Aware Music Super-Resolution Using Generative Adversarial Networks* (INTERSPEECH 2020), DOI 10.21437/INTERSPEECH.2020-2605.
- Liu et al., *AudioSR: Versatile Audio Super-resolution at Scale* (2023), arXiv:2309.07314.

### Finding the original sample

The literature pass on robust audio fingerprinting supports using distortion-resistant fingerprints as a **candidate retrieval stage**. Methods have been demonstrated against combinations of compression, equalization, filtering, time-scale change and related content-preserving transforms.

That is useful for old sample-library archaeology because the game asset may be a short, resampled, filtered, gain-changed and BRR-quantized descendant of a library waveform.

But fingerprint similarity is not provenance. The pipeline is therefore:

```text
BRR object
↓ decode
robust fingerprint / transformed-search descriptors
↓
shortlist upstream candidates
↓
fit only explicit preparation transforms
↓
compare transformed upstream candidate to decoded game sample
↓
verify loop / trim / resample / gain mapping
↓
independent historical/source evidence
↓
exact lineage admitted
```

Relevant literature:

- Seo, *A Robust Audio Fingerprinting Method Based on Segmentation Boundaries* (2012), DOI 10.7776/ASK.2012.31.4.260.
- Dupraz & Richard, *Robust frequency-based Audio Fingerprinting* (ICASSP 2010), DOI 10.1109/ICASSP.2010.5495944.
- Ouali, Dumouchel & Gupta, *A robust audio fingerprinting method for content-based copy detection* (2014), DOI 10.1109/CBMI.2014.6849814.

### Retrieval is now executable

`tools/spc_original_sample_candidates.py` implements the first corpus-search rung. It intentionally performs **candidate retrieval only**:

- PCM WAV ingestion and mono folding;
- DC/gain normalization;
- coarse duration/window search for resampled or trimmed descendants;
- multiscale smoothed waveform correlation, which downweights local BRR/filter damage;
- derivative correlation as a smaller edge/transient term;
- JSON-ranked candidate output with an explicit claim boundary.

A high ranking still has no authority to replace audio. The candidate must pass `spc_sample_lineage_verification.h` and independent exact-lineage evidence before it can enter `spc_original_sample_bank.h`.

### Code frontier now present

```text
spc_sample_restoration.h
    evidence + exact preparation map

spc_upstream_sample_reconstruction.h
    candidate reconstruction separated from automatic admission

spc_sample_lineage_verification.h
    transformed-source vs decoded-BRR validation metrics

spc_original_sample_bank.h
    ambiguity-safe runtime lookup of proven pre-BRR sources

spc_enhanced_native_interval.h
    source-coordinate reconstruction when no upstream master is available

tools/spc_original_sample_candidates.py
    distortion-tolerant candidate retrieval, never provenance by itself
```

The runtime bank refuses a choice when two different approved upstream identities conflict for the same BRR identity. Ambiguity returns to the BRR reference path rather than choosing whichever file appears first.

## 2. YM2612: six channels stay six channels

The FM target is best described as:

> replay the original six-channel Genesis FM composition on the highest-ceiling Yamaha-FM descendant that can preserve its patch/control identity.

For normal automatic Enhanced playback:

```text
6 physical FM channels                  LOCKED
4 source operators per channel          LOCKED
8 OPN algorithm topologies              LOCKED
operator ratios / detune / TL            LOCKED
key timing / articulation                LOCKED
feedback program                         LOCKED
source event order                       LOCKED
stereo-route evidence                    LOCKED

sine-table precision                     RELAXABLE
phase precision                          RELAXABLE
amplitude arithmetic precision           RELAXABLE
channel accumulator clipping             RELAXABLE when identity survives
output DAC ladder                        RELAXABLE
output bandwidth                         RELAXABLE
anti-aliasing / reconstruction quality   IMPROVABLE
```

This gives the musical effect of having a much more capable Yamaha FM instrument available at composition time **without changing the six authored parts into a larger arrangement**.

### Why automatic Enhanced does not add operators

Modern Yamaha FM engines can offer substantially richer operator graphs. Yamaha's current FM-X documentation describes an eight-operator engine and many more algorithms than the old OPN family.

That makes an expanded Yamaha-FM projection a useful experiment, but not a transparent default. Adding operators or selecting a different algorithm changes the modulation graph, and therefore can change the spectrum and instrument identity even when the notes remain identical.

The SciSpace FM pass reinforces that topology is not cosmetic. Higher-order FM research explicitly treats modulation arrangement, feedback and digital implementation as part of the synthesis system. DDX7 likewise treats FM resynthesis as a constrained parameter-recovery problem rather than a trivial preset conversion.

Relevant literature:

- Lazzarini & Timoney, *Theory and practice of higher-order frequency modulation synthesis*, Journal of New Music Research (2024), DOI 10.1080/09298215.2024.2312236.
- Caspe, McPherson & Sandler, *DDX7: Differentiable FM Synthesis of Musical Instrument Sounds* (ISMIR 2022), arXiv:2208.06169.

The product rule is consequently:

```text
normal Enhanced
    = higher-fidelity OPN descendant
      preserving the original four-operator graph on six channels

expanded modern Yamaha FM
    = explicit experimental projection
      until patch-specific identity testing says otherwise
```

### Automatic rung: exact-state Nuked carrier lift

The first audible automatic FM rung no longer needs an approximate second envelope/control engine.

`nuked_opn2_hq_lift.h` reads the **same live authoritative Nuked OPN2 state** immediately around each native chip cycle. Nuked remains responsible for:

- register/write-buffer timing;
- FNUM, block, detune and multiplier behavior;
- key-on/off and exact envelope state;
- LFO, AM and PM;
- channel-3 special mode and CSM;
- SSG-EG;
- the exact quantized modulation history;
- authored pan and source mute state.

The lift changes only the final carrier/channel/output ceiling:

```text
exact Nuked q10 modulated phase
+ exact Nuked eg_out attenuation
        ↓
continuous sine carrier
        ↓
exact OPN carrier-connect topology
        ↓
floating carrier accumulation
(no 9-bit channel clamp)
        ↓
FM bus without YM2612 sign-leak/DAC-ladder artifact
        ↓
no optional MD1 analog low-pass
```

The six lifted FM lanes are then sent through the **same outer libvgm linear-resampler timing and device-volume coordinate** as the exact six reference FM lanes. PlayerA performs:

```text
protected reference mix
- exact FM1..FM6
+ lifted FM1..FM6
```

all six at once. DAC remains a separate seventh YM source identity and is untouched by this FM operation.

This is intentionally conservative. Modulator history is still the exact hardware-quantized OPN history, which preserves difficult timbral/control identity. The audible improvement is at the final sine/carrier sum/channel/DAC/bandwidth ceiling rather than an uncontrolled reinterpretation of the patch.

### Deeper experimental rung

`ym2612_hq_fm_backend` remains the deeper all-floating candidate engine. It:

- keeps six FM channel states and four source operators per channel;
- uses the Nuked OPN2 operator-routing table as modulation topology;
- preserves ordered VGM register writes instead of converting to MIDI;
- reconstructs OPN detune/multiple phase increments into a floating oscillator domain;
- runs at a bounded oversampled internal rate;
- can eventually relax modulation-path quantization as well as output quantization.

Its current smooth envelope/control implementation is not yet a universal automatic replacement for every OPN semantic. The exact-state lift exists specifically so normal `Enhanced` can improve FM now without pretending that deeper experimental renderer has already matched every control behavior.

### Reference-source correctness repair

While building the FM lift, the source-aware Nuked decomposition was re-audited against the pinned libvgm core and corrected to mirror the real `NOPN2_GenerateResampled` path:

- `NOPN2_Clock`, `smplRate`, `dacen`, `rateratio`, and `samplecnt` semantics now match the pinned ABI;
- the buffered-register-write scheduler is executed after each authoritative chip clock;
- the 24-cycle output-bus mute/channel mapping is retained;
- the no-filter x11 scaling and optional MD1 filter recurrence match the pinned core;
- the exact mix remains the accounting authority, with only bounded per-lane filter-rounding residual accepted.

This repair matters more than any enhancement. An Enhanced delta is meaningful only if the source being subtracted is the exact source that actually built the protected reference mix.

## 3. Product composition

The two quality paths remain orthogonal to Omniphony:

```text
SNES / VGM source execution
        ↓
reference source lane(s)
        ├───────────── Enhanced OFF ─────────────┐
        │                                        │
        └─ Enhanced ON                           │
           ├─ proven pre-compression sample      │
           ├─ high-rate BRR source reconstruction│
           ├─ exact-state higher-ceiling OPN FM  │
           └─ source-specific descendants        │
                         ↓                        │
                  source result ←────────────────┘
                         ↓
             Spatial OFF / Spatial ON
                         ↓
                  stereo / Omniphony
```

The scientific control is always reachable. No source enhancement is allowed to hide inside the spatial checkbox, and no spatial interpretation is allowed to masquerade as synthesis quality.
