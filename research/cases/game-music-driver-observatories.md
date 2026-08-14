# Game-music driver and tracker observatories

## Question

What can source-available trackers, music engines and reconstructed game drivers teach Game Music Interpreter that a device register trace cannot?

The central distinction is:

```text
authored / symbolic representation
        ↓ compiler or music driver
logical tracks + notes + instruments + effects + control flow
        ↓ execution
hardware-facing state and writes
        ↓
VGM / device trace / rendered audio
```

A lower layer can preserve the performance exactly while no longer exposing the vocabulary used at the layer above it. Therefore an exact register trace must not be promoted into exact authored notes, instruments or effects without a justified inverse mapping.

## Evidence roles

These repositories are mechanism observatories unless independent provenance ties one to the particular commercial work being analyzed. Similar output is not evidence that two works used the same tool or driver.

```text
source tied to work
→ direct source evidence

source not tied to work
→ mechanism observatory

reconstructed driver
→ strong behavioral observatory with reconstruction limits

register log
→ exact downstream execution evidence, not automatic upstream source evidence
```

## NES: tracker to NSF to APU

### Dn-FamiTracker

Pinned observation:

- `Dn-Programming-Core-Management/Dn-FamiTracker`
- commit `6660b0e5fdbebda9742a75cd01116d708d90d1c4`
- `Source/drivers/asm/driver.s`

The exported NSF driver explicitly models channel classes and tracker effects before APU writes. Its expansion paths include VRC6, VRC7, FDS, MMC5, N163 and Sunsoft 5B/FME7. It also distinguishes effects such as arpeggio, portamento and slides.

A source effect token can therefore be exact while the realized pitch trajectory still depends on prior channel state and tick-by-tick driver execution.

### FamiStudio

Pinned observation:

- `BleuBleu/FamiStudio`
- commit `70625dd09d8acaef831bbce468d416fb0b596be1`
- `FamiStudio/Nsf/nsf.s` and `SoundEngine/`

This supplies an independent modern route from structured music data through a native NES engine and NSF wrapper. Agreement between independent engines helps identify stable concepts; disagreement exposes engine-specific semantics.

### NSF boundary

The permanent `star-soldier-nsf` and `star-soldier-nes-apu-vgm` fixtures preserve the same work at different representation altitudes. The manifest deliberately claims no byte, event, track-order or playback equivalence.

An NSF can establish executable code/data, entry points, timing and expansion flags. It does not by itself establish a tracker row, instrument number or effect command. Driver identification must come first.

This makes the pair a future forward/inverse control:

```text
executable music object
        ↓ execute
NES APU activity

captured NES APU activity
        ↓ infer
candidate musical / driver state
```

## Game Boy: hUGEDriver

Pinned observation:

- `SuperDisk/hUGEDriver`
- commit `727ab8ebb6bac20eecf82f35356fe4871a2a383b`
- `hUGEDriver.asm`, `include/hUGE.inc`

The driver keeps four channel trajectories with order/pattern pointers, periods, note identities, portamento targets, vibrato/tremolo phase, envelopes and instrument-table state. Its exported note constants are explicit source coordinates rather than frequencies inferred from the final waveform.

```text
source note token
+ instrument/effect token
+ prior channel state
+ driver tick semantics
        ↓
performed device trajectory
```

The permanent Game Boy VGM control can be compared against this kind of source grammar without claiming the commercial game used this driver.

## PC Engine / HuC6280: MML and bytecode

Pinned observation:

- `BouKiCHi/HuSIC`
- commit `94c7f916c0df631e77a129cd632c56117886f764`
- `src/huc/doc/huc/mmldoc.txt`

The preserved MML documentation gives a concrete symbolic-to-bytecode route with six voices and constructs for tempo, stereo pan, volume, envelope, detune, octave, notes, rests, ties, duration, calls, jumps and repeats.

HuC6280 hardware itself exposes different execution modes: wavetable playback, direct data output and noise, plus an LFO relationship involving the first two channels. A source voice, a hardware mode and a persistent musical part are therefore different objects.

## SCC: source reconstruction above wavetable registers

A source-available SCC reconstruction project demonstrates a round trip from reverse-engineered Konami sequence data to a human-readable music representation and back to SCC-oriented sequence data. It is not original manufacturer source, but it shows concretely how structured notes, instruments and performance commands can exist above the K051649 register surface.

Current MAME and libvgm implementations independently agree on a five-channel, 32-sample wavetable mechanism and a halted low-period region. They also expose a preservation wrinkle: some VGM files store the already-halved SCC sound-core clock. MAME compensates by scaling those clocks before emulation, while libvgm's VGM-side stepping consumes the declared clock convention directly.

The code therefore keeps two explicit coordinates:

```text
VGM-declared SCC clock
→ /16 nominal-frequency convention

normalized SCC core clock
→ /32 nominal-frequency convention
```

The two can converge to the same oscillator frequency, but they are not interchangeable input domains.

## Namco: driver semantics above PCM devices

Pinned observation:

- `superctr/QuattroPlay`
- commit `448f316945d50a5aec5d1ed9607e04b4a0a3ee9c`
- `src/drv/track.c`, `src/drv/track_cmd.c`, `src/drv/enum.h`

The reconstructed Namco driver layer exposes track commands and state for note events, volume, pan, legato, note delay, preset maps, wave selection, pitch envelopes and volume envelopes before those states reach the PCM devices.

That matters for the permanent C140 and C352 controls. C140 exposes per-voice playback rate, stereo level, bank and sample start/end/loop state. C352 extends the PCM voice into a four-output state machine with forward/reverse/link looping, interpolation/filter state, phase flags and noise mode.

The stable decomposition is therefore not `voice = sample`:

```text
sample / waveform identity
+ playback-rate trajectory
+ loop/address state
+ decode state where applicable
+ gain trajectory
+ routing state
+ lifetime state
        ↓
voice episode
```

A persistent musical part remains a separate inference above the voice episode.

QuattroPlay itself documents substitutions used for some older Namco devices, so its reconstructed driver semantics are valuable while generated logs remain bounded by those implementation choices.

## Literature agreement

Music-computing literature independently supports two parts of this decomposition:

1. static musical notation/control flow requires execution semantics to map source location into performed time;
2. sample and wavetable synthesis separate stored waveform/sample identity from playback-rate pitch, looping and other expressive controls.

Literature tests whether the decomposition is principled. Exact device and driver behavior still comes from source code, emulator implementations and the real corpus.

## Earned consequences

### Source token != performed event

A note can be exact at the source layer while performed pitch remains unresolved until execution applies transpose, detune, slides, vibrato, LFO or other state.

### Executable rip != authored notation

NSF, KSS, HES and similar executable/ripped formats can preserve code and behavior without preserving a recognizable tracker grammar. Their semantic altitude must be discovered rather than inferred from the extension.

### Instrument reference != instrument definition

A sequence may select a patch, waveform, sample or preset whose definition lives elsewhere in ROM, RAM or chip tables.

### Physical voice != persistent part

Logical tracks can schedule device voices, PCM engines can retrigger/reuse them, and listeners can group multiple episodes into one continuing auditory stream.

### Clock identity is provenance

The SCC case shows that a numeric clock field is incomplete without its coordinate convention.

## Next tests

1. recover time-bearing pitch/voice trajectories from the real AY, SCC, HuC6280, NES and Game Boy controls;
2. execute the same-work NSF/VGM NES control and compare downstream APU trajectories without assuming track correspondence;
3. build one bounded open-source tracker/driver adapter so an exact source note can be followed through execution to device state;
4. inspect C140/C352 controls for sample identity, rate, loop and routing trajectories before attempting part inference;
5. seek historically tied source material only when provenance can actually be established.

## Stop conditions

Do not infer a tracker or driver from output similarity alone.

Do not flatten NSF, VGM, SPC, tracker projects and native driver source into one generic sequence format.

Do not call a hardware voice a musical part merely because it remains active across events.

Do not convert nominal oscillator frequency directly into note spelling, key or harmonic function.

> **The interpreter should learn the music by following the transformations that actually produced the sound, not by projecting one convenient vocabulary onto every machine.**
