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

### First executable high-fidelity backend

`ym2612_hq_fm_backend` now exists as the first bounded candidate engine.

It:

- keeps six FM channel states;
- keeps four operator states per channel;
- uses the Nuked OPN2 operator-routing table as the modulation topology;
- preserves ordered VGM register writes rather than converting the music to MIDI;
- reconstructs OPN detune/multiple phase increments into a floating-point oscillator domain;
- runs at a bounded oversampled internal rate;
- replaces the quantized sine/output arithmetic with floating-point synthesis;
- separates the YM2612 DAC from FM channel 6;
- fails its automatic-admission fence when it encounters semantics not yet matched closely enough, currently including enabled OPN LFO/AM/PM, channel-3 special mode, or SSG-EG.

That last behavior is intentional. Unsupported semantics do not become zero, generic FM, or a guessed modern equivalent. The exact Nuked reference lane remains the playback answer for that source until the enhanced implementation reaches it.

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
           ├─ higher-ceiling Yamaha FM           │
           └─ source-specific descendants        │
                         ↓                        │
                  source result ←────────────────┘
                         ↓
             Spatial OFF / Spatial ON
                         ↓
                  stereo / Omniphony
```

The scientific control is always reachable. No source enhancement is allowed to hide inside the spatial checkbox, and no spatial interpretation is allowed to masquerade as synthesis quality.
