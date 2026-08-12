# OpenMusic libraries

## Purpose

OpenMusic and its library ecosystem are research observatories for VGM Tooling. They are not a proposed runtime dependency stack and they do not define the project ontology.

Detailed repository archaeology and literature notes are preserved in `research/cases/openmusic-libraries.md`.

The durable transfer is narrower:

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

### Persistent musical identity is an inference problem

`Streamsep` and the broader voice-separation literature reinforce:

```text
physical execution slot
!= physical voice episode
!= persistent musical voice / part
!= auditory stream
```

The important transfer is the distinction between event evidence and grouping interpretation. Persistent-part hypotheses may use source-supported onset, duration, pitch, timbre/sample evidence, driver identity, continuity, allocation changes and provenance, but a grouping remains a hypothesis unless stronger source evidence proves it.

This frontier has already moved from research-only wording into the executable model regressions. It is no longer the project's next conceptual milestone.

### Pattern and motif analysis should use explicit projections

`LZ`, `Patterns`, `Profile` and `Morphologie` show useful ways to reason above literal event equality:

- variable-length repetition;
- patterns as processes;
- contour independent of absolute pitch;
- transformation and morphological comparison.

VGM Tooling should allow several analysis-specific feature projections over the same exact performance evidence without promoting any one projection into canonical truth.

```text
exact execution / performance graph
        ↓
analysis-specific projection
        ↓
repetition / contour / motif / phrase hypothesis
```

### Rhythm quantization must not rewrite exact timing

`RQ` reinforces the distinction between source/performance time and authored/notated time. A rhythm reconstruction may propose one or more authored-time hypotheses plus mappings back to exact execution time, but it must never overwrite source ticks, driver clocks or sample coordinates.

### Constraint programming is a reconstruction formalism, not an ontology

`Situation`, `Clouds`, `Cluster Engine`, `OMGecode` and related work support this decomposition:

```text
known evidence
+
unknown variables
+
constraints
+
objective / preference
+
search
=
candidate realization
```

That is useful for source-conditioned reconstruction, but the solver and its candidate are not canonical state. A higher-quality realization may relax historical storage/bandwidth limits while preserving proved notes, timing, control flow, modulation, instrument/sample relationships, deliberate effects and provenance boundaries.

### Spectral analysis/resynthesis is an intermediate representation

`OM-pm2`, `OM-SuperVP`, `OM-Pursuit` and related literature support partial, transient, residual and spectral-envelope descriptions between source synthesis and final PCM.

```text
exact source waveform / synthesis render
        ↓
partial + transient + residual / spectral representation
        ↓
source-conditioned realization candidate
```

Such a representation may be useful for enhancement without implying exact recovery of information that never survived the source.

### Symbolic structure can control synthesis without becoming synthesis state

`OMChroma`, `OM2Csound` and related systems reinforce:

```text
musical object / structure
        ↓
control / transformation
        ↓
synthesis object / running voice
        ↓
acoustic realization
```

The levels remain related but distinct.

### Spatial scenes and orchestration remain projections/hypotheses

`OM-Spat` shows that source identities and trajectories can be represented separately from a spatial renderer. `OM-Orchidee` shows that a target plus an allowed realization vocabulary can generate candidate orchestrations.

For VGM Tooling:

- source-supported spatial/routing evidence may be exposed;
- hardware-channel identity does not reveal authored 3D truth;
- a candidate orchestration or modernization is not evidence of historical composer intent.

## Current project consequence

The earlier OpenMusic pass ended with persistent musical identity as the immediate research frontier. That statement is now historical.

Since then VGM Tooling has added or pressure-tested:

- competing persistent-part hypotheses;
- source-relative analysis-feature availability;
- theory-level and perceptual alternatives over unchanged lower evidence;
- listener-response context;
- musicological work/version/attribution claims;
- timbre versus instrument identity;
- role-relative attribution;
- exact programmed expression versus higher musical interpretation;
- a synchronized song-level reasoning target.

Therefore the current OpenMusic contribution is **supporting machinery for the upper musical-analysis and reconstruction layers**, not the next project roadmap item.

The next high-value experiment is the end-to-end real-song control described in `research/cases/retro-composition-programming-listening.md`: traverse from unusually well-documented authored/programmed source to a coherent whole-song account while preserving evidence routes all the way down.

## What this research does not justify

Do not:

- embed OpenMusic in normal playback;
- make MIDI, notation or chord sequences canonical;
- create an OpenMusic-specific graph layer;
- promote hardware channels into persistent musical parts;
- treat spectral resynthesis as exact restoration;
- make one solver/MIR algorithm the architecture;
- move chip-specific implementation into libaural or Omniphony.

The common model remains project-owned and small. New shared primitives are added only when real source families force the same distinction.

## Sources

Primary library repositories inspected include:

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

Literature inspected alongside them includes musical stream/voice separation, rhythm transcription, musical constraint programming, computer-assisted orchestration, partial tracking, spectral envelopes, additive-plus-residual analysis/synthesis and source-filter representations.

See `research/cases/openmusic-libraries.md` for the evidence trail and `docs/music-representation-systems.md` for the broader representation model.
