# Driver-lineage controls

## Question

How should VGM Compiler distinguish hardware semantics from software-driver semantics when many games share one sound platform but use unrelated or only partially related music engines?

The answer is to treat **driver lineage as an explicit control surface**, not as a hidden synonym for platform or file format.

```text
same hardware
!= same driver
!= same sequence language
!= same allocation policy
!= same modulation semantics
!= same musical execution path
```

This pass adds two unusually strong observatories:

- `loveemu/vgm-disasm` at `e96c5b35649f8e814cac3c31b65cedc07b52d76d`
- `loveemu/agbinator` at `ec56b1ecfebddd29131f4934f65a94dfabcaf65d`

VGMTrans provides an independent integration signal. Its recent AKAO SNES work explicitly cites Loveemu's driver analysis for vibrato, vibrato fade, tremolo, pitch slides, and pitch envelopes, showing that these reverse-engineered driver semantics are already useful enough to improve a mature extraction tool.

## SNES: one DSP, many software worlds

The SNES has one broad SPC700/S-DSP hardware target, but `loveemu/vgm-disasm` contains annotated/reconstructed driver material organized across many independent developers, publishers, programmers, and build lineages.

The repository includes distinct families associated with, among others:

- Nintendo / NSPC lineages;
- Square / Minoru Akao lineages;
- Capcom;
- Hudson;
- Falcom;
- Factor 5;
- ASCII;
- Atlus;
- Chunsoft;
- Compile;
- Arc System Works;
- and many others.

This creates a far stronger generalization surface than learning SNES behavior from one soundtrack or one engine.

### Consequence

A physical S-DSP voice trajectory can be exact while the upstream meanings remain driver-specific.

For example, two drivers may both ultimately write:

```text
voice pitch
voice volume
sample source
ADSR/gain
KON/KOFF
```

while exposing completely different higher-level constructs for:

```text
note duration
instrument selection
vibrato
pitch envelope
portamento
track loop
conditional control flow
voice stealing
priority
reverb send
```

Therefore:

```text
same downstream DSP behavior
!= same upstream command semantics
```

and:

```text
similar driver byte pattern
!= same lineage unless independently supported
```

## Driver lineage is evidence, not identity by assertion

Driver identification may be supported by:

- exact embedded signatures;
- matching executable routines;
- matching command dispatch structure;
- shared data structures;
- stable build-specific code regions;
- known SDK/source lineage;
- historically documented toolchain information.

But a detector result remains a typed claim with provenance.

The system must preserve distinctions such as:

```text
exact source-signature match
strong code-structure match
partial implementation match
behavioral similarity
unknown
```

A weak similarity classifier must not silently become a source fact.

## GBA: hardware is especially insufficient

`loveemu/agbinator` exists specifically to identify different Game Boy Advance sound engines from ROM evidence.

Its current detectors include materially different engine families such as:

- Nintendo MusicPlayer2000 / m4a / MP2K;
- GAX Sound Engine;
- MusyX Audio Tools;
- Krawall;
- GBAModPlay / LS_Play;
- Konami/KCEJ variants;
- Apex/AAS;
- RADriver;
- Software Creations;
- DICE Canada;
- NMod;
- Nintendo R&D2-related code;
- and additional commercial/custom engines.

The exact supported list can evolve; the important architectural result is stable:

```text
GBA ROM
-> identify sound-engine lineage
-> select lineage-specific parser/runtime semantics
-> execute into common hardware/acoustic evidence only where earned
```

Do not create one generic `GBA sequence` ontology.

## False positives are scientifically useful

The recent AGBinator history contains an instructive correction: a Krawall signature was removed because of false positives and later replaced/refined with stronger evidence.

This is exactly the behavior VGM Compiler should preserve.

```text
detector matched
-> provisional lineage claim
-> adversarial corpus test
-> correction when false positive appears
```

The detector's mistakes are not noise to hide. They reveal which features are insufficiently discriminative.

## VGMTrans as an independent consumer

Recent VGMTrans changes are especially useful because they demonstrate a second implementation lineage consuming reverse-engineered driver semantics.

Relevant 2026 examples include:

- AKAO SNES vibrato support;
- vibrato fade-in;
- tremolo;
- pitch slides;
- pitch envelopes;
- abstraction of vibrato/tremolo controller handling;
- PSF metadata hint plumbing;
- 2SF/NCSF track-name recovery;
- PCM8 signedness correction on export.

These changes reinforce several current project laws:

```text
sequence command != static note
```

because modulation can evolve through time, and:

```text
sample bytes != interpreted PCM waveform
```

because signedness/format semantics remain part of decoding.

VGMTrans is still an observatory, not the canonical ontology.

## Recommended control matrix

Future driver work should intentionally compare at least three kinds of pairs.

### Same hardware, different driver

Example:

```text
SNES driver A
vs
SNES driver B
```

Question: which semantic relations survive while bytecode and runtime policies change?

### Same driver family, different builds/games

Question: which implementation differences are version/build differences rather than new semantics?

### Similar output, unrelated driver

Question: can the classifier resist inferring lineage from musical or acoustic similarity alone?

This third case is the adversarial negative control.

## Common model consequence

The vertical route should retain a driver-lineage coordinate:

```text
preserved source/runtime object
-> driver/toolchain lineage claim
-> driver-specific sequence/control state
-> logical tracks/notes/instruments/effects
-> allocation/runtime state
-> device state
-> acoustic realization
-> musical/perceptual interpretation
```

The lineage claim can be exact, derived, candidate, or unknown independently of downstream device certainty.

## Highest-information next tests

1. Select three materially different SNES drivers from `vgm-disasm` and compare note-duration, pitch-modulation, instrument, and control-flow semantics.
2. Use one known related-build pair to measure which driver fingerprints remain stable across revisions.
3. Use one unrelated pair with superficially similar DSP output to ensure lineage inference does not promote acoustic similarity.
4. Add GBA/GSF only with an explicit engine-lineage field so MP2K, GAX, MusyX, Krawall, and custom engines cannot collapse together.
5. Treat detector disagreements and false positives as corpus controls.

## Stop conditions

Stop rather than guess if:

- platform name is used as a driver name;
- one downstream register pattern is treated as proof of one upstream command;
- output similarity is treated as source/toolchain identity;
- a detector's heuristic match is stored as exact provenance;
- one successful driver parser becomes the generic platform parser;
- a driver-version difference is flattened away before testing whether it changes semantics.

Correction outranks coherence.
