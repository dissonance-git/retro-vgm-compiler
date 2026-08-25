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

This note pins the execution core and hook boundary used for the forensic SPC path. The core is a source of device facts only. It must not receive catalog metadata, ID666 data, external tags, composer labels, or attribution hypotheses.

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
snes_spc/SNES_SPC_misc.cpp
snes_spc/SPC_DSP.h
snes_spc/SPC_DSP.cpp
```

The core declares:

```text
sample_rate       = 32000
clock_rate        = 1024000
clocks_per_sample = 32
```

Runtime instrumentation therefore uses the 1,024,000-clock SPC basis. Reducing observation time to 32 kHz sample boundaries would erase DSP phase ordering that this experiment specifically needs.

## Why the accurate DSP

The accurate `SPC_DSP` schedules internal DSP work across 32 hardware phases. That gives explicit observation points for facts that a player-oriented per-sample abstraction can collapse together.

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

## Controlled-load boundary

The raw SPC fixture remains the sole initial-state evidence object, but `SNES_SPC::load_spc()` performs deterministic machine setup after copying those bytes. In particular, the saved control state may make the IPL ROM visible at `$FFC0-$FFFF` before controlled execution begins.

Therefore the forensic sequence is:

```text
SNES_SPC::init()
        ↓
attach metadata-blind runtime sink
        ↓
SNES_SPC::load_spc(exact fixture bytes)
        ↓
record deterministic load-time visible-RAM transitions
        ↓
controlled execution
```

Attaching only after `load_spc()` would lose an initial IPL visibility mutation and make replay begin from the wrong visible high-RAM state for affected fixtures.

The sink still receives no ID666 text or catalog metadata. It observes only the memory/device effects produced while loading the exact fixture.

## Causal host-order requirement

Normal playback may let the SPC700 execute ahead of the DSP and catch the DSP up lazily. That host scheduling is acceptable for audio output but not for one ordered forensic trace: a later CPU RAM callback could otherwise arrive before an older DSP phase callback.

A broad upstream switch is deliberately **not** used to solve this. `SPC_MORE_ACCURACY` includes behavior beyond host callback synchronization, including an upstream probabilistic timer-glitch model. Importing that unrelated stochastic validation behavior would make deterministic corpus replay harder to defend.

The isolated forensic build therefore uses:

```text
SPC_LESS_ACCURATE=0
SPC_MORE_ACCURACY=0
RETRO_VGM_SPC_FORENSIC_ORDERING=1
```

`RETRO_VGM_SPC_FORENSIC_ORDERING` is a local exact-sentinel patch to `MEM_ACCESS`. It catches the already-accurate DSP up before SPC700 memory accesses only when `time > m.dsp_time`, using the normal accurate `RUN_DSP` path. It does not enable upstream `SPC_MORE_ACCURACY` timer/glitch behavior.

This is an observation-order constraint, not a new musical assumption: callback order must preserve the hardware causal order already represented by the emulator.

The default VGM Compiler build does not enable or fetch this dependency. The instrumented core lives behind the standalone `tools/spc/forensic/` CMake entrypoint.

## APURAM mutation coverage

Event-time BRR identity is only exact if every APURAM mutation visible to the DSP advances the RAM generation model.

### 1. SPC700 CPU-visible RAM writes

`SNES_SPC::cpu_write(...)` is the ordinary CPU write funnel. The observation describes the final bytes visible in APURAM after the source mutation has completed.

High-memory writes require care because IPL ROM visibility can make the transient CPU write differ from the final DSP-visible byte.

Origin:

```text
spc700_cpu
```

### 2. IPL ROM visibility transitions

`SNES_SPC::enable_rom(...)` copies 64 bytes at `$FFC0-$FFFF` between the visible RAM window and IPL/hidden high-RAM state.

The visible 64-byte replacement is represented as one mutation boundary when the overlay actually changes bytes.

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

Ignoring echo writes would make the claim `exact event-time RAM version` false.

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

The first instrumentation pass prioritizes exact lifecycle/sample facts over broad controller telemetry.

### `key_on_accepted`

Emit at the accurate DSP phase where a voice actually consumes a pending KON and enters the KON setup delay, not when software writes register `$4C`.

### `sample_phase_started`

Emit when the voice's BRR address is actually initialized from the resolved directory pointer at the beginning of its KON setup sequence.

This is the strongest point for event-time BRR materialization.

### source identity

The source-number/directory-address pipeline is staggered. Do not label a direct instantaneous read of `SRCN` as the source that necessarily produced the current BRR pointer.

The `sample_phase_started` hook reports the source identity associated with the directory entry that actually produced the active BRR address. A duplicate decorative `source_latched` event is unnecessary for this first pass.

### `release_entered`

Emit once on transition into release, including KOFF-driven transition. Do not emit once per sample merely because KOFF remains set.

### `became_inactive`

Emit when envelope state actually reaches inactive/zero, and handle immediate-stop/reset paths explicitly rather than inferring them from later silence.

### `execution_reset`

Reset/soft-reset is a hard semantic continuity boundary. It closes live voice episodes and cannot be healed later by sample/pitch similarity.

## State-serialization exclusion

The forensic target is compiled with:

```text
SPC_NO_COPY_STATE_FUNCS=1
```

The upstream `copy_state()` implementation temporarily toggles IPL-ROM visibility while serializing RAM and then restores register state. Those operations are serialization machinery, not hardware-time mutations in the controlled musical execution.

Rather than invent observer-suppression semantics for a feature the experiment does not need, save/load-state support is excluded from the forensic target. The upstream state serializer remains untouched and the ordinary project build is unaffected.

## Helix-owned boundary

Producer-facing interface:

```text
components/spc/spc_runtime_instrumentation_sink.h
```

Project-owned vendor bridge:

```text
components/spc/snes_spc_runtime_hook_bridge.h
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

Pinned exact-sentinel transformers:

```text
tools/spc/patch_snes_spc_runtime.py
tools/spc/patch_snes_spc_runtime_strict.py
tools/spc/patch_snes_spc_forensic.py
```

The first two define and strictly verify the runtime hook surface. `patch_snes_spc_forensic.py` composes those hooks with the narrower deterministic host-order patch required by the research build.

Standalone forensic build and sidecar runner:

```text
tools/spc/forensic/CMakeLists.txt
tools/spc/forensic/spc_forensic_features.cpp
```

The current sidecar patch-contract identifier is:

```text
retro-vgm-compiler:snes-spc-runtime-hooks-v1-ordering-v1
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
one monotonic observation clock contract
```

A vendor callback cannot choose its own RAM generation or trace ordinal.

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

The emitted `spc_forensic_features` JSON contains implementation provenance, capture/replay diagnostics, and musical geometry. It deliberately does not emit title, game, artist, composer, ID666, or external-tag fields. Sidecar filenames may be used as routing identities after extraction.

## First calibration surface

The manual `.github/workflows/spc-forensic.yml` first runs the transformer-integrity regression and direct accurate-DSP bridge regression. Only after those succeed does it execute six real SPC fixtures and upload their creator-blind sidecars.

The six are selected downstream from existing authoritative external-tag routing so both Cube candidates have controls in more than one soundtrack:

```text
Ancient Magic 01  → Hikichi route
Ancient Magic 04  → Takaoka route
Ancient Magic 10  → Hikichi route
Terranigma 03     → Takaoka route
Terranigma 06     → Takaoka route
Terranigma 08     → Hikichi route
```

Those names/routes determine which immutable fixtures are executed first. They are not arguments to the feature extractor and are absent from its JSON geometry.

## Acceptance tests before real corpus claims

Do not promote the instrumentation to real-corpus evidence until all are true:

1. identical SPC + identical controlled run produces byte-identical trace facts;
2. a RAM rewrite at the same BRR address produces a distinct runtime sample identity;
3. DSP echo writes advance RAM generation and can invalidate an overlapping BRR version;
4. IPL overlay changes are visible as high-RAM mutations, including deterministic load-time overlay;
5. KON register write and accepted voice start remain distinct events;
6. dropped capture observations create a hard continuity barrier;
7. poisoned title/game/composer/ID666/GD3/external-tag nodes do not change extracted motif geometry;
8. the pinned upstream core and local hook/ordering contract are recorded in provenance for every generated sidecar;
9. CPU RAM callbacks and DSP callbacks remain monotonic under the narrow forensic memory-access synchronization;
10. the direct accurate-DSP bridge regression compiles and executes against the actually patched pinned core;
11. the forensic target remains deterministic and does not opt into the broad stochastic `SPC_MORE_ACCURACY` path.

Until those tests run against an actual instrumented core, the SPC corpus path is architecturally connected and workflow-ready but not yet experimentally executed.
