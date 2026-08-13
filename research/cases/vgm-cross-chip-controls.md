# Cross-chip VGM control corpus

## Question

How should VGM Tooling expand beyond the current YM2612/SN76489 vertical slice without mistaking one famous hardware family for universal digital-music semantics?

The purpose of a larger VGM corpus is not simply to accumulate more songs. It is to expose the same musical and executable questions through chips with different synthesis models, register maps, clocks, channel topologies, internal sub-devices, and historical driver ecosystems.

The working rule is:

> **Each chip family is an independent teacher. Shared machinery is earned only when the same distinction survives across those teachers.**

---

## Why VGM is unusually useful here

The VGM format already provides a common transport while retaining device-specific commands. This lets the repository compare chip families without first flattening them into MIDI or PCM.

At the VGM transport layer, Yamaha register writes occupy a particularly useful contiguous command family:

```text
51      YM2413
52/53   YM2612 ports 0/1
54      YM2151
55      YM2203
56/57   YM2608 ports 0/1
58/59   YM2610 ports 0/1
5A      YM3812
5B      YM3526
5C      Y8950
5D      YMZ280B
5E/5F   YMF262 ports 0/1
```

For dual-chip VGM files, the corresponding second Yamaha instance uses the `A1-AF` mirror commands. `A0` remains the AY8910 command and is not part of that mirror.

This exact transport commonality does **not** imply common synthesis semantics.

Current implementation controls:

- `components/vgm/enhancement/vgm_yamaha_register_write.h`
- `components/vgm/enhancement/vgm_chip_clock.h`
- `components/vgm/enhancement/yamaha_opn_family.h`
- `tests/vgm/vgm_yamaha_register_write_test.cpp`
- `tests/vgm/vgm_chip_clock_test.cpp`
- `tests/vgm/yamaha_opn_family_test.cpp`

---

## Corpus selection should maximize information, not file count

A useful growing corpus should contain several kinds of controls.

### 1. Pure or nearly pure chip controls

Use soundtracks where one target chip dominates so basic register, clock, voice, pitch, envelope, and synthesis semantics can be validated without a large mixed-device attribution problem.

### 2. Related-family controls

Compare siblings that share real hardware ancestry.

Examples:

```text
OPN family
YM2203
YM2608
YM2610 / YM2610B
YM2612 / YM3438

OPL family
YM3526
YM3812
Y8950
YMF262
YMF278B

OPM branch
YM2151 / YM2164
```

The goal is to discover exactly which lower-level abstractions survive family changes.

### 3. Same-game / related-version controls

A title available in several hardware configurations can separate:

```text
same work / game context
from
same arrangement
from
same sequence
from
same synthesis implementation
```

Do not assume those are equivalent merely because the game title matches.

### 4. Mixed-chip controls

Mixed soundtracks are needed after single-family semantics are stable. They pressure-test part continuity and musical analysis when FM, PSG, PCM, ADPCM, wavetable, or other sources coexist.

### 5. Dual-chip controls

Dual-chip files test whether instance identity survives the VGM transport, state reconstruction, voice analysis, and later musical reasoning without accidental cross-instance state sharing.

---

## First high-value Yamaha control: The Scheme

VGMRips currently publishes two PC-8801 packs for *The Scheme*:

- OPN / YM2203: `https://vgmrips.net/packs/pack/the-scheme-nec-pc-8801-opn`
- OPNA / YM2608: `https://vgmrips.net/packs/pack/the-scheme-nec-pc-8801-opna`

This is unusually useful because it holds game and broad historical context relatively close while changing to a related Yamaha family member.

However, the pack notes explicitly state that the OPN and OPNA songs differ. Therefore this pair is **not** an exact-render or note-for-note equivalence control.

Use it for questions such as:

- which OPN FM register semantics remain invariant?
- which pitch/control abstractions survive?
- how does the larger OPNA capability surface alter arrangement choices?
- which differences are version/arrangement evidence rather than chip evidence?
- can higher musical analysis recognize related style/work context without inventing identical source structure?

Do not use it to claim:

```text
OPN track X == OPNA track X
```

without track-specific evidence.

---

## What the OPN-family source comparison has already earned

The current ymfm OPN implementation uses a common OPN FM register base while preserving substantial chip-specific behavior.

The useful shared FM register surface includes frequency/block, algorithm/feedback, operator parameters, keying, and the six-slot extension used by later family members.

Important differences already protected locally include:

```text
YM2203
3 FM register/channel slots
+ onboard SSG
+ one FM register port

YM2608
6 FM slots
+ SSG
+ ADPCM/rhythm machinery
+ two FM register ports

YM2610
six-slot register map
but four active FM channels in the YM2610 variant
+ SSG
+ ADPCM-A/B

YM2610B
same broad register family
but all six FM channels active

YM2612 / YM3438
6 FM channels
no onboard SSG/ADPCM block
+ DAC path
```

The VGM clock/header field is required to distinguish variants that the command byte alone cannot prove.

This is the desired pattern:

```text
shared transport
        ↓
shared register-family evidence where proven
        ↓
chip-specific state and synthesis
        ↓
shared musical observations only where independently earned
```

---

## Current real-corpus control

The existing Sonic 3 & Knuckles corpus contains 58 immutable VGZ files.

A fresh header audit over all 58 resolves the YM2612 clock field to:

```text
clock: 7,670,453 Hz
dual-chip flag: false
bit-31 variant flag: false
resolved VGM target: YM2612, not YM3438
```

for every file.

This gives the new generic clock/variant layer a real-corpus control before additional Yamaha families arrive.

---

## Suggested next corpus slices

Do not ingest hundreds of packs immediately. A small orthogonal matrix is more useful first.

### OPN

1. existing Sonic 3 & Knuckles YM2612/SN76489 corpus;
2. *The Scheme* YM2203 pack;
3. *The Scheme* YM2608 pack;
4. later, a strong YM2610 and/or YM2610B control.

### OPM

Add a clean YM2151 corpus. This is valuable precisely because OPM is Yamaha FM but not the OPN register/pitch model.

### OPL

Add at least:

1. YM2413/OPLL;
2. YM3812/OPL2;
3. YMF262/OPL3.

This should prevent four-operator OPN assumptions from leaking into two-operator/paired-channel OPL reasoning.

### Non-Yamaha contrast

After those families have stable low-level adapters, add deliberately different architectures such as:

- AY/YM2149-class PSG;
- Game Boy DMG;
- NES APU;
- HuC6280;
- Konami SCC/K051649;
- QSound;
- representative PCM/ADPCM systems.

These controls matter for the same reason libaural matters: a musical abstraction is more trustworthy if it survives implementation systems that do not share the same convenient hardware geometry.

---

## Per-corpus test ladder

Every newly admitted set should first pass inexpensive structural checks before it is allowed to influence higher analysis.

```text
immutable file identity
→ valid VGM/VGZ structure
→ header/version/data-offset sanity
→ declared chip clocks + flags
→ observed command-family inventory
→ complete register-write transport decoding
→ chip-specific register/state reconstruction
→ key/voice/pitch/control trajectories
→ persistent-part hypotheses where supported
→ rendered/audio comparison where available
→ harmonic/formal analysis only after its prerequisites survive
```

For cross-family studies, compare at multiple altitudes:

```text
transport equivalence?
register-semantic equivalence?
synthesis-state equivalence?
performance-event equivalence?
perceptual equivalence?
musicological equivalence?
```

A failure at one altitude must not automatically erase similarities at another.

---

## Acquisition and provenance

When new packs are admitted as permanent fixtures, preserve direct runnable VGM/VGZ files and seal them through the existing corpus manifest/hash path.

Record at minimum:

- source pack/page;
- source chip/system labeling;
- exact file names;
- exact hashes;
- VGM version;
- declared clocks/flags;
- acquisition date;
- any known rip/tool provenance;
- whether the set is a pure-chip, related-family, mixed-chip, dual-chip, or version-comparison control.

Do not treat a current VGMRips tag as stronger evidence than the bytes actually present in the file.

---

## Stop condition for premature sharing

Do not create one universal Yamaha FM state object merely because several chips have operators, envelopes, and algorithms.

A shared abstraction graduates only when at least two independently implemented chip adapters need the same semantic object and tests show that sharing it does not erase a source-specific distinction.

Likewise, do not create one universal `note` field merely because several chips expose frequency controls.

The target remains:

> **many exact machines, one increasingly well-earned musical understanding.**
