# Sonic 3 VGM / Gun Hazard SPC cross-format case

Status: preliminary structural result  
Purpose: pressure-test `docs/MUSICAL_EXECUTION_MODEL.md` against two very different source representations  
Metadata policy: embedded descriptive metadata is ignored; only executable/machine state is relevant here

## Inputs

Two soundtrack collections supplied directly for analysis:

- `Sonic 3 & Knuckles.zip`
- `Front Mission ~ Gun Hazard.zip`

The source archives themselves are not committed to this repository.

## Sonic 3 & Knuckles: VGM execution trace

The supplied collection contains 58 `.vgz` objects.

Header-level facts:

- all 58 identify SN76489 clock `3,579,545 Hz`;
- all 58 identify YM2612 clock `7,670,453 Hz`;
- 57 use VGM version `0x150`;
- `Competition Menu` uses VGM version `0x110`.

The relevant source model is therefore primarily:

```text
sample-timed VGM commands
        ↓
YM2612 / SN76489 state
        ↓
FM operators / PSG / DAC
        ↓
reference acoustic render
```

The VGM log directly preserves device-facing execution but normally does not prove the original SMPS logical-track identity by itself.

The current repository already has the beginning of this adapter:

- exact YM2612 register state;
- sample-accurate YM2612 register timeline;
- six-channel FM backend contract;
- SN76489 state/rendering;
- classic YM2612 DAC source bytes;
- VGM source-bank PCM streams.

The next reasoning-layer step is to emit musical-performance objects from this exact execution without flattening FM/PSG/DAC semantics into MIDI.

## Front Mission: Gun Hazard: SPC machine snapshots

The supplied collection contains 61 SPC snapshots.

All 61 files:

- have the normal `SNES-SPC700 Sound File Data v0.30` signature;
- are 66,144 bytes;
- preserve the 64 KiB SPC700 RAM snapshot;
- preserve the S-DSP register image;
- have S-DSP `DIR = $20`, placing the sample directory at RAM `$2000` in every snapshot.

### Stable RAM

Comparing the 64 KiB RAM image byte-for-byte at the same address across all 61 songs:

- `27,173 / 65,536` byte positions are identical across every snapshot;
- identical fraction: approximately `0.414627`.

Large contiguous identical regions include:

- `$01FF-$132D`: 4,399 bytes;
- `$1361-$1E12`: 2,738 bytes;
- `$3A24-$5DE7`: 9,156 bytes;
- `$D7C2-$F81F`: 8,286 bytes.

These ranges prove stable shared memory content across the soundtrack. Their semantic roles must be established by execution/disassembly before labeling them as code, tables, samples, or another structure.

### Stable and moving BRR objects

Raw BRR objects reachable through the DSP source directory were hashed by content.

A strong result appears immediately:

- SRCN slots `0-10` contain the same terminating raw BRR object in all 61 snapshots.

But physical SRCN is not a stable identity for the higher dynamic sample bank.

Examples:

- one exact 6,939-byte BRR object appears in 28 tracks and occupies SRCN `32`, `33`, `34`, or `35` depending on the snapshot;
- one exact 4,050-byte BRR object appears in 25 tracks and occupies six different SRCN values across the collection;
- one exact 3,960-byte BRR object appears in 23 tracks and occupies eight different SRCN values.

Therefore:

```text
SRCN number
≠ persistent sample / instrument identity
```

Content identity survives relocation.

This is structurally similar to driver systems where one logical musical part may be assigned to different physical FM channels over time.

## Cross-format invariant

The first invariant survives two unrelated source architectures:

> **Physical execution coordinates must remain distinct from persistent musical identity.**

Examples:

```text
Genesis / GEMS
logical part
→ dynamically allocated FM channel

SNES / Gun Hazard
same BRR object
→ different SRCN slot between song snapshots

MIDI
instrument / part
→ potentially different MIDI channel
```

A common musical model should therefore maintain at least three identities when evidence permits:

1. source/driver identity;
2. physical synthesis identity;
3. persistent musical-object hypothesis.

Do not collapse them.

## Why SPC is especially useful to Helix

VGM and SPC expose complementary evidence.

```text
VGM
strong device-command timeline
weaker original-driver context

SPC
complete sound-machine snapshot
CPU/RAM/driver context survives
but the musical event stream may need execution to recover
```

A collection of SPC snapshots from one game is more informative than one SPC alone because cross-file comparison can separate stable engine/sample structures from song-specific state.

This suggests a general corpus operation:

```text
many snapshots from one engine
        ↓
shared-state analysis
        ↓
content identities / stable regions / moving allocations
        ↓
controlled execution
        ↓
higher-level driver and musical semantics
```

## Converter comparison

`spc2midi-tsuu` demonstrates a useful intermediate strategy: run an SPC simulator with a MIDI-oriented DSP model, observe the eight executing S-DSP voices, and derive note/pitch-bend/volume/pan/expression/sample/effect information even without first identifying the game's original music driver.

That is valuable prior art for VGM Tooling, but MIDI itself is not the target representation because it cannot preserve all source synthesis/effect semantics.

The intended hierarchy is:

```text
exact SPC execution
        ↓
S-DSP voice / BRR / envelope / routing state
        ↓
common musical events and trajectories
        ↓
optional MIDI projection
```

not:

```text
SPC → MIDI → reasoning
```

## Next tests

### Gun Hazard

1. Run each SPC under a validated SPC700/S-DSP executor.
2. Trace actual SRCN triggers instead of treating every directory entry as active.
3. Assign BRR content identities independent of SRCN.
4. Recover per-voice pitch/onset/release/envelope/pan/echo trajectories.
5. Compare those trajectories across songs and physical channel changes.
6. Test driver-level recovery against the stable RAM regions.

### Sonic 3

1. Use the existing live YM2612/SN76489 state core rather than an ad-hoc parser.
2. Build exact FM patch identities independent of physical channel.
3. Recover note-like pitch/onset trajectories while retaining continuous FM controls.
4. Identify PCM/DAC sample objects independently of trigger channel.
5. Compare prototype/final tracks at composition, instrument and realization layers.
6. Route the resulting musical objects into the Sonic 3 attribution subproject as technical evidence, not automatic authorship conclusions.

### libaural

Render controlled examples from both systems and ask libaural to infer auditory events/streams from audio alone.

Compare:

```text
known source/performance state
↔ libaural auditory organization
```

The mismatch is research data rather than an error to hide.
