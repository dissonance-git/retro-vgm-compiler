# Sonic SMPS pitch recovery frontier

## Status

Active inverse-analysis research case over the immutable Sonic 3 & Knuckles VGM corpus.

This case asks a deliberately narrower question than note transcription:

> How much of the source-side SMPS pitch trajectory can be recovered from downstream YM2612 register execution without pretending that VGM retained the original sequence tokens?

The current answer is stronger than generic audio-to-note estimation but weaker than exact source recovery.

---

## 1. Source semantics

Two independent executable/source lineages agree on the important mapping.

### Sonic & Knuckles disassembly

Pinned source:

`sonicretro/skdisasm@9fad8e21b6ca86f3e8fb654d9519d83a45b3e1f9`

The disassembled S&K Z80 sound driver records:

- `NoteRest = $80`;
- coordinate flags begin at `$E0`;
- track `Transpose` is stored separately;
- a note has track transposition added before frequency lookup;
- PSG uses `zPSGFrequencies`;
- FM reduces the transposed note to octave plus the one-octave `zFMFrequencies` table;
- the resulting frequency is stored separately from `Detune`;
- `zUpdateFreq` adds the signed detune displacement to the stored frequency before output.

The accompanying `_smps2asm_inc.asm` makes the compiled pitch syntax explicit:

```text
nRst = $80
nC0  = $81
nCs0 = $82
nDb0 = nCs0
...
nB0
nC1
...
```

Therefore the compiled note token preserves a discrete chromatic coordinate but does **not** preserve unique enharmonic spelling.

### Clone Driver v2

Pinned source:

`Clownacy/Clone-Driver-v2-CPP@0b2eb1f829f648c0d805709039268889a9938a5c`

This independent SMPS recreation uses the same conceptual relation:

```text
note token
- note-domain base
+ track transpose
→ frequency-table index
→ track frequency
```

and exposes current track frequency plus detune, with modulation optionally added afterwards.

The clone is a teacher/challenger, not authority over the historical driver. Agreement with the S&K disassembly is what makes the relation useful.

---

## 2. The resulting evidence ladder

The source evidence supports these separate coordinates:

```text
exact SMPS note token                  source fact when sequence is present
        ↓
transposed chromatic pitch coordinate  deterministic source/driver relation
        ↓
SMPS table frequency code              deterministic driver relation
        +
signed frequency displacement          independent driver state
        +
modulation / pitch slide                independent time-varying control
        ↓
YM2612 FNUM / BLOCK execution           exact VGM/device evidence when captured
        ↓
nominal channel frequency               deterministic synthesis projection
        ↓
acoustic / perceived pitch              separate later observation / hypothesis
```

Do not collapse these.

In particular:

```text
VGM FNUM
!= original SMPS token

transposed pitch
!= pre-transposition note

frequency residual
!= proven Detune byte

nominal FM frequency
!= guaranteed heard fundamental

chromatic coordinate
!= enharmonic spelling
```

---

## 3. Why nearest-note recovery fails

The first inverse experiment compared each observed VGM FM key-on with the S&K SMPS frequency table.

That immediately exposed an important failure mode.

SMPS detune is a signed **frequency-code displacement**, not a semitone count. Adjacent low-octave FM table entries may be separated by only about 39-40 code units. A legal source displacement such as `smpsAlterNote $1E` (+30) can therefore move the executed frequency closer to the next table note.

So this tempting rule is invalid:

```text
nearest SMPS table frequency
→ recovered note
```

A nearest-neighbor implementation would sometimes produce the wrong note precisely because it ignored valid source expression.

That negative result is now part of the design.

---

## 4. Candidate-set formulation

For one observed combined YM2612 frequency code:

```text
observed = (BLOCK << 11) | FNUM
```

construct every S&K FM table entry for which:

```text
observed - table_frequency
```

fits the driver's signed 8-bit frequency-displacement range.

Each surviving pair is only a candidate:

```text
(transposed table pitch, compatible frequency displacement)
```

The immutable 58-file Sonic corpus produces:

```text
ordinary full FM key-ons: 53,419

compatible displacement candidates per isolated key-on:
2 candidates:  2,772
3 candidates:    809
4 candidates: 18,886
5 candidates: 27,407
6 candidates:  3,545
```

So an isolated key-on is normally underdetermined. The median case has five compatible displacement explanations.

This is useful evidence against frame-local note classification.

---

## 5. Continuity collapses much of the ambiguity

The driver stores frequency displacement as persistent track state until another command changes it. That suggests a stronger inverse question:

> Which displacement values remain compatible across consecutive key-ons on the same physical FM channel?

`tools/sonic_smps_pitch_audit.py` implements a bounded first version:

```text
start with all signed-displacement candidates for key-on 1
∩ candidates for key-on 2
∩ candidates for key-on 3
...

when intersection becomes empty:
seal the current compatibility segment
start a new one
```

This is a compatibility segmentation, not a claim that every boundary is literally an SMPS detune command.

Current real-corpus result:

```text
continuity segments:             763
segments with one surviving d:   650
key-ons inside unique segments: 53,013 / 53,419
model coverage:                  99.2399707969%
```

The high percentage means that under the **S&K table + persistent signed displacement** model, the transposed table-pitch trajectory becomes unique for most observed key-ons.

It does **not** mean that 99.24% of original SMPS note tokens have been recovered exactly.

Remaining distinctions include:

- original note token versus track transposition;
- persistent Detune versus other frequency displacement;
- modulation/pitch-slide state;
- physical channel versus logical track identity under override conditions;
- sequence spelling;
- acoustic/perceptual pitch.

---

## 6. Named source controls

The inferred displacement states are not merely numerically convenient.

Several unusual values independently appear in the disassembled music sources.

### IceCap

The VGM continuity model repeatedly isolates a `-95` displacement on FM3 in IceCap Act 1/2.

The source corpus independently contains:

```text
smpsAlterNote $A1
```

where `$A1` is signed `-95`.

### Knuckles theme

The VGM model isolates a long `-16` segment in the S&K Knuckles theme.

The disassembled source independently contains:

```text
smpsAlterNote $F0
```

which is signed `-16`.

### Special Stage / Blue Spheres

The VGM model isolates a long `-8` segment in the Blue Spheres/Special Stage material.

The disassembled Special Stage source independently contains:

```text
smpsAlterNote $F8
```

which is signed `-8`.

These controls support the inverse mechanism:

```text
source table pitch
+ persistent frequency displacement
→ downstream executed FNUM/BLOCK
```

They do not prove that every inferred displacement segment corresponds one-to-one with one source command.

---

## 7. Why this matters for musical understanding

This gives VGM Tooling a stronger route toward harmony than either of the naive alternatives.

Not:

```text
FNUM
→ round to MIDI note
→ chord classifier
```

and not:

```text
rendered audio
→ generic pitch detector
→ chord classifier
```

Instead:

```text
exact device execution
        ↓
source-family pitch-table candidates
        +
temporal continuity / control-state hypotheses
        ↓
transposed programmed-pitch trajectory candidate
        ↕
independent libaural acoustic / heard-pitch evidence
        ↓
part / pitch interpretation
        ↓
harmonic segmentation
        ↓
chord / key / progression / cadence / form
```

The source-side and hearing-side routes should challenge each other rather than silently overwrite one another.

---

## 8. Immediate next discriminators

Before this trajectory is promoted into a general harmony input, test:

1. compare inferred displacement boundaries against exact `smpsAlterNote` timelines for source-available S&K songs;
2. verify how modulation initialization and pitch-slide modes appear at key-on boundaries;
3. test music/SFX overrides and physical-channel continuity so a hardware lane does not become a fake logical part;
4. compare inferred transposed pitch trajectories against exact SMPS note+transpose data where source files are available;
5. quantify failure cases rather than forcing a note when several source explanations survive;
6. compare the source-side trajectory with an audio/libaural pitch observation after the reference FM renderer is available;
7. only then expose a harmony-facing pitch observation with explicit source-family/model provenance.

The strongest next success criterion is not percentage agreement with MIDI.

It is:

> **Can the inverse model recover the same pitch/control distinctions that the source driver actually carried, while preserving the cases where VGM no longer contains enough information to choose uniquely?**
