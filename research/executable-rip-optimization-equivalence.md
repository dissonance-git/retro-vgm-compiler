# Executable-rip optimization as program slicing

## Status

Research input for xSF optimization, corpus provenance, behavioral equivalence, and future rip-validation tooling.

## Question

When a GSF/2SF/USF/PSF-family set is optimized into a small shared library plus mini files, what has actually been proven?

Not that the removed bytes are universally irrelevant.

The stronger model is:

```text
original executable/runtime object
+ chosen songs / states / traces / slicing criterion
-> optimized executable slice
```

Optimization is therefore a program transformation whose correctness is scoped to a behavioral criterion.

Core law:

```text
smaller valid xSF object
!= behaviorally equivalent executable witness
```

and:

```text
not observed during optimization
!= universally unreachable
```

## 1. GSF supplies a concrete historical counterexample

### `loveemu/gsfopt` 2018 batch-optimization bug

The HCS64 announcement from May 31, 2018 warned that older `gsfopt`/`snsfopt` builds could unexpectedly wipe ROM data which was still needed, especially sample data.

The reported trigger was highly specific:

- song A references an address once;
- song B references the same address at least 255 times;
- merging their byte-reference counts can incorrectly turn that byte into `unused`.

Source discussion:

- https://hcs64.com/mboard/forumlong.php?showpage=28&showthread=

The exact source fix is preserved in `loveemu/gsfopt` commit:

- `eb1b3783a061ee0d3e2dae2ca30d3da8ffdb9326`
- `Fix the bug of batch optimization, which sometimes could unexpectedly wipe used bytes (#1)`

The bug was in `GsfOpt::MergeRefs`.

The pre-fix overflow branch performed:

```cpp
dst_refs[i] += 0xff;
```

instead of saturating with:

```cpp
dst_refs[i] = 0xff;
```

For an 8-bit destination count of `1`, overflow through the former operation can wrap to `0`.

Thus a byte which had *more* evidence of runtime use could become classified as *unused*.

The repaired code explicitly saturates at `0xff`.

This is an unusually clean executable preservation lesson because the problem is not format validity or emulation accuracy. It is a liveness-analysis failure inside the optimizer.

## 2. Monotonic liveness is a required invariant

If an optimizer merges evidence from more observed executions, liveness should be monotone:

```text
live under observation set A
and
A subset B

must imply

live under observation set B
```

Adding another observed song, trace, or state must never make an already-live byte/object become dead merely because counters, bitsets, hashes, or reference maps overflow or collide.

Synthetic boundary:

```text
reference count 1
merged with
reference count 255

=> used / saturated
NOT unused
```

This exact regression is historically motivated rather than invented.

The invariant generalizes beyond byte counters:

- union of referenced ROM ranges;
- library-resource reachability;
- sample/bank inclusion;
- sequence dependency closure;
- code-page retention;
- save-state/RAM-range retention;
- observed callback/interrupt dependencies.

## 3. Nintendo DS independently repeats the optimizer problem

HCS64's 2SF tool history records an early SDAT optimizer bug that deleted files which were actually needed. The later update changed the approach toward deleting everything *except* objects proven needed.

Source:

- https://hcs64.com/mboard/forumlong.php?showpage=9&showthread=5327

The same history describes 2SF generation that can optimize an SDAT for selected tracks to remove SFX and unrelated material.

Sources:

- https://hcs64.com/mboard/forumlong.php?showpage=12&showthread=39
- https://hcs64.com/mboard/forum.php?showpage=96&showthread=5327

NCSF goes further toward an explicitly structure-oriented slice:

- retain selected SSEQ material;
- strip excessive SBNK material;
- rebuild/strip SWAR dependencies;
- omit unrelated objects where safe;
- historically omit unsupported/unneeded material such as streams.

Source:

- https://www.hcs64.com/mboard/forumlong.php?showpage=1&showthread=34052

Therefore:

```text
optimized SDAT
!= original complete game SDAT
```

and:

```text
successful selected-sequence playback
!= proof of whole-archive equivalence
```

This is one reason the planned Mario Kart DS 2SF/NCSF pair is scientifically valuable: the two formats intentionally preserve different amounts and kinds of runtime structure.

## 4. Program-slicing literature supplies the right vocabulary

Program slicing removes program statements or data that do not affect a chosen computation or slicing criterion.

The literature distinguishes important families:

### Static slicing

A static slice typically over-approximates what may affect the criterion across possible program inputs/states.

This tends to retain more than one observed execution needs, but aims for broader behavioral coverage.

### Dynamic / observation-based slicing

A dynamic slice is derived from one or more actual executions.

It can be much smaller, but it is only justified for the observed execution/input set unless additional analysis expands the claim.

A useful modern statement appears in work on quasi-static executable slices: static approaches over-approximate behavior across possible inputs, whereas dynamic approaches use execution and therefore under-approximate the slice for a subset of inputs.

Relevant literature includes:

- Mark Weiser's foundational program-slicing work;
- Xu et al., `A brief survey of program slicing` (2005);
- Canfora, Cimitile & De Lucia, `Conditioned program slicing` (1998);
- Binkley et al., `A formal relationship between program slicing and partial evaluation` (2006);
- Stiévenart, Binkley & De Roover, work on quasi-static executable slices (2021/2023);
- Beckert et al., `Using Relational Verification for Program Slicing` (2019).

These papers are not evidence about any console. They provide established terminology for the transformation problem exposed by the game-audio tools.

## 5. Executable-rip optimization is usually conditioned slicing

A game-music optimizer rarely asks:

> preserve every behavior of the original game binary.

Instead it asks something closer to:

> preserve the behavior required to play this selected set of songs under this rip harness/player/runtime model.

That is naturally expressed as a conditioned slicing problem.

Conceptually:

```text
program P
condition C = selected songs + initialization/runtime-state assumptions
criterion O = required music/audio observations

slice S
such that
P and S agree on O for executions satisfying C
```

The condition must be part of provenance.

Without it, a statement like `these bytes are unused` is underspecified.

## 6. Trace coverage is not reachability proof

This matters because the project has already established several runtime-dependent systems:

### MP2K

`MEMACC` can branch according to shared game memory.

Therefore one song can contain paths not exercised by one default rip state.

### Nintendo DS SSEQ

Random, variable, comparison, conditional, open-track, call, and loop commands can change runtime behavior.

### Nintendo 64 MML

Branches, loops, calls, memory I/O, and variables can affect execution.

### GameCube BMS/JAudio

Runtime-created child tracks, callbacks, ports, registers, random operations, and conditional branches can change the execution topology.

### Dynamic/adaptive game music

Game state can enable/mute layers, select branches, or change runtime parameters.

Thus:

```text
byte/resource absent from one trace
!= unreachable from the source program
```

If an optimizer intentionally preserves only one path, that can be valid, but it must be labeled as a path-conditioned slice rather than a complete source preservation object.

## 7. Runtime environment is part of the slicing criterion

Existing HCS64 evidence shows that executable-rip behavior can depend on:

- initialization arguments;
- timer configuration;
- interrupt/DMA timing;
- save-state RAM/register state;
- song selector plus additional game-state variables;
- emulator/runtime behavior tolerated by historical rips.

See:

- `research/rip-fidelity-projection-boundaries.md`

Therefore two optimizers can retain the same apparent code/data and still make different liveness decisions if they execute under different machine models.

A future optimizer provenance record should identify the observation engine and relevant timing/runtime options.

## 8. Source-object provenance must survive optimization

For an optimized xSF set, ideally retain provenance linking:

```text
optimized byte/range/object
-> source byte/range/object
```

and retain transformation metadata such as:

- original/pre-optimization object digest when available;
- optimizer name/version/commit;
- optimization mode;
- selected songs/subsongs;
- observation player/emulator/core;
- initialization state used;
- number and identity of traces;
- dependency objects retained/removed;
- transformation output digest;
- known optimizer limitations.

The canonical admitted optimized file remains immutable evidence once admitted.

This does not require committing copyrighted ancestor objects that were not supplied or authorized. A digest/transformation record can still preserve part of the provenance chain.

## 9. Behavioral equivalence must be observable-specific

It is too strong to demand bit-identical whole-machine execution after optimization if the transformation intentionally removes unrelated game systems.

Instead declare the observation set.

Possible music-rip observables include:

```text
sequence/control-flow trajectory
logical note events
instrument/sample selections
memory reads relevant to audio
sound-chip register writes
physical voice trajectories
mixed PCM output
loop point and loop-state behavior
runtime errors / missing-resource accesses
```

Then state what equivalence was actually tested.

Examples:

```text
exact device-write equivalence
exact logical-event equivalence
sample-identical PCM output
PCM within declared numerical tolerance
same selected cue but broader runtime state not tested
```

Do not collapse these into one `optimized = correct` bit.

## 10. Differential optimizer test contract

For any future optimizer or slice builder owned by Game Music Interpreter, require at least:

### A. Monotonic liveness

Adding observations cannot delete previously live dependencies.

### B. Idempotence

```text
optimize(optimize(P, C), C)
```

should not continue deleting semantically required content or change the declared observations after reaching the optimizer's fixed point.

Byte identity is preferable but not universally required if deterministic repacking changes layout; behavioral/output identity remains required under the declared criterion.

### C. Dependency closure

Every retained executable path must retain the data/code/resources it can access under the declared condition.

### D. Order independence where intended

If a batch optimizer claims set-union semantics, processing song A then B should produce the same liveness set as B then A.

### E. Duplicate-observation stability

Observing one song many times must not make a dependency disappear through counter overflow or weighting artifacts.

Historical boundary:

```text
A references byte once
B references byte 255 times
```

### F. Negative-control deletion

Unused synthetic data should actually be removable, proving the optimizer is not merely preserving everything.

### G. Differential execution

Run pre- and post-optimization objects under the same declared runtime and compare the strongest available observation trajectory.

## 11. Corpus consequence

The current corpus preservation law correctly keeps admitted runnable files immutable.

A later corpus update should additionally distinguish, when known:

```text
original supplied object
optimized executable-rip object
optimizer/transformation provenance
pre-optimization ancestor digest
behavioral-equivalence evidence
```

Do not edit `tests/CORPUS.md` merely from this research note while concurrent GSF/NCSF corpus work may be touching it. Fold the requirement in when that work lands and the exact current file can be re-read.

## 12. Enhancement consequence

An enhanced renderer must not use a historically optimized rip as unquestioned proof of the entire authored sound program.

Optimization may have intentionally removed:

- unused SFX;
- alternate dynamic layers;
- unreachable-under-default-state branches;
- samples used only in another cue;
- game callbacks or control state;
- non-selected sequence objects.

If enhancement/reconstruction needs information outside the slice, return to a stronger ancestor/source observatory rather than hallucinating the missing structure.

## 13. Stop conditions

Stop rather than overclaim if:

- an optimized xSF is called equivalent merely because it parses and plays;
- one observed trace is treated as proof of global unreachability;
- adding more traces can make prior live content dead;
- optimizer counters can overflow/wrap without a liveness regression;
- batch result depends on song ordering when union semantics are claimed;
- NCSF/2SF stripped SDAT is called the complete original SDAT;
- pre/post optimization are compared through different emulator/timing settings;
- playback-only equivalence is promoted to source-program equivalence;
- an omitted dynamic branch is reconstructed from guesswork;
- optimizer provenance is discarded when the output enters the permanent corpus.

Correction outranks coherence.
