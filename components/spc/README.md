# SPC component

This directory is the development home for the foobar2000 SPC input component and its editable SNESAPU rendering foundation.

## Two different upstream states

Do not collapse these into one thing:

1. **Editable SNESAPU source** is the implementation base.
2. **SPCPlay / improved SNESAPU v2.21.3 reference build** supplied as `spcplay-2.21.3.9130.zip` represents newer behavior that the editable source has not yet fully incorporated.

The supplied reference package is not the foobar component source and must not become a binary dependency architecture.

## Snapshot-first model

The first project-owned SPC model now begins below playback, at the exact `.spc` machine snapshot.

`spc_snapshot.h` preserves the conventional SNES-SPC700 file image as separate source facts:

```text
0x00000..0x000FF  header / CPU register image / raw ID666 area
0x00100..0x100FF  64 KiB SPC700 RAM
0x10100..0x1017F  128-byte S-DSP register image
0x10180..0x101FF  optional trailing state when present
```

`spc_snapshot_graph_adapter.h` then exposes the saved S-DSP register image and its eight physical voice-register slots without pretending that the file contains every hidden live DSP state needed by an emulator.

This distinction is important. Mature emulators restore the saved CPU/RAM/register image and then initialize additional runtime DSP state such as decoded BRR buffers, interpolation position, envelope phase, key delay, echo history and other internal pipeline state. Ares likewise maintains substantially richer live per-voice and echo state than the 128 saved DSP registers alone contain.

Therefore:

```text
saved S-DSP register slot
!= live voice episode
!= persistent musical part

saved ENVX / OUTX byte
!= complete envelope / acoustic history

SRCN
= exact saved sample-directory index
!= semantic instrument identity
```

The current snapshot graph can recover exact or deterministic facts such as:

- SPC700 PC/A/X/Y/PSW/SP register image;
- exact 64 KiB sound RAM;
- exact 128-byte S-DSP register image;
- eight saved physical voice-register slots;
- per-slot L/R volume, pitch code, SRCN, ADSR/GAIN and saved ENVX/OUTX;
- saved global KON/KOFF, PMON, NON and EON masks;
- sample-directory page and each slot's current directory entry;
- sample start and loop addresses derived from exact RAM;
- echo volume/feedback/start/delay and eight FIR coefficients;
- optional trailing bytes when the full-size SPC image preserves them.

It deliberately creates **no** `voice_instance`, musical event, persistent part, or MIDI/notation projection from a static snapshot alone.

This makes SPC a useful second-source pressure test against the VGM path:

```text
VGM
→ chronological command trace
→ replayed device state

SPC
→ machine-state snapshot
→ executable continuation
```

The common model must support both without forcing one source geometry into the other.

## First engineering sequence

1. Preserve and validate the exact SPC snapshot representation before assuming emulator-internal state is source truth.
2. Import/preserve the existing `foo_snesapu` wrapper source and its license/provenance.
3. Import the editable SNESAPU source with dgrfactory/Alpha-II provenance intact.
4. Establish a build of the editable source before enhancement.
5. Diff/reconcile behavior and implementation against the supplied v2.21.3 reference and current dgrfactory material.
6. Verify parity for the relevant playback paths.
7. Add controlled execution from an exact snapshot and observe which live S-DSP/driver distinctions become recoverable beyond the saved register image.
8. Expose the eight SNES DSP voices, source/sample identity, pitch, envelope/key state, authored L/R volume, echo routing, FIR/feedback state, noise/pitch-modulation state, and other useful realtime information at a stable boundary.
9. Only then begin source-native enhancement.

## Enhancement frontier

The target is not stricter SNES authenticity. Accuracy remains available as the scientific/reference render.

Enhanced playback may pursue:

- higher-quality BRR/sample realization
- high-rate / high-precision interpolation
- reusable-source reconstruction without replacing instrument identity
- transient and low-frequency body recovery
- per-voice masking reduction
- modern high-precision summation
- dry/echo separation
- a higher-quality realization of authored echo/environment intent
- source-aware stereo construction for later Omniphony processing

The result must remain realtime and traceable to information encoded in the SPC/DSP state.
