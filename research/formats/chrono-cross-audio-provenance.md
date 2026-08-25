# Chrono Cross audio provenance roles

## Question

How should VGM Compiler preserve the human and technical transformation chain behind *Chrono Cross* without collapsing composer, synthesizer programmer, sound programmer, sequence format, driver, and PlayStation SPU into one vague idea of “the soundtrack”?

This note records a human-provenance boundary that became technically actionable once PSF1, VGMTrans AKAO parsing, and PlayStation SPU execution were all available as independent observatories.

## Credited roles

The *Chrono Cross* soundtrack/game credits consistently distinguish:

```text
Yasunori Mitsuda
    composition / arrangement / production

Ryo Yamazaki
    synthesizer programming / synthesizer operation

Minoru Akao
    sound programming
```

Do not rewrite these into interchangeable labels.

The distinction is useful because the current technical evidence also separates several corresponding transformation layers.

## Minoru Akao and the AKAO system

The PlayStation Square sequence family parsed by VGMTrans uses the literal `AKAO` signature and is conventionally called AKAO.

Reverse-engineering documentation for Square's PlayStation sound system attributes the custom AKAO sound driver/format lineage to sound programmer Minoru Akao. That attribution is strong enough to retain as documentary provenance, but the project should still distinguish:

```text
format/driver traditionally attributed to Minoru Akao
!= proof that every byte or every version was personally authored by him
```

VGMTrans identifies *Chrono Cross* as AKAO PS1 version 3.2, alongside later Square titles such as *Legend of Mana*, *Front Mission 3*, *Vagrant Story*, and *Final Fantasy IX*.

For AKAO v3.2 it exposes a substantial source-level event vocabulary including:

- program change
- volume / expression / pan and fades
- pitch slides
- octave and transpose
- ADSR parameter changes and mode controls
- vibrato / tremolo / pan LFO
- reverb
- noise
- pitch modulation
- loops
- slur / legato
- tuning
- portamento
- pitch side-chain
- pitch-to-volume side-chain

It also reconstructs later-AKAO instrument/drum structures including articulation IDs, key regions, ADSR-related values, tuning, sample mappings, loop points, and region-level routing data.

These are parser/reverse-engineering observations, not automatic historical source truth. VGMTrans itself contains explicit uncertainty/TODO comments in some mappings, so every field should retain parser confidence and runtime corroboration status.

## Ryo Yamazaki is a different provenance layer

Yamazaki is credited as synthesizer programmer/operator rather than sound programmer.

Secondary translations of the original soundtrack liner notes describe Mitsuda consulting Yamazaki on the practical realization of the score, including:

- solving the guitar-sound problem that Mitsuda regarded as central to the soundtrack's character;
- altering the realization of the Snakebone/Viper Manor material when additional sound memory was available.

These accounts should be retained as documentary evidence with their translation provenance. They are useful because they indicate that synthesizer programming was not merely mechanical data entry. It participated in timbre, sample/instrument realization, and platform adaptation.

Therefore the project should preserve a possible human transformation boundary like:

```text
Mitsuda musical intent / composition / arrangement
-> Yamazaki synthesizer realization decisions
-> Akao driver / sequence execution system
-> PlayStation SPU physical realization
```

This is a provenance hypothesis for organizing evidence, not a claim that every cue passed through exactly one person in a perfectly serial workflow.

## Why this matters to source-native enhancement

The human role split gives the project a better test for whether a hardware-era property is an unwanted implementation ceiling or part of the work's identity.

A simplistic rule such as:

```text
PlayStation limitation -> remove limitation
```

would be unsafe.

Some audible properties may reflect:

- Mitsuda's authored musical choice;
- Yamazaki's deliberate realization/timbre choice;
- AKAO driver semantics chosen or exploited musically;
- unavoidable SPU memory/rate/voice constraints;
- emergent interactions among all four.

The enhanced renderer should relax only the last category automatically, and even then only when identity survives. The other categories require evidence.

This strengthens the existing historical guardrail:

```text
constraint != unwanted artifact
```

and adds a more useful question:

```text
which human/technical transformation introduced this audible property?
```

## Chrono Cross vertical slice

The strongest prospective same-work route is now:

```text
PSF1 object / library graph
-> effective PS-X executable memory
-> AKAO v3.2 sequence object
-> AKAO track/event trajectory
-> AKAO instrument/articulation/sample mapping
-> runtime driver state
-> SPU register/memory commands
-> SPU physical voice/resource graph
-> decoded ADPCM / noise trajectory
-> pitch-modulation and envelope evolution
-> reverb / mix realization
-> persistent musical-part inference
-> human musical interpretation
```

At each arrow, retain which source supports the transformation:

```text
PSF/xSF loader
VGMTrans parser hypothesis
runtime execution trace
SPU emulator/hardware semantics
documentary human provenance
music-analysis inference
```

Do not let one source silently stand in for another.

## High-value correspondence test: AKAO pitch modulation

VGMTrans's later AKAO event vocabulary contains explicit pitch-modulation and pitch-side-chain controls.

Independent PlayStation SPU implementation evidence shows that an SPU voice can have pitch modulation enabled such that the preceding physical voice's live output affects the current voice's sample step.

This creates a particularly valuable future falsification test:

```text
AKAO pitch-mod / side-chain event
-> driver allocation decision
-> SPU pitch-modulation-enable bit(s)
-> concrete predecessor -> target voice edge
-> time-varying effective target sample step
```

Do not assume those layers correspond one-to-one merely because the names sound similar.

If a bounded Chrono Cross cue demonstrates the full route, it would be one of the strongest source-to-hardware correspondences in the project.

## High-value correspondence test: synthesizer realization

A separate research question concerns Yamazaki's realization layer.

Where a cue has documentary evidence of deliberate sound-realization work, compare:

```text
AKAO instrument/articulation/sample structure
+ SPU memory/sample allocation
+ tuning / envelope / loop / modulation choices
```

against the audible result.

The goal is not to identify “Ryo Yamazaki fingerprints” from technical similarity.

The goal is to document which machine-level choices plausibly implement an independently documented realization goal.

Technical similarity alone must never become personnel attribution.

## Evidence states

Use distinct confidence labels for claims such as:

```text
credited role
format attribution
parser-recognized AKAO object
runtime-observed driver behavior
SPU-observed physical behavior
documented realization intent
musicological interpretation
```

Examples:

```text
Ryo Yamazaki credited as synthesizer programmer
    -> direct/strong credit evidence

Minoru Akao credited as sound programmer
    -> direct/strong credit evidence

AKAO lineage named after / developed by Minoru Akao
    -> strong reverse-engineering/documentary attribution, not original-source proof

Chrono Cross uses AKAO v3.2
    -> parser/reverse-engineering evidence, to be corroborated against the actual PSF runtime image

specific AKAO side-chain event maps to specific SPU pitch-mod edge
    -> unknown until runtime correspondence is demonstrated
```

## Next tests

1. Search the reconstructed Chrono Cross PSF memory for exact `AKAO` signatures and classify candidate objects without relying on album tags.
2. Run VGMTrans's v3.2 structural expectations against those bytes and record every exact match, mismatch, heuristic, and unknown field.
3. Resolve one bounded sequence's instrument/articulation/sample dependencies.
4. Once runtime execution exists, trace one note/event through AKAO driver state to SPU allocation.
5. Deliberately search for AKAO pitch-modulation / pitch-side-chain events and test whether they produce the expected SPU causal state.
6. Keep Yamazaki documentary realization evidence separate from technical authorship attribution.
7. Use the result to refine the enhanced-renderer question: which limitations are implementation ceilings, and which machine choices are part of the realized work?

## Stop conditions

Stop rather than guess if:

- “synthesizer programmer” and “sound programmer” are collapsed into one role;
- the `AKAO` name is treated as proof of sole authorship by Minoru Akao;
- VGMTrans's heuristic fields are promoted to exact source truth;
- a high-level AKAO event is equated with an SPU register mechanism without runtime evidence;
- an SPU sample/envelope choice is attributed to Yamazaki merely because he is credited on the project;
- a hardware limitation is removed before determining whether the realized work deliberately depends on it;
- technical similarity is used as a personnel-attribution shortcut.

Correction outranks coherence.
