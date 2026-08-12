# SPC component

This directory is the development home for the foobar2000 SPC path, its editable SNESAPU rendering foundation, and the project-owned SPC snapshot/runtime analysis layer.

Two upstream states remain intentionally distinct:

1. **editable SNESAPU source** is the implementation base;
2. the supplied **SPCPlay / improved SNESAPU v2.21.3 reference build** represents newer behavior to compare against.

The binary reference is not a substitute for editable source and must not become the runtime architecture.

For the common semantic model, see `../../docs/musical-execution-model.md`.

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

## Engineering foundation

The editable SNESAPU source remains the implementation foundation. The supplied v2.21.3 package remains a behavior/version reference that must be reconciled in source form before audible enhancement claims rely on it.

Reference parity, runtime analysis, enhancement-core tests and retained listening improvements are separate validation states.

## Enhancement frontier

Accuracy remains the scientific/reference render, not the quality ceiling.

Enhanced SPC playback may eventually pursue:

- higher-quality BRR/sample realization;
- higher-rate/high-precision interpolation;
- source-conditioned reconstruction without replacing instrument identity;
- transient and low-frequency body recovery;
- per-voice masking reduction;
- modern high-precision summation;
- dry/echo separation;
- higher-quality realization of authored echo/environment intent;
- source-aware stereo construction before downstream Omniphony processing.

Any reconstructed information that was never present in the source must remain conditional, reversible and distinguishable from historical truth.

Normal playback remains realtime. Whole-song analysis may operate separately over captured/source evidence and must not become a prerequisite for hearing an SPC.
