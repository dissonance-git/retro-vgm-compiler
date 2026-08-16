# Gain, envelope, and perceived-loudness semantics

## Status

Cross-device research input for amplitude trajectories, dynamics, instrument realization, and human-facing descriptions of loudness.

## Central correction

A parameter called `volume`, `level`, `gain`, or `TL` is not universally an instantaneous acoustic amplitude, and acoustic amplitude is not perceived loudness.

```text
programmed level/control
-> device-local envelope/gain state
-> source amplitude trajectory
-> mixer/effects/acoustic signal
-> perceived loudness hypothesis
-> musical dynamic description
```

Therefore:

```text
volume register != amplitude trajectory != loudness != musical dynamics
```

## Yamaha OPN2: TL is operator attenuation

Pinned observatory:

- `nukeykt/Nuked-OPN2`
- commit `335747d78cb0abbc3b55b004e62dad9763140115`

Nuked OPN2 folds operator Total Level into the evolving envelope-attenuation path alongside envelope state and LFO amplitude modulation. TL is therefore not a channel-output amplitude.

A modulator TL change can substantially alter timbre without producing a proportional loudness change.

```text
TL + envelope + AM state
-> operator attenuation
-> FM algorithm
-> channel waveform
```

See also `research/fm-operator-trajectory-semantics.md`.

## C352: programmed gain is a target

Pinned observatory:

- `mamedev/mame`
- commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `src/devices/sound/c352.cpp`

C352 keeps programmed front/rear gain bytes separately from four `curr_vol` values used by the mixer. `ramp_volume()` autonomously moves the current values toward their programmed targets.

Thus:

```text
written gain at t0 != realized gain immediately at t0
```

and realized amplitude can continue changing with no new gain write.

## PlayStation SPU: volume can be an autonomous sweep

Pinned observatory:

- `stenzek/duckstation`
- commit `5fd366809053fe287291de7a39752c4d5d5b146b`
- `src/core/spu.cpp`

The SPU left/right volume-register family can encode either a fixed signed level or sweep mode. Sweep mode carries rate, direction, linear/exponential behavior, and phase-negative state. DuckStation therefore keeps a separate time-bearing `VolumeSweep`/`VolumeEnvelope` current level.

The voice also has a distinct ADSR envelope.

```text
volume-register write
-> autonomous L/R sweep trajectory

ADSR register state
-> source envelope trajectory
```

Those two processes must not be flattened into one static `volume` number.

## AY-3-8910 / YM2149: level codes are nonlinear

Pinned observatories:

- `mamedev/mame` commit `1e1b6ce78a1645805bb5eef4049e3f1d3f926194`
- `openMSX/openMSX` commit `0d462ce723c0850940c348a348d96908c9ea7ad1`

AY/YM fixed-volume codes are mapped through nonlinear/logarithmic and measurement-informed electrical transfer functions. The same register can also select the shared envelope generator instead of fixed level.

Therefore numeric register increments are not proportional acoustic-amplitude increments.

## Programmed envelope and realized envelope are different coordinates

Across these devices:

```text
programmed control
-> device state machine
-> realized gain/attenuation trajectory
```

Examples:

```text
OPN2 ADSR/TL -> operator attenuation trajectory
PS1 sweep parameters -> L/R current-level trajectory
C352 target gain -> ramped four-bus gain trajectory
AY envelope mode -> shared envelope level through output transfer
```

Recover autonomous device state rather than inventing interpolation between register writes.

## Gain trajectory is not final acoustic level

Final level can additionally depend on:

- source waveform amplitude;
- FM topology;
- simultaneous voices;
- nonlinear mixing;
- shared feedback/effects;
- signed/phase-inverted routing;
- filtering and board/output stages.

See:

- `research/nonlinear-device-mixing-semantics.md`
- `research/shared-feedback-dsp-state.md`
- `research/native-routing-and-perceived-position.md`

Thus a voice-gain increase need not produce a proportional final-mix increase.

## Acoustic level is not perceived loudness

Psychoacoustic literature shows that loudness depends on level but also frequency/spectrum, duration, temporal integration, context, and listener factors.

Useful literature surfaced in the SciSpace pass includes:

- Florentine, Buus & Poulsen, `Temporal integration of loudness as a function of level`;
- Buus, Florentine & Poulsen, `Temporal integration of loudness, loudness discrimination, and the form of the loudness function`;
- research on complex time-varying loudness and spectro-temporal loudness effects.

These sources establish the perceptual boundary, not console register semantics.

Do not infer absolute SPL, sones, or phons from normalized emulator samples without a calibrated acoustic path.

## Musical dynamics are higher again

Natural descriptions such as:

```text
gets louder
swells
backs off
fades into the background
hits harder
```

should descend through an evidence bundle rather than a raw register number.

A conservative `swells` claim might require:

```text
persistent part identity
+ increasing realized gain/acoustic energy over time
+ compatible perceptual evidence
```

Prominence is also not identical to loudness. Spectral separation, spatial separation, transient change, or competing parts dropping out can make a part more prominent without a large level increase.

## Highest-information regressions

### C352

Write one target gain and issue no further gain writes. Verify the current gain continues moving until the target is reached.

### PlayStation SPU

Compare fixed-volume and sweep-mode voices with identical source/ADSR state. Verify sweep mode develops a time-bearing L/R trajectory without further volume writes.

### OPN2

Change TL on a principally modulating operator while preserving carrier TL/base pitch. Verify the result is not represented merely as a channel-gain event.

### AY/YM

Compare level-code increments against the reference transfer table and verify the realized amplitude increments are nonlinear.

## Common-model pressure test

No new common graph primitive is earned. Preserve programmed controls and autonomous realized state separately where they differ.

A generic normalized `volume` field is a projection unless exact native semantics remain underneath it.

## Stop conditions

Stop rather than overclaim if:

- Yamaha TL is treated as instantaneous channel amplitude;
- C352 target gain is treated as current gain at the write tick;
- PS1 sweep mode is flattened into one fixed level;
- AY volume codes are mapped linearly without evidence;
- normalized digital amplitude is called perceived loudness directly;
- a raw level-register increase is narrated as a crescendo without downstream support;
- perceived prominence is equated automatically with loudness.

Correction outranks coherence.
