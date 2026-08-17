# Original-sample-first SNES Enhanced

The target is not "make 32 kHz SPC output brighter." It is to move the reconstruction boundary as far upstream as evidence allows while leaving the game's performance/control system intact.

The listening target is the **same sample-based instrument realized as a contemporary release master**: what the preserved composition, arrangement and performance could sound like if released today without the historical BRR/interpolation/numerical ceilings that are not themselves identity-bearing. This does not authorize rewriting the instrument. Loop design, pitch behavior, articulation, envelopes, routing, deliberate echo and other authored transformations remain part of the object.

## Quality ladder

Normal `Enhanced` chooses the highest proven rung available for each sample identity:

```text
1. verified original/upstream waveform at live game source phase
2. verified pre-BRR prepared game-grid waveform
3. exact BRR source trajectory reconstructed at high rate
4. SNESAPU high-rate 8-point sinc BRR path
5. protected historical reference
```

The distinction between rungs 1 and 2 matters.

A source may have been trimmed, gain-scaled, looped and **downsampled before BRR encoding**. Replacing the BRR decoder with the exact prepared game-grid PCM removes BRR quantization, but it cannot restore genuine upstream samples already discarded by that earlier preparation. Direct evaluation of a proven original waveform can.

`components/spc/spc_enhanced_source_policy.h` makes that ordering explicit.

## What remains authored

Even at the top rung, the game keeps control over:

```text
SRCN / instrument selection
source phase and loop trajectory
PITCH
PMON
KON / KOFF
ADSR / GAIN
VxVOL routing
EON
shared echo / FIR / feedback
MVOL
song/driver timing
```

The restoration source changes the waveform evidence boundary, not the arrangement or performance.

## Modern reconstruction policy

Historical interpolation is part of the protected reference, but it is not automatically part of the Enhanced quality ceiling.

For a source-supported reconstruction, use the highest validated modern source-domain sampler that preserves the same waveform identity and trajectory. Current top-rung upstream PCM uses:

```text
components/spc/spc_studio_sample_reconstruction.h
```

Its first implementation is a tabled 64-tap Kaiser-windowed sinc with 16,384 fractional phases. The table can be prepared outside the audio callback, and the realtime path is a bounded dot product rather than per-sample trigonometry. The older eight-tap path remains an important fallback/control, not the endpoint for a proven original waveform.

Modern reconstruction must be judged by measurable passband accuracy, imaging/alias suppression, transient behavior, numerical stability, realtime cost and listening tests. Longer or newer does not win by name. Variable-rate/pitch-up anti-aliasing remains a separate requirement: a high-quality interpolation kernel must not be mistaken for a complete rate-conversion policy when the source trajectory crosses a downsampling boundary.

Normal Enhanced still cannot invent missing spectrum. If no source-supported waveform contains information above the historical preparation/BRR boundary, generative bandwidth extension remains an explicit experiment rather than playback truth.

## Current realtime-capable rung

The current dependency patch has a concrete pre-BRR block seam:

```text
apply_source_capture.py
upgrade_source_capture_v2.py
apply_prebrr_provider.py
```

`apply_prebrr_provider.py` calls an optional provider exactly when SNESAPU would decode one 9-byte BRR block. A successful provider supplies all sixteen exact prepared samples; otherwise the historical decoder runs normally. The BRR header remains authoritative for END/LOOP behavior.

The callback receives the **full host pointer** to the current BRR block. It does not assume the emulator's 64 KiB RAM happens to be 64-KiB aligned. `snesapu_prebrr_pointer_session` calibrates the arbitrary host-pointer low-16 bias from the known logical first BRR address at `StartSrc`, then maps later and wrapped block pointers back to logical SPC addresses.

Parent/child transport should be applied with:

```text
python patches/snesapu/apply_prebrr_transport_complete.py <foo_snesapu-root>
```

That composes the SPCP-v2 transport and the current calibrated five-argument child callback ABI.

The parent looks for an optional local sidecar beside the track:

```text
music.spc.prebrr
```

Missing sidecar is normal. It falls to the next quality rung.

## Producing a `.prebrr` sidecar

`tools/spc_prebrr_sidecar.py` accepts only already-approved mono 16-bit prepared PCM. With `spc_file` in the manifest, it derives SRCN -> directory -> first BRR block and the exact BRR extent directly from the snapshot. A hand-supplied start address becomes an assertion; disagreement fails. PCM length must match the shipped BRR block count exactly.

The sidecar builder performs no hidden resampling or gain fitting. Those transformations belong to the lineage-verification stage, where they remain explicit evidence.

## Direct original/native-resolution rung

The core already has the mathematical top rung in:

```text
components/spc/spc_original_sample_interval.h
```

It evaluates an automatically approved upstream waveform at the exact live game source coordinate and sub-native output phase. If an upstream-to-game coordinate map is 2:1, for example, a 96 kHz output interval can evaluate real upstream positions between historical game-grid samples instead of inventing them from the 32 kHz result.

That evaluator now reaches the studio reconstruction primitive through `spc_upstream_sample_reconstruction.h`; candidate/lineage validation and eventual playback therefore use the same source-domain interpolation model rather than validating an eight-tap approximation and rendering a different one later.

This is the final intended SNES source model:

```text
verified original waveform
        +
exact historical preparation mapping
        +
exact live game source phase
        ↓
modern bandlimited original-waveform reconstruction
        ↓
exact game control trajectory
        ↓
SNES routing / echo intent / presentation
```

The remaining dependency frontier is to place this direct-original evaluator at SNESAPU's live pre-envelope interpolation seam and give it the exact output-rate/pitch context needed for anti-aliased variable-rate reconstruction. Until that hot-loop integration is validated against the pinned assembly, rung 1 remains modeled/testable while rung 2 is the highest concrete dependency injection point.

## Discovery is not provenance

`tools/spc_original_sample_candidates.py` is intentionally only a candidate finder. Robust similarity under resampling/filtering/compression is useful for sample-library archaeology, but a high score does not authorize replacement.

Admission remains:

```text
candidate retrieval
-> explicit preparation transform
-> decoded-BRR validation
-> loop/trim validation
-> independent source/lineage evidence
-> automatic restoration approval
```

Generative audio super-resolution stays outside normal source truth. It may be useful as an optional listening/research experiment, but generated high-frequency content is not historical evidence.
