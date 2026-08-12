# VGM Tooling

Executable understanding, analysis, and source-native rendering of digital game music.

This repository is the implementation home for the **VGM Tooling** project. `VGM` in the project name is historical shorthand for video game music tooling; the project is not limited to the `.vgm` file format.

The long-term objective is stronger than playback:

> **Understand supported digital game music deeply enough to run it as an explicit internal musical machine, inspect every meaningful layer while it runs, and render the same encoded work through both accurate/reference and higher-quality source-native paths.**

A supported object should not have to collapse immediately into stereo PCM. Where the source permits it, the system should retain the route from musical program to physical synthesis to rendered sound.

```text
music container / executable source
        ↓
driver / sequence / performance state
        ↓
instrument / sample / patch state
        ↓
physical device voices and effects
        ↓
reference or enhanced synthesis
        ↓
source-aware mix
        ↓
stereo / stems / structured telemetry
        ↓
consumer
```

The foobar2000 components in this repository are important realtime frontends, not the definition of the project.

## Levels of truth

Do not collapse these layers:

```text
FORMAT
VGM, SPC, MIDI, native sequence/container, ROM-derived data

DRIVER / PERFORMANCE
SMPS, GEMS, N-SPC and other sequencers, track state, note events,
program changes, voice allocation, modulation, loops

DEVICE / SYNTHESIS
YM2612, SN76489, S-DSP, QSound, OPL/OPN/OPM, PCM/ADPCM,
registers, operators, envelopes, sample memory, effect state

RENDER
reference hardware behavior or source-native enhanced realization

AUDITORY INTERPRETATION
what a listener hears as persistent objects, streams, fields, rhythm,
foreground/background, masking, motion, and environment
```

A physical chip channel is not automatically a persistent musical voice. A register log is not automatically the original score. A perceptual stream is not automatically one physical source. Confidence and provenance must survive transitions between layers.

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

The intended Git history migration is a true unrelated-history merge that preserves the original `vgmspc` commits as ancestors while retaining the current VGM Tooling working tree. See `docs/HISTORY.md`.

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

Measurements should catch structural regressions, but listening remains decisive for perceptual quality.

The long-term playback target is:

> **Every supported soundtrack should aim to sound like the highest-quality realization its original musical data can support.**

The broader research target is:

> **Helix should be able to inspect a supported game-music object as an executable musical system rather than seeing only the stereo waveform it eventually produces.**
