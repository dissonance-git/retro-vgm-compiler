# Execution-derived sequence topology

## Status

Cross-platform research input for sequence/driver execution, track identity, adaptive music, and export semantics.

## Question

Can Game Music Interpreter assume that a sequence file contains a fixed list of tracks which merely emit events over time?

No.

Across several unrelated game-audio systems, the stronger model is:

```text
stored sequence/program
+ runtime state
+ control flow
-> execution contexts
-> logical musical activity
```

In some formats the topology of those execution contexts is itself created, replaced, or destroyed during playback.

Therefore:

```text
source file track list
!= universal runtime track topology
```

and more generally:

```text
stored symbolic structure
!= complete performed musical structure
```

## 1. GameCube JAudio / BMS

### `arookas/flaaffy`

Pinned observatory should be recorded by exact commit when implementation work begins. The current source is used here as a research input.

`flaaffy`'s `cotton` documentation treats BMS as an assembler language corresponding closely to the binary representation rather than as MIDI data.

A BMS track has:

- registers and arithmetic;
- comparisons;
- conditional/unconditional branches;
- calls and returns;
- loops;
- timed parameters;
- ports and CPU callbacks;
- note channels;
- child-track state.

Most importantly, track topology is executable.

A track can issue:

```text
opentrack index, address
opentrackbros index, address
closetrack index
finish
```

A track may have up to sixteen children, the hierarchy may continue to arbitrary depth, and closing a parent recursively closes its children.

While a parent track is suspended by `wait`, child tracks continue to be processed recursively.

Tempo/time-base state is inherited by child tracks until overridden.

This means the runtime object is closer to:

```text
sequence program
-> root execution context
-> dynamically opened child contexts
-> independently advancing local state
-> notes/parameters/communication
```

than to a fixed SMF-style track array.

Source:

- https://github.com/arookas/flaaffy

### Independent projection pressure: `derselbst/BMS_DEC`

`BMS_DEC` is an older independent converter which discovers additional BMS tracks while parsing and emits them as MIDI tracks. Its source also documents uncertainty around track/channel counts and several native control semantics.

Source:

- https://github.com/derselbst/BMS_DEC

The value of this implementation is not that its MIDI output is canonical. It demonstrates the representational pressure created when an execution-derived hierarchy is flattened into a static MIDI track list.

### HCS64 history

The long-running BMS reverse-engineering discussion independently describes BMS as code interpreted directly from memory, with concurrently executing sequence contexts rather than one static MIDI-like event table.

Source:

- https://hcs64.com/mboard/forum.php?showthread=37639

Community terminology and early opcode meanings remain historical evidence, not automatic ground truth. The modern assembler/runtime evidence above is stronger for the structural claim.

## 2. Nintendo DS SSEQ

The existing Nintendo DS research already established that SSEQ contains program-like control mechanisms including:

- open-track commands;
- jumps and calls;
- loops and returns;
- variables;
- random values;
- conditional operations;
- note-wait/tie state;
- runtime parameter changes.

Therefore an SSEQ parser that reports only its statically visible note events has already chosen one projection.

With NCSF and 2SF the repository will have two useful observation routes:

```text
NCSF -> structured SDAT/SSEQ program
2SF  -> executable Nintendo DS runtime
```

A future same-work comparison should compare the runtime contexts actually created by the SSEQ interpreter, not merely count static SSEQ references.

## 3. Nintendo 64 Music Macro Language

`sauraen/seq64` documents first-party Nintendo 64 Music Macro Language as a hybrid musical/programming language containing:

- notes and instrument changes;
- loops;
- branches;
- calls;
- memory I/O;
- variables and technical control instructions.

For Ocarina of Time and related engines, sequence/channel/layer execution is not reducible to one flat MIDI event list.

The first sequence in some games can itself be a sound-effects program which reacts to game-engine messages.

SEQ64's byte-exact binary -> assembly -> binary round-trip for several first-party N64 sequence programs is especially useful because it distinguishes a native structural representation from a MIDI projection.

Source:

- https://github.com/sauraen/seq64

## 4. GBA MP2K

MP2K is structurally simpler than BMS in some respects, but its `MEMACC` command can update shared memory and perform conditional branches based on runtime values.

Thus even when the source has a conventional-looking multitrack structure:

```text
sequence bytes
!= one uniquely determined event trajectory
```

without the relevant runtime memory state.

See:

- `research/gba-mp2k-execution-and-rendering.md`
- https://github.com/loveemu/vgmdocs/blob/master/Summary_of_GBA_Standard_Sound_Driver_MusicPlayer2000.md

## 5. Static track identity, execution-context identity, and musical-part identity are different

The project should keep at least these concepts separable:

```text
source region / program point
runtime execution context
logical note/event producer
physical synthesis allocation
persistent musical part
perceptual stream/layer
```

They may correspond one-to-one in a simple source, but no such correspondence is universal.

Examples:

- one BMS root program can open many child contexts;
- a child context can replace a previous context at the same child index;
- one N64 sequence can contain technical logic or SFX programs rather than one musical part;
- one SSEQ path can open tracks only under runtime conditions;
- one logical track can allocate many changing physical voices;
- multiple logical sources can perceptually fuse into one textural layer.

Therefore do not assign persistent part identity from a static source-track number alone.

## 6. Existing graph pressure test

The current `model/musical_execution_graph.h` already has the needed neutral objects and relations:

```text
logical_process
program_point
execution_trace
trace_event

schedules
instantiates
control_flows_to
causes
```

and time-bounded edges.

No core graph change is earned by this pass.

A BMS adapter can represent, for example:

```text
source_object BMS
-> program_point OPEN_TRACK
-> schedules / instantiates logical_process child-context
-> execution_trace
-> trace_event note/control activity
```

with the process lifetime bounded by open/close/finish execution.

This is preferable to adding a universal `track` primitive whose semantics would immediately be wrong for other drivers.

## 7. Adaptive-music connection

Research on adaptive game music commonly distinguishes strategies such as:

- horizontal resequencing;
- vertical layering;
- dynamic mixing;
- game-state-conditioned transitions.

The low-level evidence here supplies an executable substrate beneath those musical descriptions.

A future analysis may legitimately derive:

```text
runtime branch/layer graph
-> adaptive musical structure hypothesis
```

when the relationship is supported.

But the theoretical category must not overwrite the exact driver mechanism.

For example, a BMS child-track spawn is first an exact/derived execution event. Calling it `vertical layering` is a higher musical interpretation which requires additional evidence about the role of that child context.

Scholarly observatories include:

- Patrick Hutchings & Jon McCormack, `Adaptive Music Composition for Games`, IEEE Transactions on Games (2019/2020)
- Adam J. Sporka & Jan Valta, `Design and implementation of a non-linear symphonic soundtrack of a video game` (2017)

## 8. Export consequence

A projection to MIDI should answer separately:

```text
which execution path was observed?
which runtime contexts were created?
how were those contexts mapped to MIDI tracks?
were alternate/reachable paths omitted?
were callbacks/ports/game-state inputs fixed or discarded?
was control flow executed before export or represented structurally?
```

A successful MIDI file is not evidence that the original source had the same track topology.

## 9. Highest-information future tests

### BMS synthetic topology control

When a BMS/JAudio adapter becomes useful, construct a minimal sequence that:

1. root opens child A;
2. root waits while child A continues;
3. child A opens a child or sibling;
4. root replaces child A at the same index;
5. each context emits distinguishable note/control events;
6. one context communicates via a port/callback where a bounded implementation supports it.

Expected result:

- source addresses remain stable source coordinates;
- runtime process identities remain distinct across replacement;
- process lifetimes are explicit;
- emitted notes retain process provenance;
- no physical-voice or persistent-part identity is inferred merely from child index.

### DS 2SF/NCSF paired control

For Mario Kart DS, compare:

```text
statically referenced SSEQ structure
versus
runtime-opened track contexts
```

when execution becomes available.

### Ocarina USF / SEQ64 control

For one Ocarina sequence, preserve both:

```text
native MML control-flow graph
runtime sequence/channel/layer trace
```

before projecting to conventional note tracks.

## Stop conditions

Stop rather than overclaim if:

- a fixed file offset is automatically called a persistent track;
- a BMS child index is treated as stable process identity across close/reopen;
- a source-level track is equated with one physical voice;
- a one-path execution trace is called the complete adaptive composition;
- a MIDI track number is back-projected as canonical source identity;
- game callbacks, ports, variables, random values, or runtime memory are discarded while the result is called a lossless sequence representation;
- a modern adaptive-music category is used as a substitute for the exact driver mechanism.

Correction outranks coherence.
