# Genesis driver-source / VGM boundary control

Status: active known-answer research control  
Purpose: use surviving SMPS and GEMS driver/source material to measure exactly what survives into VGM, what can be reconstructed from VGM, and what must remain source-relative or uncertain

## Why this control exists

Genesis music gives VGM Tooling an unusually strong forward/inverse validation pair.

We can observe the same broad hardware target through several representations:

```text
SMPS or GEMS authored/driver data
        ↓
known driver execution
        ↓
YM2612 / SN76489 / DAC commands
        ↓
VGM log
        ↓
VGM Tooling inverse analysis
```

The first half has source-level answer keys. The second half deliberately throws some of that information away.

The question is therefore not merely whether a VGM can be rendered accurately. It is:

> Which musical and technical identities survive the projection into VGM, which can be recovered by inference, and which are genuinely no longer represented?

That distinction is essential for both song understanding and attribution.

## Primary observatories

### Sonic Retro SMPS corpus

`sonicretro/smps-rips`

The repository describes itself as raw sound data extracted directly from ROMs for games using Sega's commonly named SMPS driver family. It contains songs, PCM samples and instrument data and separates several broad driver families, including 68k, preSMPS and Z80 variants.

It also exposes driver-specific side information that a flat chip log does not intrinsically contain:

- `DefCFlag.txt`: sequence commands / coordination flags;
- `DefDrv.txt`: instrument format, tempo algorithm, FM/PSG frequencies, available channels and other driver parameters;
- `DefDrum*.txt`: drum-note routing and instrument mappings;
- `InsSet.bin`: FM instrument sets;
- `Modulat.lst`: modulation-envelope data;
- `PSG.lst`: PSG volume-envelope data;
- `Pointers.txt`: game/driver pointer information and implementation oddities;
- `Z80Drv.bin`: driver-code dumps;
- prototype and fixed/alternate rips for some games, including Sonic 3 material.

This is much richer than a generic collection of VGM files because the source objects are already partitioned by driver semantics.

Source:

- https://github.com/sonicretro/smps-rips

### Sonic Retro disassemblies

The Sonic Retro organization also preserves game disassemblies that contain readable sound-driver and music-source material, including:

- `sonicretro/s1disasm`;
- `sonicretro/s2disasm`;
- `sonicretro/skdisasm`;
- related disassemblies and sound-source support files.

`skdisasm`, for example, includes the Z80 sound driver and SMPS assembly support definitions. These repositories are useful when a ripped data object must be tied back to concrete driver code, command handling, state updates, or game-specific behavior.

They are observatories and provenance sources, not code to absorb anonymously into the common model.

Organization quarry:

- https://github.com/orgs/sonicretro/repositories

### SMPSPlay

`ValleyBell/SMPSPlay` is a second independent observatory over the same family.

Its documented features include:

- customizable SMPS commands and drums;
- per-driver FM/PSG frequency and modulation/volume-envelope settings;
- a wide range of SMPS effects and commands;
- FM, PSG and DAC drum behavior;
- global FM instrument tables;
- DAC timing based on Z80 cycle calculations;
- driver-specific oddities and variants;
- VGM logging, including automatic looping.

Its engine is explicitly described as being based on disassemblies of various SMPS sound drivers.

This makes SMPSPlay useful for controlled forward execution:

```text
known SMPS data/configuration
        ↓
SMPSPlay execution
        ↓
VGM log
```

The resulting VGM can then be analyzed without handing the inverse side the SMPS answer key.

Source:

- https://github.com/ValleyBell/SMPSPlay

### GEMSPlay and surviving GEMS assembly

`ValleyBell/GEMSPlay` exposes a materially different Genesis driver family targeting much of the same hardware.

Its documented input organization includes four GEMS data sets in the order:

```text
instrument data
envelope data
sequence data
sample data
```

It can also load those regions from a ROM and can emit VGM logs.

More importantly for this project, the repository includes surviving/recovered GEMS Z80 assembly for multiple versions, including original and cleaned source files for GEMS 2.0, 2.5 and 2.8.

Examples in the repository tree include:

- `asm-sources/Z80_2-0.ASM`;
- `asm-sources/Z80_2-0_cleaned.ASM`;
- `asm-sources/Z80_2-5.ASM`;
- `asm-sources/Z80_2-5_cleaned.ASM`;
- `asm-sources/Z80_2-8.ASM`;
- `asm-sources/Z80_2-8_cleaned.ASM`.

The raw/original files are valuable for provenance. The cleaned files are normally the better working observatory for reading behavior. Do not silently treat a cleaned source as the historical byte-for-byte original.

Source:

- https://github.com/ValleyBell/GEMSPlay

### User-supplied raw source material

The user has additional raw SMPS/GEMS source material outside the GitHub repositories above.

That material should be treated as an archival/reference layer when its exact bytes are available to the repository-writing environment. It has not been inspected in this pass, so no claim in this document depends on unseen local files.

If a cleaned public source and the user's raw source disagree, preserve both and record the transformation or discrepancy rather than choosing silently.

## VGM is an exact lower projection, not the original music program

The VGMRips VGM specification defines VGM as a sample-accurate sound logging format. The command stream records timing and writes to supported sound devices. For Genesis, relevant commands include writes to the SN76489 and the two YM2612 ports; timing is expressed in a 44,100-sample-per-second timebase.

The format also carries useful device/header information such as chip clocks, total sample count and an optional loop point.

That is strong evidence for the executed device-facing trace.

It is not, by itself, a container for every original driver concept.

```text
VGM exactness
= exactness with respect to the preserved VGM command/timing object

VGM exactness
!= automatic recovery of the original driver program
```

A VGM-only adapter therefore must not invent an original logical track, sequence command, coordination flag, source pointer, instrument-bank identity, scheduler branch, or driver family merely because the resulting chip behavior looks familiar.

Specification:

- https://vgmrips.net/wiki/VGM_Specification

## Same hardware is the critical decoy

SMPS and GEMS make an unusually good matched control because they can target the same broad Genesis sound hardware while organizing music differently.

```text
SMPS
        ↘
          YM2612 / SN76489 / DAC
        ↗
GEMS
```

Therefore:

```text
same chip family
!= same driver

similar register traffic
!= same command grammar

same FM patch topology
!= same source instrument identity

same physical channel use
!= same logical track model

same rendered phrase
!= same authored representation
```

This is exactly the kind of confound that an attribution system must survive.

If an analyzer can distinguish SMPS and GEMS only because one corpus happens to use different musical genres, composers, or patch banks, that is not a discovered driver fingerprint. The corpus must mask those cheap cues where practical.

## Evidence ceiling by source

A useful working matrix is:

| Question | VGM only | Known SMPS source/driver | Known GEMS source/driver |
|---|---|---|---|
| exact VGM command order | exact | derivable after logging | derivable after logging |
| YM2612/SN76489 device state | exact/derived from log | exact/derived from execution | exact/derived from execution |
| VGM sample timing | exact | derivable after logging | derivable after logging |
| original driver family | normally unavailable/unknown | exact when source identity is known | exact when source identity is known |
| original logical track | normally unavailable | source/driver dependent | source/driver dependent |
| source command grammar | unavailable | available | available |
| source control-flow branch | unavailable unless independently reconstructed | available during known execution | available during known execution |
| source instrument/envelope identity | not intrinsically preserved | source-relative | source-relative |
| FM register realization | exact/derived | exact/derived | exact/derived |
| persistent musical part | derived/hypothesis | derived with stronger support | derived with stronger support |
| phrase/section/form | derived/hypothesis | derived/hypothesis | derived/hypothesis |
| composer | unresolved without independent evidence | unresolved without independent evidence | unresolved without independent evidence |

`available` here does not mean every public tool already exposes the answer through one API. It means the representation contains a route to the answer that VGM alone may not.

## Forward/inverse validation protocol

For a known driver/song object:

1. Freeze the exact source bytes and driver/configuration identity.
2. Execute through a source-aware driver path.
3. Record driver-level events before chip projection where possible.
4. Produce a VGM log from the same execution.
5. Feed only the VGM to the normal VGM inverse adapter.
6. Compare the inverse result with the hidden source/driver answer key.
7. Grade each recovered fact separately.

```text
KNOWN ANSWER
source command
logical track
instrument/envelope object
driver timing/control flow
        ↓
forward execution
        ↓
VGM device trace
        ↓
blind inverse reconstruction
        ↓
COMPARE
```

The inverse side should receive no reward for confidently guessing facts absent from the VGM evidence.

### Three outcomes matter

```text
RECOVERED
inverse result matches hidden source fact

AMBIGUOUS
several source explanations remain consistent with the VGM

LOST
source fact has no reliable route from the VGM projection
```

`lost` is not a parser failure. It is often the correct scientific answer.

## Sonic 3 becomes a much stronger control

The existing Sonic 3 VGM corpus should no longer be treated only as a register-log collection.

Where corresponding SMPS source/rips and disassembly evidence are available, the same musical object can be aligned across:

```text
Sonic 3 SMPS source / ripped sequence
        ↓
SMPS driver semantics
        ↓
YM2612 / PSG / DAC execution
        ↓
VGM trace
        ↓
current VGM Tooling analysis
```

This lets the project answer questions it could not answer from the VGM alone:

- Did a modulation shape come from an explicit driver envelope or emerge from another control pattern?
- Did a persistent musical part move physical channels, or are two physical episodes actually different logical tracks?
- Is a patch reuse source-level identity or merely register-level similarity?
- Is a timing quirk authored, driver-scheduler behavior, or a logging/rendering artifact?
- Which technical features remain stable between prototype/final variants after controlling for driver family?

For the Sonic 3 attribution project, this is especially important because it can separate:

```text
COMPOSITION
from
ARRANGEMENT / SOUND PROGRAMMING
from
DRIVER / TOOLCHAIN
from
PATCH / SAMPLE DESIGN
from
RENDERING
```

A source-level SMPS fingerprint can strengthen a driver or realization claim without becoming composer proof.

## GEMS as a matched negative control

GEMS should not be treated merely as another supported format candidate.

It is a **matched negative control** for Genesis inference.

A strong experiment deliberately chooses SMPS and GEMS examples that overlap in:

- YM2612/SN76489 usage;
- rough channel count;
- patch-family complexity;
- note density;
- tempo range;
- DAC use where practical;
- musical era/platform.

Then test whether driver-level relational features still separate them.

Potential discriminators belong to the `DRIVER / TOOLCHAIN` coordinate:

- command grammar;
- sequence/data layout;
- scheduler behavior;
- logical-track lifecycle;
- channel allocation policy;
- envelope/control representation;
- loop/branch idioms;
- driver-specific timing behavior.

Patch statistics or musical style should be treated as confounds unless the experiment explicitly studies them.

## Round-trip invariants

A forward execution that emits VGM should preserve the device-facing facts expected by the VGM specification.

Candidate invariants include:

```text
ordered YM2612 writes
ordered PSG writes
wait/sample timing
DAC byte/control stream where represented
chip clocks / variant metadata
loop boundary when the logger establishes one
```

Source-level identities that do not survive the projection must be retained on the forward side rather than smuggled into the VGM file as if the format carried them.

This gives VGM Tooling a useful two-view law:

> **The forward model may know more than the interchange artifact, and the inverse model must respect that information loss.**

## Immediate executable regression

The accompanying model regression should enforce at least these laws:

1. a VGM-only observation may carry exact device writes/timing while original driver/logical-track identity is unavailable;
2. a known SMPS source control may carry exact driver-family and command-grammar evidence;
3. a known GEMS source control may carry a different exact driver-family/data-layout identity while targeting the same Genesis device family;
4. equal hardware target must not collapse the two driver identities;
5. neither known driver identity establishes composer attribution.

The regression is intentionally small. Real source execution and byte-for-byte forward/VGM comparison come next.

## Next empirical pass

1. Select one Sonic 3 SMPS object with a clean source/rip/disassembly route.
2. Freeze source hashes and driver configuration.
3. Produce or identify the corresponding VGM trace.
4. Align source commands, driver ticks and VGM sample-time commands.
5. Measure exactly which logical identities survive or can be reconstructed.
6. Repeat with one GEMS object on the same hardware family.
7. Build a matched SMPS/GEMS decoy pair that suppresses easy musical and patch-level cues.
8. Promote only discriminators that survive held-out titles and driver variants.
9. Feed the recovered song-level information into the whole-song/human-discourse path.
10. Keep all raw, cleaned, logged and derived objects provenance-separated.

That pass will tell us whether the current model can genuinely climb from a register log back toward the program that created it, rather than merely assigning plausible musical labels to chip activity.
