# VGM Tooling

Executable understanding, analysis, and source-native rendering of digital game music.

This repository is the implementation home for the **VGM Tooling** project. `VGM` in the project name is historical shorthand for video game music tooling; the project is not limited to the `.vgm` file format.

The long-term objective is stronger than playback:

> **Understand supported digital game music deeply enough to run it as an explicit internal musical machine, inspect every meaningful layer while it runs, recover higher structure when the evidence permits, and render the same encoded work through both accurate/reference and higher-quality source-native paths.**

A supported object should not have to collapse immediately into MIDI, stems, or stereo PCM. Where the source permits it, the system should retain the route from authored or executable musical program to scheduling, synthesis, routing, rendered sound, and higher musical interpretation.

```text
authored source / executable music object
        ↓
program / control-flow structure
        ↓
execution trace / realized driver path
        ↓
musical events and control trajectories
        ↓
instrument definitions / synthesis objects
        ↓
running voices / physical device state
        ↓
routing / effects / signal graph
        ↓
reference or enhanced synthesis
        ↓
acoustic realization
        ↓
musical structure / auditory interpretation
        ↓
consumer / research projection
```

The foobar2000 components in this repository are important realtime frontends, not the definition of the project.

## Research method: many observatories, one phenomenon

VGM Tooling deliberately studies many mature systems because each exposes a different stratum of the same underlying process.

Examples include:

- VGM/VGZ and register-log tooling;
- SPC, NSF/NSFe, HES, KSS and PSF-family executable or snapshot formats;
- native game music drivers such as SMPS, GEMS, N-SPC, MDX and PMD;
- MML dialects and compilers;
- MIDI and hardware/module synthesis;
- trackers and module formats;
- MAME, Hoot, Game Music Emu, Modizer and other broad execution/player systems;
- VGMTrans, SPC-to-MIDI and VGM-to-MIDI recovery tools;
- DAWs and session systems;
- music21, Partitura, MEI and other symbolic/score representations;
- OpenMusic and related computer-assisted composition systems;
- SuperCollider, Csound, ChucK, Faust, Cmajor, Max/MSP/Pure Data and other audio programming environments;
- automatic transcription, score/audio alignment, source-separation and music-cognition literature.

These are **research observatories**, not an architecture to copy wholesale and not a dependency shopping list.

```text
one source exposes authored notation
one exposes driver execution
one exposes device state
one exposes synthesis graphs
one exposes routing and automation
one exposes musical structure
one exposes rendered audio
one exposes perceptual organization
        ↓
compare the strata
        ↓
identify distinctions that survive across systems
        ↓
implement the useful common mechanisms in VGM Tooling
        ↓
keep source-specific truth attached underneath
```

The project should go **below** convenient interfaces when they hide causality and **beyond** them when a broader representation is justified by several source families. It should not reinvent established concepts merely to rename them.

Research implementations remain conceptual/reference inputs unless their licenses and project boundaries explicitly permit reuse. New common-model mechanisms should be project-owned implementations. Imported upstream playback/reference code keeps its original provenance and license.

See:

- `docs/musical-execution-model.md`
- `docs/music-representation-systems.md`
- `docs/audio-programming-languages.md`
- `docs/upstreams.md`

## Levels of truth

Do not collapse these layers:

```text
SOURCE / AUTHORED REPRESENTATION
MML, score/pattern source, MIDI, native sequence/container, ROM-derived data,
VGM, SPC and other preserved objects

PROGRAM / CONTROL FLOW
patterns, loops, branches, macros, scheduler structure, driver program points,
legal transitions and adaptive/generative future behavior

DRIVER / SCHEDULER EXECUTION
execution traces, trace events, realized control flow, logical tracks,
note events, program changes, allocation, modulation, loops

MUSICAL PERFORMANCE
note-like events, continuous pitch, dynamics, articulation, persistent parts,
instrument relationships, authored routing/control trajectories

DEVICE / SYNTHESIS
YM2612, SN76489, S-DSP, QSound, OPL/OPN/OPM, PCM/ADPCM,
registers, operators, partials, envelopes, sample memory, effect state

MUSICAL STRUCTURE
meter, beat hierarchy, harmony, motif, phrase, section, form,
repetition and transformation relations when supported or inferred

ACOUSTIC REALIZATION
reference hardware behavior or source-native enhanced realization

AUDITORY INTERPRETATION
what a listener hears as events, streams, fields, rhythm, foreground/background,
masking, motion, environment and other perceptual organization
```

A physical chip channel is not automatically a persistent musical voice. A register log is not automatically the original score. A legal branch is not proof that a particular run took that branch. A trace event is not automatically a musical event. A MIDI export is not the internal truth. A perceptual stream is not automatically one physical source. Confidence, provenance, capture quality and source coordinates must survive transitions between layers.

## Current common model

The repository now contains a small provenance-aware musical execution graph in `model/musical_execution_graph.h` with tests integrated into the native core test path.

The current graph deliberately distinguishes:

```text
events       key-on, note, trigger, register, scheduler or trace event
values       patch, routing, configuration and persistent state
controls     time-varying parameters and executable control state
streams      PCM, oscillator/audio or execution-trace streams
graphs       synthesis, routing, effects, causal and program topology
objects      source objects, parts, instruments, voices, buffers, buses,
             logical processes, program points, execution traces and trace events
relations    causes, schedules, instantiates, realizes, occupies, routes,
             transforms, contributes, groups, repeats, projects,
             control-flow transitions and cross-domain time mappings
```

It keeps separate semantic and time domains for source representation, authored program, driver execution, synthesis, musical performance, musical structure, acoustic realization and auditory interpretation.

Three rules are now executable rather than merely documentary:

1. **static musical program structure is distinct from the path realized by one execution**;
2. **one realized run may be represented as an execution trace containing distinct trace events, including repeated visits to the same static program point**;
3. **authored, driver, device, sample and acoustic clocks remain distinct and are connected by explicit provenance-bearing mappings**.

A time span may not silently cross clock domains. Cross-domain correspondence is represented explicitly and can be piecewise when tempo, scheduling, resampling or alignment changes the relationship. Execution traces can be incomplete without turning missing observations into false claims about what did not happen.

This graph is **not finished architecture by declaration**. It is a minimal implementation that has survived the current comparison set. New abstractions should be added only when real source adapters or validation cases expose a missing distinction.

## The forward and inverse problem

VGM Tooling is unusual because it must operate in both directions.

Forward execution:

```text
authored musical program
        ↓
control-flow / scheduler / driver
        ↓
execution trace / realized performance path
        ↓
instrument and synthesis graph
        ↓
device execution
        ↓
audio
```

Inverse recovery:

```text
VGM / SPC / executable state / ROM / audio
        ↓
recover exact execution where possible
        ↓
recover the execution trace and program/control-flow structure where evidence permits
        ↓
recover synthesis objects and persistent identities
        ↓
recover musical events and structure with explicit evidence status
        ↓
reason about the music
```

Where both directions can be constructed independently, they become a powerful validation pair. Authored source can be compiled/executed forward and the resulting trace can be recovered back into the common model without pretending that every source retains the same information.

## Capability is source-relative

Different source families expose different depths of truth.

A tracker engine may expose patterns and instruments. A register log may expose exact hardware writes but no original score. A broad replay library may expose voices for one emulator and only mixed PCM for another.

Therefore:

```text
not exposed
≠ absent

unknown
≠ false

not applicable
≠ unavailable
```

VGM Tooling should not fabricate semantic parity merely because callers prefer a uniform shape. A permanent shared adapter-capability schema will be added only when concrete adapters require it.

## Project relationship

VGM Tooling is an independent implementation project connected to Helix, libaural, Omniphony, and downstream research without being absorbed by them.

```text
                         Helix
             project state / research / tests
                           │
                           ▼
                     VGM Tooling
        executable source + synthesis understanding
              │             │             │
              ▼             ▼             ▼
          foobar2000     libaural      attribution
          playback       testground     / forensics
              │             │
              ▼             ▼
          Omniphony     artificial hearing
```

### Helix

Helix owns project orientation, research questions, exact evidence, negative results, cross-project transfer, and re-entry state. This repository owns executable game-music code and its local tests/history.

### libaural

libaural owns general artificial hearing. VGM Tooling can provide unusually strong ground truth because it can know the exact source/driver/device state that generated a waveform and compare it with what libaural infers from the resulting audio.

This makes supported game music a programmable auditory-scene laboratory rather than merely a music corpus.

### Omniphony

Omniphony owns general headphone spatial presentation. It should not absorb YM2612, S-DSP, BRR, SMPS, GEMS, QSound-register, or other source-specific machinery.

The primary contract is excellent PCM. A later compact bridge may expose source-supported evidence such as multiplicity, directness, extent, stable motion, environmental energy, and confidence.

Useful mechanisms discovered in VGM Tooling may inform libaural or Omniphony, and those projects may in turn supply useful perceptual or rendering tests. Cross-project transfer does not change ownership.

## Realtime playback and executable analysis

Normal playback remains realtime. Do not require whole-song preprocessing, offline stem export, or reverse compilation before audio can begin.

But the broader project may contain **analysis and driver-understanding tools** that recover structure not present in a plain register log. Those tools are source-knowledge machinery, not a mandatory preprocessing stage for the foobar player.

For realtime playback:

```text
source stream
   ↓
live driver/register/DSP state
   ↓
chip- or system-specific renderer
   ↓
source-aware realtime mix
   ↓
foobar2000 PCM
```

## Accuracy is the foundation, not the ceiling

The accurate renderer is the scientific reference.

It is not the default quality ceiling for enhanced playback.

Enhanced rendering may remove or reduce historical limitations in sample storage, interpolation, synthesis precision, DAC behavior, bandwidth, channel mixing, aliasing, output filtering, and effects realization when the result remains traceable to the encoded musical work.

Preserve:

- notes and exact timing
- rhythm, groove, and phrasing
- instrument/patch identity
- authored modulation and automation
- musical hierarchy
- deliberate effects and coloration
- meaningful hardware behavior that became part of the instrument

Enhance where evidence supports it:

- source reconstruction
- bandwidth and interpolation
- transient fidelity and low-frequency body
- synthesis precision
- masking and separation
- mixing precision and headroom
- source extent
- environmental rendering
- stereo presentation

A hardware limitation is not automatically artistic intent. A hardware artifact that materially defines the programmed instrument may be.

## Current design centers

### Mega Drive / Genesis

The current VGM frontier tracks live YM2612 and SN76489 state before final stereo collapse.

Implemented or in active development:

- exact YM2612 register state and four-operator patch state
- sample-accurate YM2612 register timelines
- isolated FM backend contract for six channels
- enhanced SN76489-family tone/noise stems
- resolved classic YM2612 DAC playback
- direct VGM source-bank PCM streams with high-quality resampling
- authored left/right routing baseline
- high-precision source summation

The next major synthesis milestone is a mature six-channel YM2612 renderer that preserves exact patch/envelope/algorithm/feedback/LFO behavior before experiments remove selected hardware constraints.

### SPC / Super NES

The SPC path should retain both driver-level and S-DSP-level knowledge when available.

The editable SNESAPU source is the implementation foundation. The supplied SPCPlay/SNESAPU 2.21.3.9130 build is a newer behavioral reference.

Important future source layers include:

- eight S-DSP voices
- BRR sample identity and decode state
- pitch and interpolation
- envelopes/key state
- per-voice L/R routing
- noise and pitch modulation
- dry/echo distinction
- FIR/feedback/echo-buffer state
- higher-level driver/sequence state where recoverable, such as N-SPC tracks, instruments, notes, ties, modulation, and percussion mapping

## QSound

QSound is both a supported device family and an unusually valuable research system.

Its explicit PCM voices, pan state, per-voice echo, FIR filters, wet/dry delays, and final stereo stage provide a controlled example of source-domain spatial processing.

Native QSound playback must preserve authored behavior. Separately, its mechanisms may inform generalized source-domain stereo rendering for other systems. Generalize principles, not QSound branding or coloration.

## Sonic 3 subproject

**Sonic 3 Music Attribution** is a bounded VGM Tooling subproject/case in Helix.

It is useful in two directions:

1. VGM Tooling can give Sonic 3 research much stronger technical evidence about SMPS tracks, persistent musical identity, voice allocation, FM patches, PSG behavior, DAC samples, modulation, and prototype/final realization.
2. Sonic 3 provides a demanding real soundtrack on which VGM Tooling must prove that driver state, physical channel state, musical identity, and arranger fingerprints are not carelessly conflated.

The attribution evidence hierarchy remains stricter than technical resemblance. VGM Tooling can produce evidence; it does not convert similarity into authorship confirmation.

## Historical lineage

This repository supersedes the earlier private `dissonance-git/vgmspc` implementation line.

The old project already explored:

- VGM register shadowing
- YM2612 and SN76489 source state
- SPC eight-voice telemetry
- OPM/OPN/OPL family adapters
- persistent source IDs
- semantic/role experiments
- realtime foobar playback experiments

Useful state/provenance ideas survive. Premature role heuristics and old spatial-rendering architecture are historical evidence, not current truth.

The intended Git history migration is a true unrelated-history merge that preserves the original `vgmspc` commits as ancestors while retaining the current VGM Tooling working tree. See `docs/history.md`.

## Repository shape

The tree should converge only as real code justifies it:

```text
formats/       file/container semantics

drivers/       SMPS, GEMS, N-SPC, etc.

devices/       YM2612, SN76489, S-DSP, QSound, OPL/OPN/OPM, PCM...

model/         source/performance/device state and provenance

render/
  reference/
  enhanced/

frontends/
  foobar2000/

bridges/
  libaural/
  omniphony/

research/      fixtures, references, bounded experiments

tests/
```

Do not refactor existing imported component trees merely to match this diagram. First preserve and validate working code; move only when ownership is clear.

## Validation law

Every audible enhancement must remain reversible and be compared with the accurate/reference render.

Semantic recovery needs its own controls:

```text
authored source
→ known compiler / driver / synth
→ execution trace
→ recovered common model
→ compare with source truth
```

The common-model tests also protect representation boundaries such as static control flow versus runtime execution traces, repeated visits to the same program point, and explicit cross-domain time mapping.

Measurements should catch structural regressions, but listening remains decisive for perceptual quality.

The long-term playback target is:

> **Every supported soundtrack should aim to sound like the highest-quality realization its original musical data can support.**

The broader research target is:

> **VGM Tooling should become a comprehensive, provenance-preserving implementation for understanding emulated game music across authored source, executable state, driver behavior, synthesis, musical structure and acoustic realization, without forcing every system into one lossy representation.**

And for Helix-facing research:

> **Helix should be able to inspect a supported game-music object as an executable musical system rather than seeing only the stereo waveform it eventually produces.**
