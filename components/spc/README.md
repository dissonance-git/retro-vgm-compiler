# SPC component

This directory is the development home for the foobar2000 SPC path, its editable SNESAPU rendering foundation, and the project-owned SPC snapshot/runtime analysis layer.

Two upstream states remain intentionally distinct:

1. **editable SNESAPU source** is the implementation base;
2. the supplied **SPCPlay / improved SNESAPU v2.21.3 reference build** represents newer behavior to compare against.

The binary reference is not a substitute for editable source and must not become the runtime architecture.

For the common semantic model, see `../../docs/musical-execution-model.md`.
For the durable enhancement target, see `../../docs/source-native-enhanced-rendering.md`.

## Snapshot truth

`spc_snapshot.h` preserves the conventional SPC file image as separate source facts:

```text
header / CPU register image / ID666 area
64 KiB SPC700 RAM
128-byte S-DSP register image
optional trailing state when present
```

`spc_snapshot_graph_adapter.h` exposes the saved S-DSP register image and eight physical voice-register slots without pretending the file contains every hidden live DSP state used by an emulator.

Therefore:

```text
saved S-DSP register slot
!= live voice episode
!= persistent musical part

saved ENVX / OUTX
!= complete envelope / acoustic history

SRCN
= exact sample-directory index at the relevant observation
!= semantic instrument identity
```

The static snapshot layer deliberately does not fabricate live voice episodes, notes, persistent parts or MIDI/notation truth.

## BRR sample identity

`spc_brr_sample.h` lifts reusable BRR storage from exact RAM while keeping sample storage, directory references and instrument identity separate.

```text
voice/source observation
→ SRCN
→ directory entry
→ BRR start address
→ BRR block stream
```

The same BRR bytes/storage can participate in different references or musical uses without becoming several unrelated sample objects, while identical-looking storage at different proven source locations is not collapsed without stronger identity evidence.

```text
sample slot/reference
!= stored sample object
!= instrument identity
```

## Runtime voice lifecycle

`spc_runtime_voice_adapter.h` represents controlled live S-DSP voice episodes from instrumented runtime state rather than inferring lifecycle from a static saved register image.

The bounded physical lifecycle is approximately:

```text
accepted KON
→ physical voice episode begins

key-on delay / sample-producing state
→ same episode continues

KOFF / release
→ same episode continues in release

runtime inactive
→ physical voice episode ends
```

A retriggered accepted KON closes the previous bounded episode and begins another on that physical slot. A capture/semantic gap terminates continuity with an explicitly incomplete boundary rather than inventing a clean musical release.

Critically:

```text
KOFF
!= end of physical voice episode

physical voice episode
!= musical note
!= instrument
!= persistent part
```

## Runtime sample/source continuity

The earlier documentation treated runtime sample continuity as the next pressure point. That frontier has now moved.

The repository includes runtime sample/source tracking and RAM-generation/shadow machinery so event-time BRR identity can remain tied to the correct RAM version rather than assuming one static snapshot forever.

Current source-side work can preserve or reason about:

- runtime physical voice episodes;
- current source/sample observations;
- BRR storage and event-time RAM generation where continuity is known;
- device-native pitch rate;
- envelope/key/release/inactive state;
- source transitions and continuity loss;
- exact snapshot/runtime evidence needed by higher analysis.

These facts still do not automatically prove authored note names, absolute sample tuning, original driver tracks, persistent musical parts or auditory-stream identity.

## Current semantic frontier

SPC is now a second major source-family pressure test for the shared musical model rather than merely a future telemetry target.

The path is:

```text
exact SPC snapshot
        ↓
controlled executable continuation
        ↓
runtime S-DSP events / sample state
        ↓
bounded physical voice episodes
        ↓
source-relative performance evidence
        ↓
persistent musical identity / structure only when justified
        ↓
synchronized whole-song reasoning
```

Persistent-part analysis must use the strongest available combination of source/sample continuity, timing, control behavior, driver evidence when recovered, and other musical constraints. A hardware voice number is not enough.

### Cross-voice handoff null

Real-corpus pressure now gives this warning a stronger executable form. Pairwise cross-voice similarity is not enough even when it combines the same event-time BRR source, close timing, and source-relative pitch continuity. On the 31-cue creator-blind SPC panel, the handoff falsifier compressed:

```text
2,078 pairwise source/timing/pitch bundles
-> 34 bidirectionally unique cross-voice associations
-> 14 with strong same-voice flanks
-> 14 capture-boundary-safe candidates
-> 12 synchronized directed voice-swap cycles
-> 2 non-cyclic candidates
-> 0 uncontested candidates after same-voice competition
```

The cross-voice confidence ceiling therefore remains deliberate. One-in/one-out graph geometry is a useful falsifier and diagnostic, not a new independent evidence domain.

Pinned N-SPC disassembly also shows that the ordinary Cube and Quintet music paths keep eight logical sequence lanes tied to fixed S-DSP voice-register bases. Therefore:

```text
cross-voice runtime similarity
!= same recovered driver track moving slots

fixed driver track lane
!= proof that two distinct tracks form one musical part
```

If a future cross-voice persistent-part hypothesis is to exceed the current ceiling, it needs an independent authored relation between distinct driver tracks or another independent musical witness. Do not obtain that promotion by stacking more correlated runtime similarity.

See `../../research/validation/spc-cross-voice-handoff-null.md`.

## Engineering foundation

The editable SNESAPU source remains the implementation foundation. The supplied v2.21.3 package remains a behavior/version reference that must be reconciled in source form before audible enhancement claims rely on it.

Reference parity, runtime analysis, enhancement-core tests and retained listening improvements are separate validation states.

## Enhancement frontier

Accuracy remains the scientific/reference render, not the quality ceiling.

The target is **not** `SPC -> MIDI -> SoundFont` and not a generic VST replacement.

SoundFont/DLS/VST ecosystems are useful research observatories for sample-based synthesis, multisampling, envelopes, modulation, interpolation, routing and rendering quality. They are not the planned playback backend.

The intended SPC enhancement route is:

```text
exact BRR/sample identity
+ exact pitch / envelope / loop / articulation history
+ exact routing / echo behavior
        ↓
upstream sample provenance when available
        ↓
identify historical edits / filtering / truncation / looping / BRR encoding
        ↓
preserve intentional transformations
+ relax unwanted degradation
        ↓
higher-quality realization of the same SNES instrument design
```

Enhanced SPC playback may pursue, one variable at a time:

- higher-quality BRR/sample realization;
- source-provenance-assisted reconstruction when an upstream sample is known;
- higher-rate/high-precision interpolation;
- transient and low-frequency body recovery;
- higher-precision per-voice synthesis and summation;
- dry/echo separation;
- higher-quality realization of authored echo/environment intent;
- source-aware stereo construction before downstream Omniphony processing.

### Upstream samples are evidence, not automatic replacements

If an original pre-BRR sample or original synth preset can be identified, compare the whole transformation chain:

```text
upstream source sample
        ↓ edit / filter / truncate / retune / loop
prepared game sample
        ↓ BRR encode
shipped sample
        ↓ programmed envelope / pitch / echo / mix
shipped instrument realization
```

Then ask which transformations were only storage/quality loss and which became part of the instrument.

Do not merely swap in the pristine source sample. A composer may have deliberately selected or modified material for how the degraded SNES result behaved.

### Historical-intent evidence

Creator testimony shows both sides of this boundary.

Some practitioners described:

- manual sound-data programming as a production burden;
- severe sample-memory pressure as a quality problem;
- notes/sections being removed or shortened to fit;
- later higher-capacity recordings as opportunities to move closer to their original intentions.

Other practitioners deliberately exploited:

- waveform truncation artifacts;
- tiny sample loops;
- fake/repeated echoes;
- constrained timbres that only existed because of the platform.

Therefore no platform-wide rule such as `uncompressed = intended` is valid.

See `../../research/cases/historical-constraint-friction-counterfactual-rendering.md`.

### Memory accounting

The SNES S-SMP/S-DSP audio subsystem has 64 KiB of local RAM. That space may be occupied by driver/code/data, sequence state, BRR samples, directory structures, echo buffer/state, and other audio-engine data.

Graphics do not literally consume this local audio RAM. Cartridge ROM is a separate whole-game storage constraint that may also limit how much audio data can be stored or swapped.

Use game-specific resource evidence where available rather than one generic soundtrack-memory number.

## Validation rule

Any reconstructed information that was never present in the source must remain conditional, reversible and distinguishable from historical truth.

Each proposed relaxation should identify:

1. the exact historical ceiling being changed;
2. the evidence that the ceiling was unwanted or safely relaxable;
3. the musical/instrument identities that must remain locked;
4. at least one reference where the original artifact should intentionally survive;
5. the measurable and listening result of the A/B.

Normal playback remains realtime. Whole-song analysis may operate separately over captured/source evidence and must not become a prerequisite for hearing an SPC.
