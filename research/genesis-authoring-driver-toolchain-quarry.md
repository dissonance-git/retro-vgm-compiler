# Genesis authoring, driver, and toolchain quarry

Status: active research input  
Purpose: map authored music, driver-specific representations, reconstructed drivers, device logs, and later projections so VGM Tooling can validate each boundary independently

## Scope

This quarry expands the active `genesis-driver-source-vgm-boundary.md` control.

The central chain is:

```text
authored musical source / macro language
        ↓
parser / assembler / compiler
        ↓
driver-specific sequence + instrument + envelope + sample data
        ↓
running sound driver
        ↓
logical tracks / scheduler / allocation / modulation
        ↓
YM2612 / SN76489 / DAC writes
        ↓
VGM or another device-facing projection
        ↓
inverse reconstruction / later playback driver / musical analysis
```

Each arrow can lose information. The purpose of the quarry is to identify and test those losses rather than assume that any one representation is canonical.

## Naming rule

Use **SMPS** as the project-facing name for the Sega driver family because it is the established and widely used community term.

First-party Sega of Japan documents preserved by Hidden Palace identify historical driver material under the name `SOUND-SORCE`. Keep that name only as lineage/provenance when discussing the preserved Sega documentation.

```text
project-facing family name
SMPS

historical lineage metadata
Sega SOUND-SORCE documentation/source lineage
```

Do not rename the project model or tests around `SOUND-SORCE`.

## First-party Sega sound documents

Hidden Palace preserved official Sega of Japan Mega Drive sound-driver documents, YM2612/PSG documentation, floppy-disk source code, sample material, and development tools.

Useful provenance facts from the preservation report include:

- Sega had Z80 and later 68000+Z80 sound-driver lines;
- the preserved documents describe a PC-98-centered music/sound development workflow;
- sound data was written in driver-specific macro/assembly-like notation;
- source was assembled/compiled into Motorola-format or binary data;
- data could be transferred to a ROM-RAM board and inspected/played through a SOUND EDITOR on real Mega Drive hardware;
- the preserved media include source code, demo/project files, samples, utilities, and technical documentation.

The preservation report itself warns that official documentation can contain technical mistakes and should not automatically outrank later reverse engineering on behavior. This is a useful project rule:

```text
first-party documentation
= strongest historical provenance
!= infallible behavioral specification
```

Use first-party material to establish lineage, workflow, terminology, and documented intent. Use executable source, hardware behavior, and validated reverse engineering when they disagree on mechanics.

Source:

- https://hiddenpalace.org/News/Sega_of_Japan_Sound_Documents_and_Source_Code

## Sega Retro documentation

The following Sega Retro pages are useful human-readable maps of the community-understood SMPS and GEMS formats:

- https://segaretro.org/SMPS
- https://segaretro.org/SMPS/Headers
- https://segaretro.org/SMPS/Song_data
- https://segaretro.org/SMPS/Voices_and_samples
- https://segaretro.org/GEMS

Use these as navigation/reference material. When a claim can be checked directly against preserved driver source, exact game data, or first-party documents, retain the lower-level evidence as the answer key.

## Sonic Retro SMPS source corpus

### `sonicretro/smps-rips`

This repository is unusually valuable because it retains driver-specific source objects rather than only rendered logs.

Useful object families include:

```text
song / sequence data
FM instrument sets
modulation envelopes
PSG volume envelopes
DAC / drum mappings
driver configuration
coordination-flag definitions
pointers / layout notes
Z80 driver binaries
prototype / alternate / fixed data in some sets
```

Important files include:

- `DefCFlag.txt`;
- `DefDrv.txt`;
- `DefDrum*.txt`;
- `InsSet.bin`;
- `Modulat.lst`;
- `PSG.lst`;
- `Pointers.txt`;
- `Z80Drv.bin`.

This is the source-side complement to the user's Sonic 3 VGM corpus.

Source:

- https://github.com/sonicretro/smps-rips

### Sonic disassemblies

Useful repositories include:

- `sonicretro/s1disasm`;
- `sonicretro/s2disasm`;
- `sonicretro/skdisasm`.

`skdisasm` contains the Sonic & Knuckles Z80 sound-driver disassembly and support definitions. This is currently the strongest readable source-side control for the exact driver family underlying much of the Sonic 3/S&K work.

Organization quarry:

- https://github.com/orgs/sonicretro/repositories

## Independent reconstructed Sonic runtimes

### Sonic 3 A.I.R.

`Eukaryot/sonic3air` contains a high-level C++ reconstruction of the Sonic 3 & Knuckles sound driver.

Its source explicitly states that the implementation is heavily based on the S&K sound-driver disassembly from `skdisasm`.

The reconstructed `SoundDriver` exposes chip-write output as explicit objects with fields including:

```text
target
address
data
cycles
verification location
frame number
```

Targets distinguish:

```text
YAMAHA_FMI
YAMAHA_FMII
SN76489
```

This creates a valuable independent validation route:

```text
historical/reverse-engineered Z80 driver
        ↓
Sonic 3 A.I.R. readable reconstruction
        ↓
explicit chip-write stream
```

A.I.R. is **not** historical source truth. It contains fixes, extensions, and remaster-oriented behavior. Treat agreement with `skdisasm`/game behavior as validation evidence and any A.I.R.-specific change as a separate modern transformation.

Sources:

- https://github.com/Eukaryot/sonic3air
- `Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundDriver.cpp`
- `Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundChipWrite.h`

### Sonic Clean Engine

`TheBlad768/Sonic-Clean-Engine-S.C.E.-` is a heavily modified/cleaned Sonic 3 & Knuckles engine and is useful as a modern integration observatory.

It supports alternative sound-driver configurations and includes SMPS2ASM support definitions. The support file demonstrates that even within the broad SMPS family, note ranges, PSG tone/envelope identities, DAC mappings, and conversion behavior differ by driver/version.

This makes it useful for **variant pressure**, not as a historical answer key.

Source:

- https://github.com/TheBlad768/Sonic-Clean-Engine-S.C.E.-

## ValleyBell driver and conversion quarry

ValleyBell's repositories are especially useful at the current stage because several of them expose the transitions *between* representations.

### `ValleyBell/SMPSPlay`

Useful as a source-aware forward executor.

It supports driver-specific SMPS command/driver definitions, FM/PSG behavior, envelopes, drums, DAC timing, and VGM logging.

Strong control:

```text
known SMPS source/config
→ SMPSPlay
→ VGM
→ blind VGM Tooling inverse analysis
```

The inverse side must not receive the SMPS source identity as a hidden shortcut.

### `ValleyBell/GEMSPlay`

GEMSPlay is a matched Genesis control using a materially different driver representation over much of the same hardware.

It retains four major source data regions:

```text
instrument
envelope
sequence
sample
```

and contains original/recovered plus cleaned assembly for multiple GEMS driver versions, including 2.0, 2.5, and 2.8.

It can also emit VGM logs.

This yields:

```text
known GEMS source/driver
→ GEMS execution
→ VGM
→ blind inverse analysis
```

SMPS and GEMS should be used as matched same-hardware/different-driver controls.

### `ValleyBell/MidiConverters`

This repository is not valuable merely because it produces MIDI. Its stronger contribution is a broad corpus of reverse-engineered **driver-specific sequence grammar documentation** and converters.

The tree includes format notes and parsers for several unrelated driver families and platforms. This is useful for identifying recurring driver concepts without pretending that one command vocabulary is universal.

Particularly useful now: `Mucom88_Format.txt` documents an observed compiled MUCOM88/PC-88 representation including:

- FM/SSG/rhythm/ADPCM channel pointers;
- instrument selection;
- volume;
- detune;
- early note stopping and echo-related controls;
- modulation setup/disable;
- loops and loop exits;
- pan;
- raw FM register writes;
- timer/tempo writes;
- note hold.

This gives the MUCOM88 control a missing middle representation:

```text
MML text
→ compiled MUCOM88 command stream
→ OPNA execution
```

The MIDI output remains only a projection.

Source:

- https://github.com/ValleyBell/MidiConverters

### `ValleyBell/ExtractorsDecoders`

This repository is useful one layer earlier than a sequence parser. It contains extraction/decompression/descrambling tools for assorted game resources.

The durable lesson is:

```text
ROM / disk / archive bytes
→ extraction / decoding
→ driver data object
→ sequence parser / executor
```

Do not treat extracted music data as though it were stored as a clean standalone sequence in the original artifact when an extraction/decoder step was required.

Source:

- https://github.com/ValleyBell/ExtractorsDecoders

### `ValleyBell/sbfmdrv`

This unrelated Creative Labs FM-driver repository is valuable as a **methodological control**.

It contains:

```text
original-driver disassemblies
+ independent C ports
+ test files for comparing the C ports against the original COM drivers
```

That is an excellent template for VGM Tooling's own driver-validation discipline:

> A readable reconstruction should be validated against the original implementation's observable behavior, not trusted merely because the code looks equivalent.

This pattern should be applied to SMPS, GEMS, and later recovered SPC drivers whenever feasible.

Source:

- https://github.com/ValleyBell/sbfmdrv

## VGM specification boundary

The VGMRips VGM specification defines VGM as a sample-accurate sound logging format. For Genesis work it preserves device-facing writes, chip clocks, waits/sample timing, loop location when supplied, and related format metadata.

The key project rule remains:

```text
exact VGM device log
!= original driver program
```

VGM's 44.1 kHz sample-time coordinate is exact relative to the VGM artifact. Original driver ticks, source commands, logical tracks, macro structure, instrument-bank identity, and authorship are not automatically encoded by it.

Source:

- https://vgmrips.net/wiki/VGM_Specification

## Modern VGM re-encoding control: SGDK / XGM2

`Stephane-D/SGDK` provides a useful modern inverse-direction control through XGM2.

The XGM2 driver can take VGM/XGM2 music input and drive Genesis sound hardware with its own runtime semantics. Its public interface documents FM/PSG volume envelopes, multiple PCM channels, PCM priority/allocation, loop control, and tempo behavior.

This creates a distinct experiment:

```text
VGM device-facing object
→ modern XGM2 compilation/re-encoding
→ XGM2 runtime
→ YM2612 / PSG / PCM behavior
```

If the resulting device behavior matches sufficiently, that does **not** mean XGM2 recovered the original SMPS/GEMS program. It demonstrates another realization compatible with the lower device-facing object.

This is a useful negative control for source recovery:

```text
behaviorally compatible replay
!= historical program recovery
```

Source:

- https://github.com/Stephane-D/SGDK
- `inc/snd/xgm2.h`

## MML and authored-source representation quarry

The supplied MML projects are useful primarily as **representation counterexamples**.

### `starg2/yammld3`

YAMMLd3 has an explicit parser/AST/IR pipeline before MIDI output.

Its IR distinguishes, among other things:

- note nominal time and duration;
- previous/last nominal duration;
- gate time;
- per-note time shift;
- velocity;
- control changes;
- program changes;
- pitch bend;
- tempo;
- meter;
- key signature;
- higher MIDI/system events.

Durable lesson:

```text
source syntax
!= parsed AST
!= musical/control IR
!= MIDI serialization
```

Source:

- https://github.com/starg2/yammld3

### `kinkinkijkin/kPMML`

kPMML demonstrates a custom MML-like compiler with explicit synthesis controls such as pitch envelopes, pitch vibrato, amplitude, low/high-pass filtering, FM definitions, envelopes, generators, macros, channels, and expanded loops.

Useful lesson:

```text
MML-like language
can encode synthesis and realization state
far beyond note / pitch / duration
```

The compiler's loop expansion also provides a warning: an expanded event stream is not identical to the authored loop/control structure.

Source:

- https://github.com/kinkinkijkin/kPMML

### `cat2151/mml2abc`

This translator makes several projection choices explicit, including defaults for octave, tempo, volume and note length, mappings from MML volume to ABC dynamics, staccato approximation, repeat expansion, and resetting state on new tracks.

This is useful precisely because it shows where translation can introduce target-format policy.

Durable rule:

```text
translation default
!= authored fact
```

Source:

- https://github.com/cat2151/mml2abc

### `cat2151/chord2mml`

This project explicitly refuses to guess ambiguous octave behavior and leaves octave control to the user because the input does not determine whether an octave should move up or down.

That is a strong epistemic precedent:

```text
underspecified source
→ preserve ambiguity / require explicit choice
not
→ invent the most convenient default and call it recovered truth
```

Source:

- https://github.com/cat2151/chord2mml

### `Catz1301/mmlJS`

This small parser is useful as a lower-bound example: it recognizes a compact note/octave/length language and maps notes toward frequency values.

Its value is contrast. A parser that handles only note-name/frequency semantics is not sufficient for historical executable music whose source language also carries instruments, envelopes, modulation, control flow, routing, and machine-specific realization.

Source:

- https://github.com/Catz1301/mmlJS

## Authored-source preservation rule

The project should treat each historical/source dialect as its own exact representation first.

```text
source text / bytes
        ↓
dialect-specific parse
        ↓
dialect-specific authored semantics
        ↓
compiled / executable representation
        ↓
common musical/execution graph
```

Do **not** begin with:

```text
all MML
→ generic MML parser
→ generic notes
```

Different dialects assign different meanings to:

- `<` / `>` octave direction;
- gate commands;
- volume scales;
- instrument numbers;
- modulation syntax;
- ties/holds;
- loop syntax;
- macros;
- tempo units;
- direct register access;
- rhythm/noise/PCM commands;
- channel declarations.

The common model should receive normalized meaning **after** dialect-specific semantics are known.

## Strong validation program

### 1. Historical/reconstructed-driver equivalence

For a driver where original/reverse-engineered implementation and readable reconstruction coexist:

```text
same frozen source data
→ original/reverse-engineered driver
→ observable driver/device trace A

same frozen source data
→ independent readable reconstruction
→ observable driver/device trace B

compare A ↔ B
```

Use Sonic 3/S&K `skdisasm` ↔ Sonic 3 A.I.R. as one candidate.

Compare more than final PCM when possible:

- ordered chip writes;
- timing/cycle positions;
- logical channel state;
- key on/off behavior;
- envelope/modulation state;
- DAC scheduling;
- loop behavior.

### 2. Driver-source → VGM → blind inverse

For SMPS and GEMS independently:

```text
known source/driver truth
→ execute
→ capture VGM
→ hide source identity
→ VGM Tooling inverse analysis
→ compare recovered / ambiguous / lost facts
```

### 3. Same-hardware driver decoys

Match SMPS and GEMS samples on easy observables where possible:

- hardware family;
- approximate channel use;
- tempo;
- density;
- patch count/complexity;
- DAC usage;
- era/platform.

Then test whether driver/toolchain relational features still discriminate them.

This protects against mistaking hardware or repertoire for a driver fingerprint.

### 4. MML-source round trip

For a source whose compiler/runtime is available:

```text
exact source span / macro / loop
→ parsed authored semantics
→ compiled command stream
→ runtime events
→ device trace
→ inverse musical view
```

Measure which authored distinctions remain recoverable from each lower layer.

Candidate families:

- MUCOM88;
- modern synthetic MML dialects for parser controls;
- historical SMPS macro/assembly forms where source text exists.

### 5. Projection-loss controls

Project the same authored object into MIDI, ABC, VGM, or XGM2 where meaningful.

For each projection record:

```text
preserved
transformed
ambiguous
lost
new default/policy introduced
```

A successful projection is not evidence that its target format is a canonical model.

### 6. Extraction provenance

For music embedded inside ROM/disk/archive resources:

```text
original artifact hash
→ exact extractor/decoder version
→ extracted object hash
→ parser/compiler/executor
```

The extracted object must retain provenance back to the container and extraction transform.

### 7. Discovery versus exploitation

Following current Helix experimental law, fingerprint discovery cost and known-rule application cost must be reported separately.

A driver or arranger fingerprint is not considered discovered merely because it classifies a known target after the relevant cue was chosen from that target.

Use:

- known controls;
- held-out titles;
- matched decoys;
- masked easy cues;
- ablation of patch/hardware/library metadata;
- frozen rules before target evaluation.

## What this quarry changes now

No new runtime dependency is justified by this pass.

The pass strengthens five immediate implementation requirements:

1. historical/source dialects need source-specific parsers before normalization;
2. source/compiled/driver/device identities must remain separately addressable;
3. driver reconstructions should be validated against lower observable traces, not trusted by code inspection alone;
4. VGM-only inference must respect the format's information ceiling;
5. real attribution tests must mask easy driver/hardware/metadata cues and distinguish discovery from exploitation.

These requirements should feed the existing Genesis source-boundary, Sonic 3 corpus, MUCOM88, attribution-discovery, and whole-song/human-discourse controls rather than creating a separate architecture.
