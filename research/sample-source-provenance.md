# Sample-source provenance

## Status

Research input for sample attribution, instrument-source claims, library provenance, and source-native reconstruction.

This pass uses the HCS64 VGM/Others Instrument Source Thread as a community discovery surface, then pressure-tests what kinds of claims can actually be promoted into Game Music Interpreter.

The central law is:

```text
sample similarity
!= sample derivation
!= historical source identity
!= instrument identity
```

Those claims may support one another, but they require different evidence.

## 1. Why this matters

Game-audio samples are frequently:

- resampled;
- truncated;
- looped differently;
- normalized or attenuated;
- pitch-shifted;
- filtered;
- compressed or encoded;
- converted between signed/unsigned PCM;
- converted to BRR, ADPCM, or another hardware format;
- combined with envelopes, reverb, or other processing;
- extracted from a larger commercial synthesizer or sample-library program.

Therefore an original-library sample and its game representation may be audibly obvious relatives while sharing no byte-for-byte identity.

The reverse failure is also possible: two short generic waveforms can resemble each other strongly without one being historically derived from the other.

## 2. Community discovery is valuable but heterogeneous

The long-running HCS64 `The VGM/Others Instrument Source Thread` is an unusually rich discovery index for commercial synthesizers, sample CDs, modules, workstation patches, and other source candidates used in games.

Source:

- https://www.hcs64.com/mboard/forumlong.php?showthread=24937

Its posts contain several qualitatively different claim types, including:

- direct comparison of a raw in-game sample against a physical/module source;
- claims that a sample is the `same exact sample`;
- named source-library and preset claims;
- historical/personnel claims about hardware used by a composer;
- tentative `I think`, `maybe`, or `probably` identifications;
- requests for help based only on hearing similarity.

Those distinctions must survive ingestion.

A community spreadsheet or forum post is excellent candidate-generation evidence. It is not automatically exact provenance.

## 3. Provenance ladder

Use the strongest available evidence class and preserve weaker supporting evidence separately.

### Level A: exact encoded-byte identity

Strongest when both objects are already in the same representation.

```text
bytes(game_object) == bytes(candidate_source_object)
```

Record:

- cryptographic digests;
- exact byte ranges;
- container/encoding context;
- whether headers/metadata were excluded;
- provenance of both objects.

This proves object identity in that representation.

It does not by itself prove the historical direction of copying.

### Level B: exact identity after a known reversible transform

Examples:

```text
known endian conversion
known signed <-> unsigned PCM mapping
known lossless container extraction
known deterministic deinterleave
known reversible codec/container step
```

If applying a fully specified transform to one artifact produces the other exactly, record:

```text
source object
--transform T-->
game object
```

with code/version/parameters for `T`.

This is strong derivation-compatible evidence, but historical direction still requires chronology or documentary evidence.

### Level C: exact/near-exact identity under a bounded lossy hardware transform

Many game systems require transformations such as:

- sample-rate conversion;
- BRR/ADPCM encoding;
- bit-depth reduction;
- deterministic truncation;
- fixed filtering;
- loop-point adaptation.

A strong test is not merely `the waveforms correlate`.

Prefer:

```text
candidate source
-> explicitly modeled platform/toolchain transform family
-> predicted game representation
-> compare with actual game representation
```

Record:

- transform family;
- searched parameter range;
- best parameters;
- residual/error metric;
- whether the transform was historically available/plausible;
- uniqueness against competing candidates.

If one bounded transform reproduces the game artifact exactly, the result can be very strong.

If several candidates fit, retain ambiguity.

### Level D: robust content-match / fingerprint evidence

Audio-fingerprinting and audio-forensics research demonstrates that related audio can often be recognized despite changes such as:

- resampling;
- pitch or speed variation;
- filtering/equalization;
- compression;
- noise;
- cropping/truncation.

Useful research families include robust/invariant audio pattern matching and perceptual audio hashing.

This is appropriate for candidate generation and support when exact inverse transformation is unavailable.

But:

```text
robust fingerprint match
!= proof of historical source
```

A robust matcher is intentionally invariant to transformations, which also means it discards information that could distinguish different historical paths.

Record the matcher, version, parameters, similarity score, tested candidate set, and false-positive controls.

### Level E: structural/acoustic feature match

Examples:

- waveform shape;
- spectral envelope;
- transient structure;
- periodicity;
- formants;
- loop-compatible region;
- relative harmonic pattern.

This is useful as supporting evidence, especially when the in-game sample is short or heavily transformed.

It remains a similarity claim unless a stronger transform/source route is established.

### Level F: documentary/historical evidence

Examples:

- composer/sound-programmer interview naming a module/library;
- project documentation;
- source file label;
- purchase/library inventory;
- credited synthesizer programmer describing the source;
- official SDK/toolchain material.

Documentary evidence can strongly establish that a library/device was available or used.

It does not automatically prove that every acoustically similar game sample came from that source.

Combine documentary and object-level evidence where possible.

### Level G: perceptual resemblance / community hypothesis

Examples:

```text
sounds like preset X
probably from library Y
I recognize this waveform
```

These are valuable search hints.

Store them only as hypotheses with source attribution.

Do not propagate them into canonical instrument/source metadata.

## 4. Historical direction is a separate claim

Even exact object correspondence does not automatically establish direction.

If:

```text
sample A == sample B
```

possible histories include:

```text
A -> B
B -> A
common earlier source -> A and B
both copied from public/vendor material
```

Direction should consider:

- release dates;
- library publication dates;
- development chronology;
- source-file timestamps only where trustworthy;
- interviews/documentation;
- known toolchain/library ownership;
- transformation asymmetry.

Represent:

```text
same_content_as
```

separately from:

```text
derived_from
```

until direction is earned.

The current graph already provides `same_identity_as`, `derived_from`, and provenance-bearing edges, so no new graph primitive is required yet.

## 5. Patch/program identity is above sample identity

A workstation patch or sound-module program can contain:

- multiple multisamples;
- key/velocity zones;
- envelopes;
- filters;
- LFOs;
- effects;
- tuning and gain relationships.

Finding one matching waveform does not prove that the complete game instrument is the same original patch.

Possible claims must remain separate:

```text
sample waveform source
multisample source
preset/program source
instrument definition source
rendered timbre source
```

A game may copy only one waveform from a commercial preset and reconstruct its envelope/tuning differently.

## 6. Same source does not imply same realization

Suppose the game uses a waveform from a known workstation.

The realized game sound can still differ because of:

- platform resampling;
- game-side envelope;
- pitch table;
- loop edits;
- hardware interpolation;
- mixer precision;
- reverb/effects;
- voice allocation or modulation.

Therefore a future enhanced renderer must not simply replace the game sample with the commercial source preset and call the result `original intent`.

The stronger experiment is:

```text
proven source waveform / multisample
+
proven game-side performance/synthesis transforms
-> candidate higher-fidelity realization
```

and compare that against the historical reference.

## 7. Negative controls are required

Sample-source matching is especially vulnerable to confirmation bias because researchers usually begin with a candidate they already suspect.

Every automated or semi-automated matcher should include controls such as:

- unrelated samples from the same library/device;
- nearby presets sharing source ROM material;
- same instrument family from competing libraries;
- transformed versions with incorrect sample rate/pitch;
- short generic transients or periodic waves likely to collide.

Report rank/uniqueness rather than only the winning candidate's score.

A result like:

```text
candidate X similarity = 0.98
```

is weak without knowing whether 40 other candidates score 0.97.

## 8. Multi-stage matching is preferable

A practical future source-identification pipeline can use several stages:

```text
exact hashes / byte windows
-> normalized PCM comparisons
-> bounded transform search
-> robust fingerprint candidate search
-> structural/acoustic comparison
-> documentary chronology check
-> human review
```

Each stage should preserve its own result rather than collapsing all evidence into one confidence number.

## 9. Literature pressure

Robust audio-matching literature shows why perceptual/content matching is useful but insufficient for historical attribution.

Examples surfaced in the SciSpace pass include work on:

- robust and invariant audio pattern matching;
- peak-pair audio fingerprints robust to resampling, pitch shifting, time stretching, EQ, compression, and noise;
- perceptual audio hashing;
- transformed-duplicate detection under resampling/filtering/cropping.

These methods are designed to answer roughly:

> Are these audio signals substantially related despite transformations?

Game Music Interpreter additionally needs:

> Which exact transformation path is supported, and what historical/source claim does that path justify?

Those are different questions.

## 10. Relationship to existing timbre research

See:

- `research/timbre-instrument-organology.md`

That pass established:

```text
authored instrument label
!= synthesis object identity
!= acoustic timbre
!= perceived source category
!= historical instrument identity
!= musical role
```

This pass inserts a provenance ladder specifically between external candidate sources and exact game-side synthesis objects.

## 11. Immediate test contract

Future sample-source claims should record at least:

```text
claim target
candidate source identity
candidate provenance
match evidence class
transform, if any
transform parameters/search bounds
object-level match metric
candidate-set/negative-control result
documentary evidence
chronology evidence
historical direction status
confidence
remaining ambiguity
```

Suggested direction states:

```text
unknown
same-content-only
compatible-source
probable-derived-from
strong-derived-from
exact-documented-derivation
```

These are research statuses, not new common graph enums unless repeated implementation pressure later earns them.

## 12. What not to do

Do not:

- treat an HCS spreadsheet row as exact source truth;
- identify a sample solely from a YouTube comparison;
- use a perceptual fingerprint as proof of historical direction;
- call two samples different merely because their encoded bytes differ;
- normalize/resample both candidates and discard the transform provenance;
- promote one matching waveform to full synthesizer-preset identity;
- replace a game instrument with a commercial preset merely because one source sample matches;
- assume the commercial library is upstream solely because its current digital copy has an older filesystem timestamp;
- hide multiple plausible candidates behind one confidence score.

## Highest-information future experiment

Choose a corpus object for which both of these are legally/locally available:

1. exact in-game sample bytes;
2. a candidate source-library sample supplied by the user or otherwise legitimately available for analysis.

Then perform:

```text
raw comparison
-> normalized PCM comparison
-> bounded resample/pitch/truncation/loop transform search
-> negative-control ranking
-> provenance report
```

The result should be able to say precisely one of:

```text
exact same object
exact under transform T
strong content match but transform unresolved
plausible similarity only
no support
```

without jumping directly to a historical instrument claim.

Correction outranks coherence.
