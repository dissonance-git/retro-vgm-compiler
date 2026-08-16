# Accurate SPC runtime instrumentation boundary

## Purpose

The SPC attribution/corpus path needs controlled runtime evidence that survives the distinction between:

```text
SPC file snapshot
!= DSP register write log
!= accepted voice execution
!= event-time BRR memory identity
!= persistent musical part
```

This note pins the execution core and hook boundary used for the next implementation step. The core is a source of device facts only. It must not receive catalog metadata, ID666 data, external tags, composer labels, or attribution hypotheses.

## Pinned execution core

Repository:

```text
https://github.com/blarggs-audio-libraries/snes_spc
```

Pinned commit:

```text
ec8ee2bbe30451614c1d02a83f7af1c97d497d45
```

That commit is the repository's v0.9.0 import. It is intentionally pinned rather than followed by branch name because exact runtime behavior matters to evidence provenance.

License: LGPL-2.1-or-later in the upstream source headers/license.

Primary files:

```text
snes_spc/SNES_SPC.h
snes_spc/SNES_SPC.cpp
snes_spc/SPC_DSP.h
snes_spc/SPC_DSP.cpp
```

The core declares:

```text
sample_rate       = 32000
clock_rate        = 1024000
clocks_per_sample = 32
```

Runtime instrumentation should therefore use the 1,024,000-clock SPC basis. Reducing observation time to 32 kHz sample boundaries would erase DSP phase ordering that this experiment specifically needs.

## Why the accurate DSP

The accurate `SPC_DSP` schedules the internal DSP work across 32 hardware phases. That gives explicit observation points for facts that a player-oriented per-sample abstraction can collapse together.

In particular:

```text
software writes KON
        ↓
DSP reaches KON processing phase
        ↓
voice accepts key-on / kon_delay begins
        ↓
source-directory pipeline resolves BRR pointer
        ↓
BRR start address becomes active
```

These are not one event.

A `KON` register write therefore must not be emitted directly as `key_on_accepted`.

## APURAM mutation coverage

Event-time BRR identity is only exact if every APURAM mutation visible to the DSP advances the RAM generation model.

### 1. SPC700 CPU-visible RAM writes

`SNES_SPC::cpu_write(...)` is the ordinary CPU write funnel. The observation must describe the final bytes visible in APURAM after the source mutation has completed.

High-memory writes require care because IPL ROM visibility can make the transient CPU write differ from the final DSP-visible byte.

Origin:

```text
spc700_cpu
```

### 2. IPL ROM visibility transitions

`SNES_SPC::enable_rom(...)` copies 64 bytes at `$FFC0-$FFFF` between the visible RAM window and IPL/hidden high-RAM state.

The visible 64-byte replacement must be represented as one mutation boundary when the overlay changes.

Origin:

```text
ipl_rom_overlay
```

### 3. DSP echo writes

The accurate DSP writes echo feedback directly into the same shared 64 KiB APURAM. These writes can alter bytes that later become sample data, including pathological overlap cases.

Both left/right 16-bit echo writes therefore belong in the RAM trace.

Origin:

```text
dsp_echo
```

Ignoring echo writes would make the claim "exact event-time RAM version" false.

## Voice lifecycle hook points

The existing Helix capture vocabulary is:

```text
key_on_accepted
sample_phase_started
release_entered
became_inactive
source_latched
continuation_lost
execution_reset
routing_state_changed
```

The first instrumentation pass should prioritize exact lifecycle/sample facts over broad controller telemetry.

### `key_on_accepted`

Emit at the accurate DSP phase where a voice actually consumes a pending KON and enters the KON setup delay, not when software writes register `$4C`.

### `sample_phase_started`

Emit when the voice's BRR address is actually initialized from the resolved directory pointer at the beginning of its KON setup sequence.

This is the strongest point for event-time BRR materialization.

### `source_latched`

The source-number/directory-address pipeline is staggered. Do not label a direct instantaneous read of `SRCN` as the source that necessarily produced the current BRR pointer.

The hook must report the source identity associated with the directory entry that actually produced the active BRR address.

### `release_entered`

Emit once on transition into release, including KOFF-driven transition. Do not emit once per sample merely because KOFF remains set.

### `became_inactive`

Emit when envelope state actually reaches inactive/zero, and handle immediate-stop/reset paths explicitly rather than inferring them from later silence.

### `execution_reset`

Reset/soft-reset is a hard semantic continuity boundary. It must close live voice episodes and cannot be healed later by sample/pitch similarity.

## Helix-owned boundary

Producer-facing interface:

```text
components/spc/spc_runtime_instrumentation_sink.h
```

Offline recorder:

```text
components/spc/spc_runtime_trace_recorder.h
```

Trace data contract:

```text
components/spc/spc_runtime_trace.h
```

Replay:

```text
components/spc/spc_runtime_trace_replay.h
```

The vendor core reports only:

```text
exact APURAM mutation
    origin
    SPC-clock time
    address
    post-mutation bytes

exact DSP voice observation
    SPC-clock time
    device fields available at that phase
```

The recorder owns:

```text
RAM write serial
trace index
capture-window overflow accounting
```

The emulator must not invent these clocks itself.

## Downstream firewall

The runtime chain remains:

```text
exact SPC snapshot
→ instrumented SPC700 + accurate S-DSP
→ lossless trace
→ deterministic replay
→ bounded physical voice episodes
→ conservative persistent-part recovery
→ tempo/transposition-tolerant motif profiles
→ creator-blind cue geometry
→ blind attribution controls
→ only then external-tag / evidence-role join
```

ID666 and other embedded tags are outside this chain. External tags are authoritative for catalog identity when present, but they also remain outside feature extraction and are joined only after the creator-blind geometry exists.

## Acceptance tests before real corpus claims

Do not promote the instrumentation to real-corpus evidence until all are true:

1. identical SPC + identical controlled run produces byte-identical trace facts;
2. a RAM rewrite at the same BRR address produces a distinct runtime sample identity;
3. DSP echo writes advance RAM generation and can invalidate an overlapping BRR version;
4. IPL overlay changes are visible as high-RAM mutations;
5. KON register write and accepted voice start remain distinct events;
6. dropped capture observations create a hard continuity barrier;
7. poisoned title/game/composer/ID666/GD3/external-tag nodes do not change extracted motif geometry;
8. the pinned upstream core and local hook patch are recorded in provenance for every generated trace.

Until those tests run against an actual instrumented core, the SPC corpus path is architecturally connected but not yet experimentally executed.
