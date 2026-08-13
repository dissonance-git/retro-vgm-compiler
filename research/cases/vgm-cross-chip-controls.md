# Cross-chip VGM control corpus

## Question

How should VGM Tooling expand beyond the current YM2612/SN76489 vertical slice without mistaking one famous hardware family for universal digital-music semantics?

The purpose of a larger VGM corpus is not simply to accumulate more songs. It is to expose the same musical and executable questions through chips with different synthesis models, register maps, clocks, channel topologies, internal sub-devices, and historical driver ecosystems.

> **Each chip family is an independent teacher. Shared machinery is earned only when the same distinction survives across those teachers.**

## Format layer first

The VGMRips VGM specification is the authority for file-format semantics. Emulator, driver, hardware and die-analysis sources answer the next question: what the addressed device does with those commands.

The current format layer now protects:

- version-aware clock words;
- VGM 1.51 dual-chip flags and chip-specific bit-31 variants;
- the overloaded VGM 1.00/1.01 clock field;
- VGM 1.70 distinct clocks for a second chip instance;
- Yamaha writes `0x51-0x5F` and second-instance mirrors `0xA1-0xAF`;
- `0xA0` remaining AY8910 rather than a Yamaha mirror;
- generic DAC Stream Control `0x90-0x95` without assuming a YM2612 destination;
- sample/wait accounting and loop-boundary validation.

Relevant implementation:

- `components/vgm/enhancement/vgm_format_version.h`
- `components/vgm/enhancement/vgm_chip_clock.h`
- `components/vgm/enhancement/vgm_yamaha_register_write.h`
- `components/vgm/enhancement/vgm_dac_stream_command.h`
- `tools/vgm_corpus_audit.py`

```text
VGM bytes
→ format semantics
→ chip-specific state
→ performed behavior
→ musical interpretation
```

## Yamaha family comparison

### OPN

The bounded OPN model currently distinguishes:

```text
YM2203      3 FM slots + SSG
YM2608      6 FM slots + SSG + ADPCM/rhythm machinery
YM2610      six-slot register map, four active FM channels + SSG + ADPCM-A/B
YM2610B     six active FM channels + SSG + ADPCM-A/B
YM2612/3438 6 FM channels + DAC, no onboard SSG/ADPCM block
```

Shared OPN helpers now cover key/channel geometry, four-operator key masks, operator register order, frequency latch/commit behavior, channel-3 special frequency registers and algorithm/feedback fields. `genesis_state.cpp` consumes those helpers, with the established YM2612 tests remaining the control.

### OPN + OPM

YM2151/OPM does not use OPN FNUM/block pitch registers. It independently agrees with OPN on two narrower four-operator facts:

```text
algorithm = bits 0..2
feedback  = bits 3..5

physical register slot order 1,3,2,4
→ logical operator order 1,2,3,4
```

Those facts live in `yamaha_four_op_fm.h`. OPM keeps its own key-code/key-fraction pitch path, 8-channel keying, LFO sensitivities and register geometry.

### OPL is a negative control

YM3526/Y8950/YM3812/YMF262 falsify the tempting idea that the OPN/OPM packing is universal Yamaha FM behavior.

OPL uses:

```text
connection = bit 0
feedback   = bits 1..3
```

The regression deliberately checks byte `0x07`:

```text
OPN/OPM: algorithm 7, feedback 0
OPL:     connection 1, feedback 3
```

OPL-family traits remain separate:

```text
YM3526   9 two-operator channels, 1 waveform, rhythm mode
Y8950    YM3526-class FM + ADPCM-B
YM3812   9 two-operator channels, 4 waveforms
YMF262   18 channels, 2 banks, 4 outputs, 8 waveforms,
         6 dynamically configurable four-operator pairs
```

### OPLL adds patch provenance

YM2413/OPLL adds a different identity problem:

```text
instrument 0
→ user patch in registers 00-07

instrument 1..15
→ preset instrument data supplied by chip/variant context
```

A register trace can prove that a preset number was selected without, by itself, proving the complete preset timbre definition. Patch selection and patch definition are separate evidence objects.

## Cross-family pitch convergence

The current code now lets different Yamaha branches converge only after their native pitch mechanics have been respected:

```text
OPN   block + 11-bit FNUM
OPM   key code + key fraction
OPL   block + 10-bit FNUM
OPLL  block + 9-bit FNUM
        ↓
family-specific mechanics
        ↓
nominal programmed pitch coordinate
```

A concrete regression uses:

```text
YM3812 @ 3,579,545 Hz: FNUM 580, block 4
YMF262 @ 14,318,180 Hz: FNUM 580, block 4
YM2413 @ 3,579,545 Hz: FNUM 290, block 4
```

All three resolve to about `439.990595 Hz` as the nominal channel basis.

That common value is derived without converting the chips to MIDI or claiming it is automatically the heard fundamental. Operator ratios, detune, PM/LFO, topology and perceptual pitch remain separate evidence.

## Current real-corpus control

The immutable Sonic 3 & Knuckles corpus remains the first real control. The generic `vgm_corpus_audit.py` now validates all 58 files independently of the Sonic-specific SMPS analysis:

```text
58 / 58 structurally valid
58 / 58 computed wait total == header Total # samples
57 looped files
57 / 57 loop offsets on command boundaries
57 / 57 computed loop duration == header Loop # samples
57 × VGM 1.50
 1 × VGM 1.10
```

The only non-looped file is `30 - Staff Roll (S&K).vgz`.

All 58 declare:

```text
SN76489: 3,579,545 Hz
YM2612:  7,670,453 Hz
```

with no dual YM2612 or YM3438 flag.

## External corpus strategy

Do not ingest hundreds of packs merely because they exist. Prefer a small orthogonal matrix:

1. YM2203 and YM2608 related-version controls;
2. a clean YM2151/OPM set;
3. YM2413/OPLL;
4. YM3812/OPL2;
5. YMF262/OPL3;
6. later, YM2610/2610B and deliberately non-Yamaha controls.

A same-title pair such as *The Scheme* OPN/OPNA is useful precisely as a related-version comparison. It must not be assumed note-for-note identical merely because the game title matches.

External pack metadata has been inspected, but additional pack bytes are not yet permanent verified corpus fixtures. Do not describe them as byte-tested until they pass the same admission path as Sonic.

## Primary documentation as referee

Sega Retro hosts scans and archives of official Sega/Yamaha material including YM2612 and SN76489 documentation, Mega Drive development-system material, Z80/68000 documentation, schematics and board-revision material.

Use the underlying scanned manuals and official documents as primary-source guardrails for:

- documented versus later-discovered behavior;
- clocks, registers, buses, memory maps and DAC behavior;
- terminology and register ordering;
- discrete versus integrated console sound-hardware revisions.

A hosted official manual can be primary evidence. A wiki summary or discussion remains secondary unless independently corroborated.

## Per-corpus test ladder

```text
immutable file identity
→ valid VGM/VGZ structure
→ version/data-offset sanity
→ declared chip clocks + flags
→ total-sample consistency
→ loop-boundary + loop-duration consistency
→ command-family inventory
→ chip-specific state reconstruction
→ key/voice/pitch/control trajectories
→ persistent-part hypotheses
→ rendered/audio comparison where available
→ harmonic/formal analysis only after prerequisites survive
```

A failure at one altitude must not erase similarities at another.

## Future preservation tooling consequence

The structural and state models also make future assistance with VGM set preparation plausible. The current code does **not** discover loops or build sets automatically.

The useful principle is simply that future loop validation should be state-aware rather than waveform-only. A musically seamless repeat can still hide divergent chip, stream, envelope, modulation or instance state. Conversely, a proven executable-state recurrence can be stronger evidence than a noisy waveform comparison.

`vgm_corpus_audit.py` is only the structural floor: it validates existing VGM timing and loop declarations. It does not yet propose new loop points.

## Stop condition for premature sharing

Do not create one universal Yamaha FM state object merely because several chips have operators, envelopes and algorithms.

A shared abstraction graduates only when at least two independently implemented chip adapters need the same semantic object and tests show that sharing it does not erase a source-specific distinction.

Likewise, do not create one universal `note` field merely because several chips expose frequency controls.

> **many exact machines, one increasingly well-earned musical understanding.**
