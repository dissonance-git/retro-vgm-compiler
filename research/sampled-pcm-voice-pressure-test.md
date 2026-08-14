# Sampled PCM voice pressure test

## Question

Does the lower-level `sampled physical voice episode` shape discovered in the C140/C352 pass survive materially different PCM devices, or was it merely a Namco-specific coincidence?

This pass pressure-tests the candidate against:

- SegaPCM
- Yamaha YMW258 / MultiPCM
- Ricoh RF5C68/RF5C164 family behavior
- OKI MSM6295
- Capcom QSound

Primary implementation observatory:

- ValleyBell/libvgm `61fc6725644886abc3168e240e4e51588d74bdf7`

Independent documentation/research was also checked for MultiPCM, MSM6295, and QSound behavior.

## Starting candidate

The C140/C352 pass proposed:

```text
sampled physical voice episode
=
exact device/source identity
+ physical slot
+ generation/lifetime
+ sample-address identity
+ playback-rate trajectory
+ decode mode
+ loop/direction trajectory where supported
+ gain/routing trajectory
```

The pressure test changes this in several important ways.

## SegaPCM

SegaPCM exposes 16 PCM channels with:

- left/right gain
- loop address
- end address
- address delta
- current address
- bank/control bits
- loop-disable/channel-disable state

The renderer runs at `clock / 128`. Each output step advances the 24-bit address by the channel's 8-bit delta, and the fetched ROM byte is selected from the bank plus the high address bits.

### Correction: activation is not universally `key-on`

SegaPCM does not expose a C352/MultiPCM-style key-on command in the same semantic shape. Channel lifetime is controlled through enable/disable and address state.

Therefore the common field must be:

```text
activation/lifetime generation
```

not:

```text
key-on generation
```

The device-specific adapter may define a generation boundary using key-on, enable transition, address reload, or another source-supported activation event.

## MultiPCM

MultiPCM exposes 28 physical slots and immediately strengthens the candidate model.

Per-slot state includes:

- pan
- sample index
- octave
- pitch coordinate
- key-on/key-off
- total level and level-ramp behavior
- LFO frequency
- vibrato depth
- tremolo depth
- envelope state
- playback offset/step
- reverse state

More importantly, a sample index refers to a ROM metadata table. The table carries structured sample information including:

- sample start
- loop point
- sample length/end
- sample format
- attack
- decay 1
- decay 2
- decay level
- release
- key-rate scaling
- default pitch-LFO settings
- default amplitude-LFO settings

Writing the sample register loads that structured descriptor and copies some descriptor values into live slot controls.

### Correction: sample identity is not necessarily an address

The common object therefore needs a more general source descriptor:

```text
sample/source descriptor identity
```

which may resolve to:

```text
direct address region
or
sample-table entry -> address/loop/format/envelope metadata
or
another source-specific indirection
```

Do not flatten the descriptor to a start address and discard how the device reached it.

### MultiPCM also raises the semantic ceiling

Its envelope and LFO state are still exact device evidence, but they are closer to performed gesture than a bare PCM reader.

They remain below musical interpretation:

```text
device envelope != articulation label
pitch LFO != vibrato interpretation without context
sample-table entry != instrument identity
```

## RF5C68 / RF5C164 family

The RF5C68-style core exposes eight channels with:

- channel enable
- envelope/gain
- pan
- start page
- live address
- 16-bit step
- loop start
- shared chip enable
- sample RAM

The renderer runs at `clock / 384` and advances the fractional address by the channel step.

A crucial difference is end-of-sample representation: sample data itself uses `0xFF` as an end marker. Hitting the marker jumps to the loop point; a loop point that also contains the marker effectively ends the voice.

### Correction: sample end is not universally a register

The common sampled-source description must preserve an end-condition type, for example:

```text
explicit end address
sentinel in sample memory
sample-table length
command-derived count
other device-specific termination rule
```

This is stronger than storing only `start/end/loop` integers.

RF5C68 also shows why mutable sample memory matters. Source identity may include RAM generation/version, not merely an address.

## MSM6295

MSM6295 exposes four ADPCM voices. A two-byte start command selects:

- sample/phrase number
- one target voice
- startup volume

A stop command can stop one or more voices.

The selected phrase resolves through the ROM phrase table and banking logic to a base offset and finite ADPCM sample count.

### Correction: per-voice pitch trajectory is optional

MSM6295 does not provide normal per-voice variable pitch. Playback rate is derived from chip clock and pin-7 divider state:

```text
sample_rate = master_clock / 132
or
sample_rate = master_clock / 165
```

Therefore the common episode must allow playback-rate evidence to be:

- time-varying per voice;
- constant per voice episode;
- inherited from time-varying device-global state;
- or absent/unknown.

Do not require a pitch register merely because other sampled chips have one.

The ADPCM decoder state is part of physical realization, not source identity.

## QSound

QSound provides a useful stress test because the device contains both sampled voices and a substantial downstream DSP field.

The current accurate core exposes:

- 16 PCM voices with bank, address, fractional phase, rate, loop length, end address, volume, and echo send;
- 3 ADPCM voices with start/end/bank/volume and decoder state;
- per-voice pan controls;
- shared echo state;
- FIR filter state;
- wet/dry delay state;
- DSP state-machine behavior.

For PCM voices, the source position advances from the per-voice rate and loops by subtracting loop length after reaching the end address.

### Correction: downstream DSP is not voice identity

The common voice episode should retain source-local routing parameters such as pan and echo send when they are directly attached to the voice.

It should NOT absorb the entire shared DSP state merely because that state affects the final sound.

Preserve the transformation boundary:

```text
sampled voice episode
-> voice-local routing/sends
-> shared DSP/acoustic realization
-> mixed output
```

Otherwise QSound would make every simultaneous voice appear to share one giant identity object.

## Revised common object

After C140, C352, SegaPCM, MultiPCM, RF5C68-family, MSM6295, and QSound, the strongest common lower-level object is now:

```text
sampled physical voice episode
=
exact device/source identity
+ physical slot
+ activation/lifetime generation
+ source descriptor identity
+ source-memory generation where mutable
+ traversal state
+ playback-rate evidence
+ decode mode/state class
+ end/loop/direction semantics
+ gain/routing/send trajectory
```

with device-specific optional fields underneath it.

### Source descriptor identity

This may be:

- a direct ROM/RAM address region;
- a bank + address region;
- a sample-table entry;
- a phrase number resolved through a table;
- another exact source-specific descriptor.

The resolution path is evidence and must not be discarded.

### Playback-rate evidence

This may be:

- a per-voice step/rate trajectory;
- octave + pitch coordinates transformed through a device law;
- an inherited device-global sample rate;
- fixed for the whole episode;
- unavailable.

`playback-rate evidence` is intentionally more general than `pitch`.

### End/loop semantics

This may be:

- explicit end + loop addresses;
- loop length;
- sentinel-terminated memory;
- table-derived sample count;
- reverse/bidirectional loop state;
- no looping.

Do not coerce these into one anonymous integer pair.

## What survived all pressure tests

The following claims survived:

```text
physical slot != persistent musical part
sample/source descriptor != instrument identity
playback-rate evidence != note spelling
same sample/source descriptor != same musical part
voice-local routing != shared DSP realization
```

The following tentative claims did not survive unchanged:

```text
key-on generation
-> activation/lifetime generation

sample-address identity
-> source descriptor identity + resolution path

mandatory playback-rate trajectory
-> optional/inherited playback-rate evidence

explicit sample end
-> typed end/loop semantics
```

These are productive failures. They make the common object smaller and more accurate.

## Corpus mapping

Existing permanent controls already cover all of these families:

- `outrun-segapcm`
- `title-fight-multipcm`
- `dark-wizard-rf5c164`
- `magical-drop-okim6295`
- `jyangokushi-qsound`
- `super-world-court-c140`
- `super-world-stadium-95-c352`
- `ponpoko-namco-wsg` as converted C352 provenance control

This means the next pass can become executable without collecting another soundtrack first.

## Next executable tests

1. Define a source-specific capture object for SegaPCM, MultiPCM, RF5C164, MSM6295, C140, C352, and QSound without moving it into `model/` yet.
2. Recover activation-bounded episodes from one fixture per corpus family.
3. Record source descriptor resolution rather than only resolved address ranges.
4. Preserve raw clock/step coordinates and derived rates separately.
5. Split voice-local routing from global/shared DSP state.
6. Compare whether the revised episode shape can be consumed by one analysis adapter without hiding device-specific semantics.
7. Only then decide whether a common `model/` representation has been earned.

## Stop conditions

Stop rather than guess if:

- activation boundaries are ambiguous;
- a sample/phrase table resolution depends on unavailable bank state;
- mutable RAM generation is unknown;
- a playback-rate coordinate cannot be tied to its clock convention;
- an envelope/LFO parameter is being renamed as a human articulation without evidence;
- shared DSP state is being attached to one physical voice as if it were source identity;
- a physical voice episode is being promoted to a persistent musical part by slot continuity alone.

Correction outranks coherence.
