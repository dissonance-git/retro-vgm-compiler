# OpenMusic library observatory

This research owner records the OpenMusic-library evidence that pressure-tests VGM Compiler's musical representation. OpenMusic and its libraries are research inputs, not runtime dependencies and not an ontology source of authority.

The cross-system representation pressure test is [`music-representation-systems.md`](music-representation-systems.md). Durable semantics promoted from this evidence live in [`../../docs/architecture.md`](../../docs/architecture.md).

## Research question

Which established analysis, constraint, synthesis, resynthesis, orchestration, and spatialization methods expose distinctions that improve VGM Compiler without weakening source-native evidence?

The governing boundary is:

```text
exact source / execution evidence
        ↓
project-owned semantic relations
        ↓
analysis-specific projection
        ↓
hypothesis / reconstruction candidate
```

Higher analysis never rewrites the evidence beneath it.

## Persistent part identity

OpenMusic Streamsep and the broader voice/stream-separation literature reinforce that:

```text
physical voice episode
!= persistent musical part
```

Candidate grouping can use timing, overlap, pitch motion, register, source/sample/timbre continuity, validated driver identity, allocation changes, and provenance confidence. The output remains hypothesis-level unless authored or driver evidence establishes identity more strongly.

A useful VGM Compiler experiment should deliberately include cases where hardware-channel continuity conflicts with musical continuity, including allocation changes and crossing lines.

## Pattern, repetition, and transformation

LZ, Patterns, Profile, Morphologie, and related systems show that musical recurrence can be studied through temporary feature projections rather than one canonical token stream.

Useful projections may emphasize pitch intervals, rhythm, contour, duration, timbre, part-local behavior, or transformation relations. The projection is an analysis view over retained events, not a replacement for those events.

This supports motif and phrase questions such as:

- what survives transposition;
- what survives instrument or channel reassignment;
- which recurrences retain contour but change rhythm;
- which section returns preserve a relation while changing realization;
- which patterns are processes rather than expanded event lists.

## Rhythm quantization

RQ and related k-best rhythmic parsing reinforce a strong time boundary:

> A quantized rhythm is an authored-time hypothesis connected to observed timing. It is not permission to rewrite exact source, driver, device, or sample time.

Several candidate rhythmic interpretations may remain available with separate fit and complexity evidence.

## Constraint-based reconstruction

Situation, Clouds, Cluster Engine, and related constraint systems supply a useful reconstruction form:

```text
known evidence
+ unknown variables
+ preservation constraints
+ objective / preference
+ search
→ candidate realization
```

This is appropriate for bounded source-conditioned reconstruction when the invariants and relaxed dimensions are explicit. A successful search result remains a candidate realization unless independent evidence establishes historical truth.

Potential preservation obligations include notes, timing, authored modulation, source/sample identity, loop/control behavior, and deliberate device effects. Bandwidth or storage limits may be relaxed only when the experiment declares that relaxation.

## Spectral and timbral intermediates

Esquisse, OM-pm2, OM-SuperVP, OM-Pursuit, and related literature support analysis objects such as:

```text
partial / pitch trajectories
+ spectral envelope
+ transient structure
+ residual / noise structure
```

These can mediate between exact legacy source objects and higher-quality reconstruction experiments. They do not imply recovery of information absent from the surviving source.

External kernels such as pm2 or SuperVP are useful research oracles and prototypes. They are not required playback dependencies.

## Spatial scene representation

OM-Spat reinforces:

```text
source identity
!= spatial trajectory
!= spatial renderer
```

A source-aware scene may preserve explicit or hypothesized trajectories separately from the rendering engine. Physical channel number is not a spatial coordinate unless the source actually establishes that relation.

This distinction is relevant to the Omniphony boundary without making spatial presentation part of VGM Compiler's source ontology.

## Orchestration and target search

OM-Orchidee and related computer-assisted orchestration work distinguish:

```text
target
+ candidate resources
+ search constraints
→ candidate solution
```

For VGM Compiler, the transferable research question is whether a higher-resolution realization can satisfy source-supported musical and synthesis constraints. That is a reconstruction objective, not recovered creator intent.

## Current model consequences

This evidence does not justify another shared semantic layer. Existing project concepts such as persistent parts, patterns, sections, musical relations, transformations, projections, provenance, and exact/derived/hypothesis evidence are sufficient for the current experiments.

The observatory therefore imposes these obligations:

1. Exact source information outranks inference.
2. Analysis creates new relations or mappings instead of mutating evidence.
3. Physical voice and musical part remain separate identities.
4. Feature projections stay analysis-local and disposable.
5. Alternative analyses may coexist.
6. Constraint search states what is fixed and what is relaxed.
7. Spectral reconstruction remains conditional on retained evidence.
8. External systems remain research references unless a current implementation independently earns a dependency.

## Experimental priorities

The next discriminating uses of this evidence are:

1. persistent-part grouping over already-supported performance events;
2. adversarial controls where channel continuity and musical continuity disagree;
3. motif/contour experiments over the same retained event objects;
4. authored-time rhythm hypotheses with explicit mappings to exact time;
5. offline spectral or reconstruction experiments only where source identity and reference rendering are already stable.

## Evidence anchors

Primary ecosystem reference:
- OpenMusic library catalog, `openmusic-project.github.io/libraries`
- GitHub organization, `openmusic-project`

Representative literature retained as pressure evidence includes work by Szeto and Wong on polyphonic stream segregation; Rafailidis, Cambouropoulos and Manolopoulos on musical voice integration/segregation; Guiomard-Kagan and colleagues on voice/stream segmentation; information-theoretic and Lempel-Ziv musical modeling; Truchet and Codognet on musical constraint satisfaction; Sandred on polyphonic constraint solving; Maresz and IRCAM work on computer-assisted orchestration; and Rodet, Schwarz, Lazzarini, Timoney and others on partial/spectral analysis and resynthesis.

These sources motivate experiments. VGM Compiler's own source-backed tests decide which distinctions survive into shared contracts.