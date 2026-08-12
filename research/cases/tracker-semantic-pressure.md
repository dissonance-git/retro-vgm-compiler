# Tracker/module semantic pressure pass

## Status

Research input for the source-relative analysis feature model. This pass does not add a tracker parser or a libopenmpt runtime dependency.

## Question

VGM and SPC currently expose lower execution/synthesis evidence while leaving several higher musical questions unknown or unavailable.

Tracker/module sources provide a useful opposite pressure:

> What changes when notes, instruments, patterns, channels, and effect commands are explicitly authored in the source?

The goal is to test whether one feature/evidence model can describe both source families without flattening either one.

## GitHub observatory: OpenMPT

Primary implementation inspected:

- `OpenMPT/openmpt`
- `soundlib/modcommand.h`
- libopenmpt pattern/cell introspection surfaces

OpenMPT's `ModCommand` explicitly models one pattern cell with separate fields for:

- note;
- instrument;
- volume command;
- volume value;
- effect command;
- effect parameter.

Its note vocabulary also distinguishes normal notes from source-level special operations such as:

- key off;
- note cut;
- fade;
- parameter-control operations.

Its effect vocabulary includes, among many others:

- arpeggio;
- portamento up/down;
- tone portamento;
- vibrato;
- tremolo;
- volume slides;
- position jump;
- pattern break;
- retrigger;
- speed;
- tempo;
- key off;
- fine vibrato;
- panning and panning slides;
- envelope-position control;
- note slides;
- sample offset and related playback controls.

OpenMPT also classifies effect commands by semantic role such as normal, global, volume, panning, and pitch, and distinguishes continuous commands from instantaneous ones.

## Immediate lesson

A tracker source can make an authored note command exact while leaving the realized performance trajectory dependent on execution state.

Example:

```text
pattern row says: C-5
        +
tone-portamento effect
        ↓
exact authored target/control command
        ↓
requires previous channel state + format semantics + tick execution
        ↓
realized pitch trajectory
```

Therefore:

```text
exact authored note command
!= exact instantaneous performed pitch
```

This is an important third-source control against treating `normalized_absolute_pitch` as one context-free feature.

## Channel identity

Tracker channels are semantically richer than hardware chip channels because the source explicitly organizes pattern cells by logical channel. But they still must not automatically become persistent musical parts.

A tracker channel may:

- contain rests or unrelated material over time;
- trigger samples/instruments whose physical playback resources are allocated elsewhere;
- participate in polyphonic or special format behavior;
- use effects that retain state across rows;
- be perceived as more than one auditory stream or fuse with other channels.

Thus:

```text
tracker logical channel
!= physical synthesis voice
!= persistent musical part
!= auditory stream
```

However, compared with a VGM register log, an explicit tracker channel is **stronger source evidence** for one logical authored/performance lane.

## Instrument identity

An instrument number written in a pattern cell exactly proves that an instrument command is present in the source.

It does not by itself prove every later synthesis fact:

- an empty instrument field may inherit prior channel state rather than mean "no instrument";
- an instrument may map notes to different samples;
- sample playback can be transposed or offset;
- envelope/effect state can change realization;
- module-format semantics differ.

Therefore distinguish:

```text
instrument command in this source cell
from
effective instrument state at this execution point
from
sample object actually rendered
from
persistent musical instrument/role identity
```

## Timing and control flow

Patterns and order lists are authored/static structure. One playback execution can still be altered by:

- position jumps;
- pattern breaks;
- speed/tempo changes;
- loops and format-specific effects;
- row/tick effect memory.

This agrees with the existing VGM Tooling split between:

```text
static program/control-flow structure
!= one realized execution trace
```

The tracker family therefore strengthens the common model rather than requiring a tracker-specific ontology.

## Literature pressure

The literature pass found relatively little peer-reviewed work focused specifically on tracker internals compared with source code/specifications, but the broader music-representation literature strongly supports the same separation.

Relevant families include:

- Dannenberg on music representation issues and levels;
- Vinet on physical, signal, symbolic, and knowledge representation levels;
- IEEE 1599 / MX multilayer music representation;
- work on dynamic/rearrangeable score representations and constrained multi-hierarchical musical structures;
- tracker-format specifications such as the XM specification.

The durable point is that symbolic/source commands and realized performance can be linked without being identical representations.

## Feature-state consequences

For one normal tracker cell containing a pitched note and an explicit instrument, a future source adapter may legitimately report:

```text
authored_note_pitch       present / exact / authored_program
logical_source_channel    present / exact / authored_program
pattern_index             present / exact / authored_program
row_index                 present / exact / authored_program
instrument_command        present / exact / authored_program
effect_command            present / exact / authored_program
```

But if that cell uses tone portamento:

```text
performed_absolute_pitch  unknown until execution state is applied
pitch_trajectory          unknown until execution state is applied
physical_voice_episode    unavailable from the static pattern alone
persistent_part_identity  unknown
auditory_stream_identity  unavailable from the symbolic cell alone
```

This is exactly why an analysis feature must carry **claim layer + availability + evidence status + provenance** as separate dimensions.

## What this pass does not justify

Do not yet:

- add libopenmpt as a canonical dependency;
- copy OpenMPT's internal enums into the common model;
- make tracker channel equal `part`;
- make note column equal performed acoustic pitch;
- make empty instrument cell mean no instrument;
- normalize all tracker effects into MIDI controller messages;
- create a generic tracker parser before one concrete format/source path is selected.

## Next executable control

The current `analysis_feature` regression should include a tracker-shaped feature set proving that:

1. an authored note can be exact while a performance pitch remains unknown;
2. a logical tracker channel can be exact without becoming a `part`;
3. an exact tone-portamento command belongs to the authored/program layer while its pitch trajectory belongs to performance;
4. physical/acoustic/perceptual questions can remain unavailable rather than zero.

Once that survives, a concrete tracker format adapter can be added when the project needs one.
