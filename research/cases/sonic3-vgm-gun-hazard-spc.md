# Sonic 3 VGM / Gun Hazard SPC cross-format case

Status: active real-corpus control  
Purpose: pressure-test the current execution, evidence, attribution, whole-song, and discourse model against two very different source representations  
Corpus policy: `tests/CORPUS.md`

## Inputs

Two soundtrack collections supplied directly for analysis:

- `Sonic 3 & Knuckles.zip`
- `Front Mission ~ Gun Hazard.zip`

The preliminary pass analyzed the collections outside the repository and did not commit the source archives. The user has since authorized these two collections to become VGM Tooling test sets. They should be imported as immutable corpus objects when the original supplied bytes are available to the repository-writing environment.

Do not rewrite their embedded metadata during import.

## Metadata policy

The preliminary version of this case ignored all descriptive metadata because the first question was purely structural. The current project needs a more precise boundary.

For these corpus files:

```text
embedded artist field
= exact artifact metadata only
!= authoritative artist identity
```

The user's current artist names live in foobar2000 external tags and have already been ingested by Helix. Helix records `external-tags.db` as canonical for the user's local foobar metadata.

Therefore:

```text
VGM / SPC embedded artist
        ↓
preserve as artifact metadata

Helix-ingested foobar external artist
        ↓
preferred user-facing library identity
```

Neither metadata route automatically proves track-level composition authorship.

This is especially important for Sonic 3, where soundtrack-level/team metadata, composition, arrangement/implementation, prototype/final identity, and technical fingerprints can disagree legitimately.

## Sonic 3 & Knuckles: VGM execution trace

The supplied collection contains 58 `.vgz` objects.

Header-level facts from the preliminary pass:

- all 58 identify SN76489 clock `3,579,545 Hz`;
- all 58 identify YM2612 clock `7,670,453 Hz`;
- 57 use VGM version `0x150`;
- `Competition Menu` uses VGM version `0x110`.

The relevant source model is primarily:

```text
sample-timed VGM commands
        ↓
YM2612 / SN76489 state
        ↓
FM operators / PSG / DAC
        ↓
reference acoustic render
```

The VGM log directly preserves device-facing execution but normally does not prove the original SMPS logical-track identity by itself.

The current repository already has:

- exact ordered VGM command observation;
- rebuildable Genesis device state;
- bounded physical voice episodes;
- device-native pitch/control history;
- source-relative Genesis analysis features;
- SN76489 source/reference/enhanced paths;
- YM2612/DAC source-state work;
- a provenance-aware musical execution graph.

The current reasoning target is no longer merely `VGM -> note-like events`. It is:

```text
exact VGM execution
        ↓
patch / voice / control trajectories
        ↓
performance events and persistent-part hypotheses
        ↓
texture / motif / phrase / section / form
        ↓
heard musical behavior
        ↓
natural discourse with reversible evidence
```

## Sonic 3 attribution makes the corpus unusually valuable

Helix tracks an active `project:sonic-3-music-attribution` whose question is not simply “what artist tag belongs on this file?”

The current project state separates:

```text
track / exact version
→ composer
→ arranger / implementation author
→ source company or team
→ evidence
→ confidence / unresolved conflict
```

The historical project has prototype/final/replacement distinctions and a long-running track-by-track matrix. Technical arrangement evidence includes voice-bank use, MOD settings, FM/PSG initialization, patch reuse, SFX-heavy construction, and version divergence.

This gives the VGM corpus a strong **negative control**:

```text
embedded artist tag
!= composer proof

external foobar artist tag
!= composer proof

sound-team metadata
!= track-level composer proof

arrangement / implementation fingerprint
!= composition fingerprint

shared patch / voice / MOD behavior
!= authorship proof

prototype version
!= final realization
```

The corpus should preserve all of these statements simultaneously when supported rather than forcing one `artist` scalar to stand in for them.

### Useful paired Sonic 3 contrasts

The existing project history suggests a much better corpus strategy than treating all 58 files as independent random examples.

Prefer paired controls such as:

```text
prototype ↔ final
Act 1 ↔ Act 2
same composer candidate ↔ different arranger candidate
same arranger candidate ↔ different composition
shared voice/patch family ↔ different work
same work identity ↔ different realization/version
known attribution control ↔ disputed attribution target
```

These contrasts can isolate composition, arrangement/sound-programming, patch/sample design, driver/toolchain, and rendering evidence more cleanly than a flat soundtrack-wide classifier.

The active Sonic 3 frontier in Helix specifically calls for reconciling Data Select, Sonic 3 1-Up, and Sonic 3 All Clear conflicts and testing Maeda, Nagao, and Setsumaru arrangement fingerprints against known external controls.

VGM Tooling should supply technical evidence to those tests without deciding the historical attribution by itself.

## Front Mission: Gun Hazard: SPC machine snapshots

The supplied collection contains 61 SPC snapshots.

All 61 files in the preliminary pass:

- have the normal `SNES-SPC700 Sound File Data v0.30` signature;
- are 66,144 bytes;
- preserve the 64 KiB SPC700 RAM snapshot;
- preserve the S-DSP register image;
- have S-DSP `DIR = $20`, placing the sample directory at RAM `$2000` in every snapshot.

### Stable RAM

Comparing the 64 KiB RAM image byte-for-byte at the same address across all 61 songs:

- `27,173 / 65,536` byte positions are identical across every snapshot;
- identical fraction: approximately `0.414627`.

Large contiguous identical regions include:

- `$01FF-$132D`: 4,399 bytes;
- `$1361-$1E12`: 2,738 bytes;
- `$3A24-$5DE7`: 9,156 bytes;
- `$D7C2-$F81F`: 8,286 bytes.

These ranges prove stable shared memory content across the soundtrack. Their semantic roles must be established by execution/disassembly before labeling them as code, tables, samples, or another structure.

### Stable and moving BRR objects

Raw BRR objects reachable through the DSP source directory were hashed by content.

A strong result appears immediately:

- SRCN slots `0-10` contain the same terminating raw BRR object in all 61 snapshots.

But physical SRCN is not a stable identity for the higher dynamic sample bank.

Examples:

- one exact 6,939-byte BRR object appears in 28 tracks and occupies SRCN `32`, `33`, `34`, or `35` depending on the snapshot;
- one exact 4,050-byte BRR object appears in 25 tracks and occupies six different SRCN values across the collection;
- one exact 3,960-byte BRR object appears in 23 tracks and occupies eight different SRCN values.

Therefore:

```text
SRCN number
≠ persistent sample / instrument identity
```

Content identity survives relocation.

This is structurally similar to driver systems where one logical musical part may occupy different physical synthesis resources over time.

## Cross-format invariant

The first invariant survives two unrelated source architectures:

> **Physical execution coordinates must remain distinct from persistent musical identity.**

Examples:

```text
Genesis
musical / driver object
→ physical YM2612 or PSG realization

SNES / Gun Hazard
same BRR object
→ different SRCN slot between song snapshots

MIDI / module systems
instrument / part
→ potentially different physical or logical channels
```

A common musical model should maintain distinct identities for source/driver object, synthesis object, running physical voice episode, persistent musical part, and heard auditory stream when evidence permits.

Do not collapse them.

## Why the Gun Hazard collection is more than an SPC playback test

VGM and SPC expose complementary evidence.

```text
VGM
strong device-command timeline
weaker original-driver context

SPC
complete sound-machine snapshot
CPU/RAM/driver context survives
but the musical event stream may need execution to recover
```

A collection of SPC snapshots from one game is more informative than one SPC alone because cross-file comparison can separate stable engine/sample structures from song-specific state.

This suggests a general corpus operation:

```text
many snapshots from one engine
        ↓
shared-state analysis
        ↓
content identities / stable regions / moving allocations
        ↓
controlled execution
        ↓
driver and performance semantics
        ↓
whole-song comparison
```

### Useful Gun Hazard contrasts

The 61-file set should be mined for paired controls analogous to the Sonic 3 contrasts:

```text
same exact BRR object ↔ different SRCN slot
same sample family ↔ different musical role
same driver/stable RAM ↔ different song data
same physical S-DSP voice ↔ different persistent part across time
same musical role ↔ different physical voice
shared echo/global state ↔ song-specific settings
shared sample ↔ different articulation / envelope / pitch use
```

These are strong tests for whether VGM Tooling can tell **what stays the same** from **what merely occupies the same coordinate**.

## Converter comparison

`spc2midi-tsuu` demonstrates a useful intermediate strategy: run an SPC simulator with a MIDI-oriented DSP model, observe the eight executing S-DSP voices, and derive note/pitch-bend/volume/pan/expression/sample/effect information even without first identifying the game's original music driver.

That is useful prior art, but MIDI itself is not the target representation because it cannot preserve all source synthesis/effect semantics.

The intended hierarchy is:

```text
exact SPC execution
        ↓
S-DSP voice / BRR / envelope / routing state
        ↓
common musical events and trajectories
        ↓
persistent identities / structure / listening analysis
        ↓
optional MIDI projection
```

not:

```text
SPC → MIDI → reasoning
```

## Human musical discourse control

The new discourse research changes how these corpora should be evaluated.

A technically correct result such as:

```text
FM channel state changed; upper-register activity increased; envelope X shortened
```

is not a complete song-level answer.

The system should first be able to say something a person might naturally say, for example:

```text
this part suddenly feels busier and more urgent
```

or:

```text
the melody is basically still there, but the sound around it changes enough that the section feels new
```

Then it must be able to descend:

```text
natural description
        ↓
musical comparison
        ↓
parts / rhythm / register / timbre / articulation / effects
        ↓
exact VGM or SPC execution evidence
```

The wording must not be generated by a fixed feature-to-adjective dictionary. It should follow `docs/human-musical-discourse.md`.

## Paired whole-song validation

Together the two corpora now pressure several directions at once:

```text
SONIC 3 VGM
logged execution
→ synthesis
→ performance
→ structure
→ natural description
→ attribution evidence without overclaiming

GUN HAZARD SPC
machine snapshot
→ controlled continuation
→ synthesis / sample identity
→ performance
→ structure
→ natural description
```

The two collections also create a useful comparison between FM/PSG executable realization and sample-based S-DSP realization without requiring one source family to imitate the other's semantics.

## Next tests

### Gun Hazard

1. Run each SPC under the validated SPC700/S-DSP path.
2. Trace actual SRCN triggers instead of treating every directory entry as active.
3. Assign BRR content identities independent of SRCN.
4. Recover per-voice pitch/onset/release/envelope/pan/echo trajectories.
5. Compare those trajectories across songs and physical channel changes.
6. Test driver-level recovery against the stable RAM regions.
7. Mine paired sample/role/voice contrasts across the 61-song set.
8. Select several section-level musical contrasts for human-discourse evaluation.

### Sonic 3

1. Use the existing live YM2612/SN76489 state core rather than an ad-hoc parser.
2. Build exact FM patch identities independent of physical channel.
3. Recover note-like pitch/onset trajectories while retaining continuous FM controls.
4. Identify PCM/DAC sample objects independently of trigger channel.
5. Treat prototype/final/Act1/Act2 relationships as explicit paired controls.
6. Test known Maeda/Nagao/Setsumaru realization families before disputed tracks.
7. Route technical results into the Sonic 3 attribution project as scoped arrangement/sound-programming or driver evidence.
8. Keep composer attribution unresolved unless independent composition/historical evidence supports it.
9. Generate natural song-level descriptions from evidence bundles and test whether they remain recognizable to a listener.

### Metadata

1. Join corpus objects to Helix by stable local path and/or file hash when available.
2. Preserve embedded artist text as artifact metadata.
3. Use Helix-ingested external foobar tags for user-facing artist names.
4. Never let either tag route silently become composer proof.
5. Keep Sonic 3 version/role/conflict state attached separately from library display metadata.

### libaural

Render controlled examples from both systems and ask libaural to infer auditory events/streams from audio alone.

Compare:

```text
known source/performance state
↔ libaural auditory organization
```

The mismatch is research data rather than an error to hide.

## Current import boundary

The user has authorized both supplied collections to become repository test sets. The original archive bytes are not currently present in the GitHub repository, so no claim is made here that the corpus files themselves have been published yet.

When the supplied archives are available to the repository-writing environment, import them under the corpus policy, hash them, preserve them unchanged, and build the first real-file regressions from the paired controls above.
