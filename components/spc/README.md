# SPC component

This directory is the development home for the foobar2000 SPC input component and its editable SNESAPU rendering foundation.

## Two different upstream states

Do not collapse these into one thing:

1. **Editable SNESAPU source** is the implementation base.
2. **SPCPlay / improved SNESAPU v2.21.3 reference build** supplied as `spcplay-2.21.3.9130.zip` represents newer behavior that the editable source has not yet fully incorporated.

The supplied reference package is not the foobar component source and must not become a binary dependency architecture.

## First engineering sequence

1. Import/preserve the existing `foo_snesapu` wrapper source and its license/provenance.
2. Import the editable SNESAPU source with dgrfactory/Alpha-II provenance intact.
3. Establish a build of the editable source before enhancement.
4. Diff/reconcile behavior and implementation against the supplied v2.21.3 reference and current dgrfactory material.
5. Verify parity for the relevant playback paths.
6. Expose the eight SNES DSP voices, source/sample identity, pitch, envelope/key state, authored L/R volume, echo routing, FIR/feedback state, noise/pitch-modulation state, and other useful realtime information at a stable boundary.
7. Only then begin source-native enhancement.

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
