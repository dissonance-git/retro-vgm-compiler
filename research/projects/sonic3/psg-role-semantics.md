# Sonic 3 PSG role semantics

Status: active mixed-chip research lane  
Primary executable entry point: `python tools/sonic3_testbed.py vgm-harmonic-probe`  
Implementation: `tools/genesis_psg_semantics.py` + `tools/sonic3_mixed_harmonic_probe.py`

## Purpose

Sonic 3 & Knuckles uses the Genesis sound system as a mixed arrangement surface:

```text
YM2612 FM
+ YM2612 DAC / PCM
+ SN76489 PSG tone channels 1-3
+ SN76489 noise channel
```

The compiler must therefore not reduce the soundtrack to an FM-only object, but it must also avoid the opposite mistake of treating all four PSG channels as four additional independent notes.

The important distinction is:

```text
chip / physical channel
!= musical part
!= musical role
```

A PSG tone can be an independent melodic or inner voice, an ornament, accompaniment, or a doubling/shadow of FM material. The PSG noise channel can behave as percussion or texture and may be used in hi-hat-like patterns, but channel identity alone does not establish a named drum role.

## 1. Sonic source evidence

The reconstructed Sonic & Knuckles SMPS driver definition explicitly provides a PSG frequency table alongside the FM tables:

- `sonicretro/smps-rips/Z80/Sonic & Knuckles/DefDrv.txt`
- `PSGFreqs = DEF_Z80_T2`

The Sonic 3 & Knuckles disassembly provides a stronger musical control. Angel Island Zone 1 declares five FM tracks, DAC, and three PSG tracks in one song header:

- `sonicretro/skdisasm/Sound/Music/AIZ1.asm`

Its PSG1 and PSG2 streams contain substantial pitched note material. PSG3, by contrast, uses `smpsPSGform $E7`, PSG envelope changes, and repeated `nMaxPSG1` events. This gives the project a source-visible example where the same chip family participates in both pitched musical material and noise/percussion behavior.

That means the correct model cannot be:

```text
PSG = decoration
```

or:

```text
PSG channel = independent harmony voice
```

The role must be inferred from behavior and correspondence.

## 2. Composer / programmer practice

Fumihito Kasatani described one concrete Mega Drive technique in which PSG melodies shadowed FM melodies with a slight lag and different volume while working on *Alisia Dragoon*:

- https://pixelatedaudio.com/interview-fumihito-kasatani/

This is useful historical evidence because it demonstrates that simultaneous or near-simultaneous FM+PSG pitch can be an orchestration/doubling decision rather than additional contrapuntal content.

Broader Genesis documentation likewise records PSG use for lead and background melodic material rather than only sound effects:

- https://consolemods.org/wiki/Genesis%3AAudio_Chip_Notes
- https://www.smspower.org/Development/SN76489

The VGM specification establishes the source-level clock and command contract used by the executable probe:

- SN76489 master clock at VGM header offset `0x0C`
- PSG writes via command `0x50`
- https://vgmrips.net/wiki/VGM_Specification

## 3. Bass prior in mixed YM2612 + PSG arrangements

For the Sonic / Genesis mixed-chip lane, use this conservative prior:

> A low SN76489 tone does not establish `bass_foundation` merely because it is the lowest currently sounding hardware pitch while YM2612 musical channels are available.

This is an inference prior, not an immutable historical law.

The reason is architectural and musical: the PSG tone channels remain valid pitched voices, but bass function is a relational musical role. In the mixed Genesis context, the compiler should require independent persistent-part harmonic-bass ownership before a PSG part can define bass function or chord inversion.

Therefore:

```text
lowest PSG pitch
    ↓
register fact
    ✗
not automatically bass
```

while:

```text
persistent PSG part
+ repeated lowest structural voice
+ harmonic-bass ownership
+ phrase / voice-leading support
    ↓
possible bass hypothesis
```

Explicit authored source evidence must be allowed to override the prior.

This guard is intentionally context-sensitive. PSG-only systems such as the Master System must not inherit a Genesis YM2612+PSG bass restriction. PSG can and did carry bass material when it was the primary synthesis system.

## 4. Noise semantics

The SN76489 noise channel is not a pitch-class source.

The current semantic contract is:

```text
SN76489 noise active
    ↓
percussion_or_texture candidate
    ↓
inspect timing / envelope / noise mode / repetition / source structure
    ↓
possible hi-hat / snare / kick-like / accent / texture role
```

A fixed rule such as `PSG noise = hi-hat` is forbidden.

Hi-hat is nevertheless an important candidate because short repeated noise events are a common PSG percussion technique. A useful implementation reference is the SMS Power PSG tutorial, which documents white-noise hi-hat, snare, and bass-drum constructions:

- https://www.smspower.org/forums/17919-TomysSegaPSGSN76489MusicTutorial

For Sonic, SMPS source provides the stronger hidden teacher: repeated PSG3 noise patterns can later be compared with VGM-only rhythm-role inference.

## 5. MIR / literature implications

The literature pass suggests the same general architecture from a different direction: active pitch count and instrument/source identity should not be collapsed.

Relevant controls include:

- Pei & Hsu, *Instrumentation analysis and identification of polyphonic music using beat-synchronous feature integration and fuzzy clustering*, ICASSP 2009, DOI `10.1109/ICASSP.2009.4959547`. The work treats instrumentation as time-varying mid-level information rather than assuming every active source has the same role.
- Every, *Discriminating Between Pitched Sources in Music Audio*, IEEE TASLP 2008, DOI `10.1109/TASL.2007.908128`. Source grouping benefits from features beyond pitch alone.
- Martins et al., *Polyphonic Instrument Recognition Using Spectral Clustering*, ISMIR 2007. The source-separation problem is explicitly formulated as grouping simultaneous components into source objects.
- Della Ventura, *Voice Separation in Polyphonic Music: Information Theory Approach*, 2018, DOI `10.1007/978-3-319-92007-8_54`. Symbolic voice separation remains a distinct inference problem even when note events are already available.

For Retro VGM Compiler, native chip execution gives unusually strong source labels compared with audio-only MIR, but the conceptual lesson remains useful:

```text
active hardware sources
    ↓
source / episode continuity
    ↓
doubling vs independent part
    ↓
musical role
    ↓
harmony / counterpoint / orchestration
```

## 6. Executable mixed-chip probe

`tools/sonic3_mixed_harmonic_probe.py` now layers PSG semantics over the existing FM-only harmonic control.

It records separately:

- FM-only pitched intervals;
- PSG-only pitched intervals;
- mixed FM+PSG intervals;
- same-pitch FM/PSG doubling candidates;
- noncoincident PSG pitched activity;
- intervals where PSG is physically lowest but not bass-eligible under the mixed-context prior;
- PSG noise active time, control writes, attenuation writes, and observed onsets.

Pitch-class duration is counted by **presence**, not number of hardware sources, so an FM+PSG unison does not double the tonal weight of that pitch class.

The noise channel is excluded from harmonic verticalities.

## 7. Current firewall

The mixed probe is still a surface probe.

It may say:

```text
FM and PSG emit the same pitch here
→ doubling candidate
```

but not:

```text
these are definitely one musical part
```

It may say:

```text
PSG noise forms repeated short events
→ percussion candidate
```

but not:

```text
this is definitely a hi-hat
```

It may say:

```text
PSG contributes E while FM supplies C and G
→ surface C-major sonority support
```

but structural chord, key, function, cadence, and creator grammar still require the existing persistent-part / phrase / structural-harmony gates.

## 8. Sonic 3 hidden-teacher tests

The strongest next tests are source-hidden comparisons against SMPS:

### PSG independent-part test

1. read SMPS source and identify authored PSG1/PSG2 note streams;
2. hide source;
3. analyze VGM only;
4. ask whether the compiler recovers a persistent PSG line rather than calling every PSG note an FM doubling;
5. compare recovered pitch / rhythm / phrase relations to source.

### FM shadow / doubling test

1. locate source passages where PSG and FM carry the same or transformed material;
2. hide source;
3. infer correspondence from timing, pitch, contour, duration, and phrase context;
4. verify whether the compiler collapses hardware multiplicity into one orchestration relation without erasing both physical sources.

### PSG noise percussion test

1. use PSG3 source patterns as teacher labels for generic percussion-event timing;
2. hide source;
3. infer percussion pulse / accent / texture behavior from VGM writes;
4. only attempt hi-hat-like naming after temporal evidence is validated.

## 9. Research law

```text
PSG PITCH IS PITCH EVIDENCE.
PSG NOISE IS NOISE/PERCUSSION EVIDENCE.
NEITHER IS MUSICAL ROLE BY ITSELF.
```

And for Genesis mixed-chip bass inference:

```text
LOWEST HARDWARE FREQUENCY != BASS FUNCTION.
```
