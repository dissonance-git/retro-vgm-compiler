# OpenMusic libraries

## Purpose

OpenMusic was already part of the VGM Tooling research stack before the library-catalog pass. This document records the durable consequences of mining its wider library ecosystem so that future work can resume from repository state rather than reconstructing the result from conversation history.

Detailed repository archaeology and literature notes are preserved in `research/cases/openmusic-libraries.md`.

The OpenMusic libraries are research observatories, not a proposed runtime dependency stack. VGM Tooling keeps exact source, driver, device, sample, and execution evidence canonical and uses higher-level analysis only where the lower source stops answering the musical question.

```text
exact source / driver / device evidence
        ↓
common musical execution model
        ↓
higher musical inference
        ↓
optional reconstruction / rendering experiments
```

## Durable findings

### Persistent musical identity is an inference layer

`Streamsep` separates polyphonic note events into candidate monophonic streams using temporal and perceptual-pitch proximity, while allowing register crossings and preserving the input events. This independently reinforces the current VGM Tooling identity law:

```text
physical execution slot
!= physical voice episode
!= persistent musical voice / part
```

A future part/voice grouping stage may use exact or derived onset, duration, pitch, timbre/sample evidence, driver identity, overlap, continuity, allocation changes, and provenance confidence. The result remains a hypothesis unless authored or driver-level evidence proves it.

The important transfer is the separation between **event evidence** and **grouping interpretation**, not the OpenMusic implementation itself.

### Pattern and motif analysis should be feature-projected

`LZ`, `Patterns`, `Profile`, and `Morphologie` show several compatible ways to reason above literal event equality:

- variable-length repetition and context models;
- patterns as processes rather than pre-expanded event lists;
- melodic contour/profile independent of absolute pitch;
- morphological comparison and transformation of symbolic or numeric sequences.

For VGM Tooling, analysis should therefore be allowed to project the same exact performance graph into different feature spaces without promoting any one projection into canonical truth.

```text
exact execution / performance graph
        ↓
analysis-specific feature projection
        ↓
repetition / contour / motif / phrase hypothesis
```

This is particularly relevant when a musical line survives transposition, ornamentation, instrument changes, or hardware reallocation.

### Rhythm quantization must not rewrite exact timing

`RQ` ranks candidate rhythmic transcriptions using both fit to observed timing and structural/readability complexity. This reinforces the existing separation between source/performance time and authored/notated time.

A rhythm reconstruction should therefore create one or more authored-time hypotheses plus explicit mappings back to exact execution time. Quantization must never overwrite source ticks, driver clocks, or sample coordinates.

### Constraint programming is a reconstruction formalism, not a new ontology

`Situation`, `Clouds`, `Cluster Engine`, `OMGecode`, and related work demonstrate a useful decomposition:

```text
known variables / exact evidence
+
unknown variables
+
constraints
+
preference or objective
+
search
=
candidate realization
```

This is a promising formulation for later source-conditioned reconstruction. For example, a higher-quality realization may be allowed to relax historical storage or bandwidth limitations while preserving proved notes, timing, control flow, modulation, instrument/sample relationships, deliberate effects, and provenance boundaries.

The solver is not canonical state. Candidate solutions remain projections or hypotheses supported by the common graph.

### Spectral analysis and resynthesis belong below musical identity but above raw PCM

`OM-pm2` exposes partial tracking and additive resynthesis; `OM-SuperVP` exposes spectral analysis, processing, synthesis, and parameterized time-frequency transformations; `OM-Pursuit` and related dictionary/decomposition work provide another route for structured sound models.

The literature around sinusoidal/partial tracking, spectral envelopes, additive-plus-residual models, and source-filter sound models supports a reusable intermediate idea:

```text
exact source waveform / synthesis render
        ↓
partial + transient + residual / spectral-envelope model
        ↓
source-conditioned high-quality realization
```

These models are candidate analysis/resynthesis representations, not evidence that missing pre-compression source audio can be recovered exactly. Any reconstructed information that was never present in the source must remain conditional and reversible.

### Symbolic structure can control synthesis without becoming synthesis state

`OMChroma`, `OM2Csound`, and OpenMusic's synthesis-control ecosystem reinforce the separation between musical structure, control parameters, synthesis definitions, and rendered audio.

This matches VGM Tooling's existing graph direction:

```text
musical object / structure
        ↓
control / transformation
        ↓
synthesis object / running voice
        ↓
acoustic realization
```

### Spatial scenes are structured musical projections

`OM-Spat` represents sound sources and their spatial trajectories as scene data and can render those scenes through a separate spatialization kernel. The useful lesson for VGM Tooling is that spatial attributes and trajectories can be explicit side information without becoming chip-channel identity or authored 3D truth.

This is primarily downstream research for libaural and Omniphony. VGM Tooling should expose only source-supported musical/source identities and confidence, not invent spatial coordinates from implementation channels.

### Computer-assisted orchestration is a hypothesis generator

`OM-Orchidee` separates a target sound, an available orchestra/database, extracted features, and candidate orchestration solutions. The transferable idea is useful for future reconstruction experiments:

```text
source-conditioned target features
+
allowed realization vocabulary
→ candidate realization
```

A candidate orchestration or modernized synthesis realization is not evidence of composer intent. It is an experimentally testable alternative realization constrained by the surviving work.

## What this pass does not change

The pass does **not** justify:

- embedding OpenMusic in normal playback;
- making MIDI, notation, or chord sequences canonical;
- creating an OpenMusic-specific graph layer;
- promoting hardware channels into persistent musical voices;
- treating spectral resynthesis as exact restoration of information that never survived;
- making one constraint solver or MIR algorithm the project architecture;
- moving chip-specific implementation into libaural or Omniphony.

The common model remains project-owned and small. New generic primitives are added only when multiple real source families force the same distinction.

## Immediate research frontier

The highest-value next experiment is persistent musical identity because both current source families already provide unusually strong lower-layer answer keys.

```text
Genesis / GEMS
logical track may migrate across physical YM/PSG resources

SPC / S-DSP
one physical voice episode may change runtime sample source

OpenMusic Streamsep + voice-separation literature
persistent voice assignment is a separate grouping problem
```

The next implementation should therefore test a **candidate part/voice grouping analysis** above existing physical episodes and conservative performance events. It should:

1. consume existing graph evidence rather than MIDI;
2. retain every source event and physical episode unchanged;
3. produce hypotheses, not identities by fiat;
4. allow several competing groupings;
5. use exact driver identity as a stronger constraint whenever available;
6. survive register crossing and hardware reallocation controls;
7. expose why two events were grouped through provenance-bearing features or relations;
8. remain offline/analysis-side until a bounded realtime use is proven.

Only after this survives Genesis, SPC, tracker/driver controls, and the literature should `part` promotion become a common-model operation.

## Sources

Primary OpenMusic/library repositories inspected in this pass include:

- `openmusic-project/Streamsep`
- `openmusic-project/LZ`
- `openmusic-project/Patterns`
- `openmusic-project/Profile`
- `openmusic-project/Morphologie`
- `openmusic-project/RQ`
- `openmusic-project/Situation`
- `openmusic-project/Clouds`
- `PHRaposo/Cluster-Engine-Library-for-OpenMusic`
- `openmusic-project/OM-pm2`
- `openmusic-project/OM-SuperVP`
- `marleynoe/OM-Pursuit`
- `openmusic-project/OMChroma`
- `openmusic-project/OM-Spat`
- `openmusic-project/OM-Orchidee`
- `openmusic-project/Repmus`

Literature inspected alongside these repositories includes work on musical stream/voice separation, rhythm transcription and k-best quantization, musical constraint programming, computer-assisted orchestration, partial tracking, spectral envelopes, additive-plus-residual analysis/synthesis, and source-filter representations of musical timbre.

See `research/cases/openmusic-libraries.md` for the detailed evidence trail and `docs/music-representation-systems.md` for the broader representation model.
