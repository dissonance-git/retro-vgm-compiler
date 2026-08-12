# VGM component

This directory is the development home for the VGM/VGZ foobar2000 path and the current Genesis-focused source-state/enhancement core.

The imported upstream wrapper remains the reference playback foundation. Project-owned analysis and enhanced-rendering work must stay traceable to exact VGM command/device state and must not silently replace reference behavior.

For full engineering status, see `../../docs/vgm-frontier.md`. For the common semantic model, see `../../docs/musical-execution-model.md`.

## Current execution path

The current project-owned vertical slice is:

```text
VGM command stream
        ↓
allocation-free ordered command capture
        ↓
decoded Genesis device transitions
        ↓
rebuildable YM2612 / SN76489 state
        ↓
bounded physical voice episodes
        ↓
conservative pitched-activity observations
        ↓
device-native pitch/control history
        ↓
higher musical identity / structure only when stronger evidence supports it
```

The source trace remains canonical evidence. Replayed device state is a rebuildable view, not a replacement for ordered transition history.

Capture gaps, truncated payloads and stateful-command ambiguity fail closed rather than fabricating later semantic continuity.

## Genesis state currently represented

YM2612 coverage includes register state, operator/key masks, F-number/block semantics, algorithm/feedback, routing, AMS/FMS/LFO, channel-3 special-mode state, operator parameters, DAC enable and resolved DAC source activity.

SN76489-family state includes tone periods, attenuation, noise state/control and stereo routing where available.

The current semantic adapters deliberately distinguish:

```text
device transition
!= physical voice episode
!= musical note
!= persistent part
```

A physical channel is an execution coordinate, not a permanent musical identity.

## Enhanced source engines

Project-owned source-domain work currently includes:

- `sn76489_enhanced` for isolated high-quality PSG tone/noise stems;
- `ym2612_dac_enhanced` for classic YM2612 DAC playback;
- `ym2612_pcm_stream` for modern source-bank PCM streams with source-rate reconstruction;
- exact YM2612 register timeline capture and an isolated six-channel FM backend contract;
- explicit authored stereo routing and high-precision source summation primitives.

These cores are not evidence that foobar playback has already switched to an audible enhanced renderer. Reference playback, source-state infrastructure, testable enhancement cores and retained listening wins are separate evidence states.

## Current FM frontier

The next major audible Genesis milestone remains a mature six-channel YM2612 renderer with:

- exact patch/operator semantics;
- exact register/control timing;
- isolated channel output before final stereo summation;
- reference-comparable behavior;
- reversible substitution behind an experimental path.

Do not use a simplistic approximation merely to make sound sooner.

Only after a mature source-faithful FM path is validated should selected hardware limitations be relaxed experimentally.

## Musical-analysis relationship

The VGM path now contributes more than chip telemetry. It can provide exact lower evidence for higher questions about programmed expression, persistent musical identity, texture, parts and whole-song structure.

But a register trace usually cannot prove the original driver track or composer-facing score. Higher claims remain source-relative and provenance-bearing.

The intended song-level analysis is therefore:

```text
exact VGM execution
        ↓
conservative source/performance evidence
        ↓
persistent-part / structure hypotheses
        ↓
synchronized listening-level account
```

not:

```text
VGM -> MIDI -> canonical song model
```

## Realtime law

Normal foobar2000 playback remains realtime. Do not require offline song compilation, stem export or whole-song analysis before audio begins.

Offline/forensic/song-level analysis may operate over captured evidence separately.

## Legacy handlers

The imported wrapper also contains GYM, DRO and S98 compatibility paths. Keep them working where practical, but VGM/VGZ remains the active trace-format design center unless explicitly redirected.
