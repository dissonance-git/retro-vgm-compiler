# OpenMusic libraries

OpenMusic and its library ecosystem are research observatories for Game Music Interpreter. They are not runtime dependencies and do not define the project ontology. Detailed repository and literature notes live in `research/cases/openmusic-libraries.md`.

## Durable transfer

```text
exact source / driver / device evidence
        ↓
common musical execution model
        ↓
higher musical inference
        ↓
optional reconstruction / rendering experiments
```

The useful lessons are structural.

Persistent musical identity is an inference problem: a physical execution slot, a bounded voice episode, a persistent musical part, and an auditory stream are different identities. OpenMusic voice-separation work is therefore a useful comparison surface, but source-proved authored or driver identity outranks score-only grouping heuristics.

Pattern, contour, motif, and rhythm tools reinforce that analysis-specific projections should sit above exact performance evidence rather than replacing it. A quantized or notated rhythm is a hypothesis about musical organization, not permission to rewrite source ticks, driver clocks, or sample coordinates.

Constraint-based systems provide a useful reconstruction form:

```text
known evidence
+ unknown variables
+ constraints
+ objective or preference
+ search
→ candidate realization
```

The candidate remains a candidate. The solver is not canonical state.

Spectral and resynthesis libraries likewise supply useful intermediate representations for partials, transients, residual energy, and spectral envelopes. They may support source-conditioned enhancement without implying that information absent from the surviving source has been recovered exactly.

Spatial and orchestration tools reinforce another boundary: source identity and source-supported routing can be represented separately from a renderer, while candidate spatial scenes or orchestrations remain hypotheses rather than evidence of historical intent.

## Current role

The OpenMusic pass now supports the upper musical-analysis and reconstruction layers rather than defining the immediate frontier. Game Music Interpreter already has executable regressions for competing persistent-part hypotheses, source-relative feature availability, theory and perceptual alternatives, listener-response context, musicological identity, timbre versus instrument identity, role-relative attribution, and programmed expression versus higher interpretation.

The common model therefore stays small and project-owned. New shared primitives are added only when materially different source families force the same distinction.

## Guardrails

Do not make OpenMusic, MIDI, notation, chord sequences, spectral analysis, or any one solver the canonical representation. Do not promote hardware channels into musical parts. Do not treat reconstruction candidates as recovered historical source truth. Chip-specific implementation remains in the relevant source family rather than moving into libaural or Omniphony.

Primary comparison libraries include Streamsep, LZ, Patterns, Profile, Morphologie, RQ, Situation, Clouds, OM-pm2, OM-SuperVP, OM-Pursuit, OMChroma, OM-Spat, OM-Orchidee, and related OpenMusic work. See `docs/music-representation-systems.md` for the broader representation model.
