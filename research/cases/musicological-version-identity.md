# Musicological version and work identity pressure pass

## Status

Research input for the provenance-aware musical execution model.

This pass focuses on work identity, versions, arrangements, ports, editions, transcriptions, prototypes, source witnesses, reuse, similarity, derivation, transmission, and attribution.

## Question

Game-music research routinely encounters several artifacts that are related but not identical:

```text
prototype sequence
final sequence
regional revision
console port
arcade port
SPC/VGM capture
MIDI transcription
soundtrack album arrangement
rendered audio
reconstructed score
```

What can VGM Tooling legitimately claim about their relationship, and where should those claims live?

## Core result

Three questions must remain distinct:

```text
artifact identity
!= musical similarity / transformation
!= musicological work/version identity
```

The first is source truth. The second can be a structural/acoustic analytical result. The third is a contextual/historical claim that may require external evidence and does not fit cleanly into source representation, musical structure, auditory interpretation, or listener response.

This pass therefore supports one cross-cutting semantic layer:

```text
semantic_layer::musicological_context
```

The layer is not another stage in the source-to-listener pipeline. It links artifacts, performances, structures, documentary evidence, and external scholarship across that pipeline.

No new generic node or edge kind is justified yet. Initial musicological claims can use provenance-bearing `analysis_feature` values with `claim_layer = musicological_context`, while existing graph relations such as `references`, `derived_from`, `transforms`, `same_identity_as`, and `repeats` remain available when a concrete graph relation is justified.

## 1. Artifact identity is the easy layer

Exact source identity can use ordinary source evidence:

- file/content hash;
- container metadata;
- byte ranges;
- ROM/program address;
- captured file identity;
- exact sample or sequence object identity.

Example:

```text
prototype.vgm hash A
final.vgm hash B
```

proves the artifacts differ.

It does not answer whether they represent one work, one arrangement, one revision lineage, or merely similar music.

## 2. Musical difference should ignore serialization accidents

### Literature: MusicDiff

MusicDiff was developed for comparing multiple MEI sources/witnesses of a musical work. Its motivating distinction is directly applicable to VGM Tooling:

> a meaningful musical diff should compare modeled musical structures, not XML/plain-text serialization artifacts.

For game music, equivalent non-musical differences include:

- file/container offsets;
- compression/container representation;
- relocated driver data;
- reordered but semantically equivalent metadata;
- different capture packaging;
- different emulator/logging representation of equivalent musical behavior.

A cross-version VGM Tooling diff should instead be able to compare dimensions such as:

- note/pitch content where supported;
- rhythm/timing relationships;
- control flow and section structure;
- instrument/sample/patch relations;
- synthesis differences;
- arrangement density and orchestration;
- routing/effects;
- form/repetition;
- source-specific realization details.

## 3. GitHub observatory: visualizing multiple MusicDiff witnesses

Repository:

- `timeipert/visualize_musicdiffs`

The tool visualizes several sources as a network and as a Neighbor Joining tree from pairwise music-diff distances. It can show divergence/convergence over time and suggest phenomena such as contamination.

This contributes a useful negative lesson:

```text
distance network / inferred tree
!= proved historical genealogy
```

A distance matrix can expose patterns worth investigating, but chronology, copying direction, contamination, and source dependence remain musicological hypotheses unless independently supported.

For VGM Tooling this is highly relevant to prototype/final/port relationships.

## 4. Same-piece retrieval is not work-identity proof

### Literature: cover/version identification

Cover-song/version-identification research deliberately searches for invariants under transformations such as:

- key/transposition;
- tempo variation and local time warp;
- instrumentation/timbre change;
- structural editing;
- performance variation.

Useful research includes tonal/chroma alignment, transposition/time-warp invariant geometric matching, recurrence/structural-similarity methods, and melodic similarity under ornamentation.

These are useful because ports and arrangements can preserve musical identity while changing synthesis or realization dramatically.

But the output of such a system is a **similarity/classification result**.

It is not documentary proof that two artifacts are historically the same work.

### GitHub observatory: Essentia CoverSongSimilarity

Repository:

- `MTG/essentia`

The `CoverSongSimilarity` algorithm consumes a binary chroma cross-similarity matrix and uses local alignment to produce a cover-song similarity distance.

That implementation makes the representation dependency visible:

```text
audio
→ chroma representation
→ cross-similarity matrix
→ local alignment
→ similarity distance
```

The distance is therefore tied to the chosen representation/alignment method. VGM Tooling should preserve such algorithm/model provenance rather than turning a distance into a universal identity fact.

## 5. Similarity is multidimensional

Two artifacts can be:

- melodically near-identical but reharmonized;
- harmonically similar but rhythmically altered;
- structurally identical but resynthesized;
- timbrally similar but compositionally unrelated;
- the same sequence compiled for different hardware;
- the same work with substantially changed arrangement;
- a quotation/reuse rather than a version;
- a derivative arrangement with added/deleted sections.

Therefore a single `similarity = 0.87` is usually an impoverished claim.

VGM Tooling should prefer a layered comparison profile when evidence permits:

```text
source identity
program/control-flow similarity
performance-event similarity
instrument/sample/synthesis similarity
structural/form similarity
acoustic similarity
```

Each comparison must say which representation, invariances, and unavailable features were used.

## 6. Work identity is contextual

The philosophy/musicology literature asks whether a musical work coincides with a score, performance, recording, or some more abstract object. Digital music makes that ambiguity more visible rather than solving it.

For VGM Tooling, the practical rule is simpler:

> `work identity` is a musicological relation/claim whose meaning and evidence source must be explicit.

Possible evidence can include:

- explicit game/source identifiers;
- archival catalog or soundtrack documentation;
- composer/arranger documentation;
- sequence filenames or internal driver tables;
- release chronology;
- prototype/final provenance;
- matching authored source;
- structural similarity;
- shared samples/patches;
- external scholarship.

The strength of those routes differs.

A shared external catalog work ID can be exact relative to that catalog while still carrying `external_annotation` provenance. A same-work claim inferred only from musical similarity remains a hypothesis.

## 7. Version, arrangement, derivation, and reuse are not synonyms

Keep separate when the evidence permits:

```text
same artifact
same source object
same work
same version
same arrangement
revision of
port of
arrangement of
derived from
quotes/reuses
structurally similar to
stylistically similar to
```

The project should not mint a large permanent relation taxonomy before concrete cases require it. Existing graph edges plus relation attributes are enough for initial experiments.

The key obligation is to preserve which question is being answered.

## 8. Historical genealogy requires external evidence

A similarity tree or sequence of file timestamps can suggest a development path, but neither proves chronology or causality by itself.

For prototype/final/port research distinguish:

```text
artifact timestamp / version metadata
        source evidence

musical/synthesis differences
        derived comparison

A likely precedes or derives into B
        historical/musicological hypothesis
```

External release records, source-control history, developer documentation, archival material, or validated internal version markers can strengthen the historical claim.

## 9. Attribution is even stricter

Style-based composer/arranger attribution can use features such as:

- melodic/rhythmic distributions;
- harmony;
- form;
- orchestration;
- patch/sample choices;
- driver/tool habits;
- synthesis programming.

But recent review literature emphasizes methodological weaknesses and the need for strong validation, balanced metrics, cross-validation, and musicological validity.

Therefore:

```text
stylistic similarity
!= arranger identity
!= composer identity
```

This continues the existing Sonic 3 attribution law.

## 10. Layer consequence

The pass supports the following distinction:

```text
source_representation
  exact artifact/witness identity

musical_structure / acoustic_realization
  measured musical or rendered similarity

musicological_context
  work/version/arrangement/derivation/transmission/attribution claims
```

`musicological_context` is intentionally cross-cutting rather than a downstream stage after listener response.

Potential first analysis features include:

- `same_work_identity`
- `same_version_identity`
- `same_arrangement_identity`
- `revision_relation`
- `port_relation`
- `documented_attribution`
- `release_chronology_relation`

Each must carry its evidence/provenance and may be present, unknown, unavailable, or not applicable.

## 11. What this pass does not justify

Do not add:

- a universal ontology of musical works;
- a single scalar similarity as work identity;
- an automatic historical genealogy from similarity;
- automatic composer/arranger identity from style;
- a requirement that every source belong to a named canonical work;
- a new node/edge kind for each versioning word;
- a cover-song neural model as a playback dependency.

## 12. Immediate executable control

The next regression should prove that two exact, different source artifacts can simultaneously have:

1. exact distinct artifact identities;
2. a derived structural similarity measurement;
3. a hypothetical same-work relation based only on musical similarity;
4. a stronger externally documented same-arrangement/work claim with `external_annotation` provenance;
5. an attribution claim that remains separate from both identity and similarity;
6. unchanged source evidence underneath all of those claims.

If `analysis_feature` plus the existing graph can represent this after adding `musicological_context`, no new generic relation type is required.

## Primary implementation observatories checked

- `timeipert/visualize_musicdiffs`
- `MTG/essentia` `CoverSongSimilarity`
- existing MEI/music21/Humdrum/digital-musicology sources from prior passes

## Literature families checked

- MusicDiff / source-witness collation;
- computational musicology and source studies;
- identity of musical works;
- digital musicology data/source representation;
- cover/version identification;
- structural similarity;
- melodic similarity and transformation invariance;
- transposition/time-warp invariant retrieval;
- style-based composer/authorship attribution.

The durable result is the separation of artifact identity, analytical similarity, and musicological identity.
