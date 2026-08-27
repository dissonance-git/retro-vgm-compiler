# VGM Compiler roadmap

This is the canonical current-state document for VGM Compiler. It owns what is implemented, what is active, and what comes next. Durable semantic law belongs in [`architecture.md`](architecture.md); the musical target belongs in [`musical-understanding.md`](musical-understanding.md).

If a status or priority is not represented here, do not infer it from older research notes, commit messages, or durable contracts.

## Implemented surface

The repository has working machinery across these levels:

```text
source/container reconstruction
→ device/runtime evidence
→ provenance and source/driver ancestry
→ persistent musical identity and performed pitch/timing evidence
→ motif / phrase / harmony / cadence / formal evidence
→ counterpoint / orchestration / creator-grammar evidence
→ source-native enhanced rendering and realtime spatial handoff
→ semantic projection / round-trip experiments and real-corpus controls
```

The exact executable surface is owned by code and tests. This roadmap records capability level, not a second symbol-by-symbol feature registry. Every transition remains an inference boundary and missing evidence remains visible.

## Active frontier: real-corpus pressure for phrase syntax

Continuation evidence now has executable adapters for persistent-part trajectories that cross an arrival, ordered motif transformations, sustained reliable harmonic-transition chains, and explicit cross-boundary continuity from phrase-boundary analysis. Cadence-derived boundary evidence is kept visible but excluded from continuation support.

New-phrase and return evidence now separates formal re-entry from recurrence. The lightweight adapter preserves a boundary-backed onset candidate, while the canonical path keeps phrase-boundary evidence and performed persistent-part re-onset as independent domains. Canonical return additionally requires a recurrence or varied-recurrence relation between two materialized phrase regions, and recurrence confidence is a hard ceiling. Recurrence without grounded re-entry, rhythm-only echoes, and pre-boundary reappearances are rejected rather than promoted into formal syntax.

Long-range harmonic-span relations now preserve every local transition while proposing non-adjacent dependencies above the surface. Prolongation requires independent cross-span continuity plus a retained structural anchor. Delayed resolution requires continuity, evidence that the earlier process remained unresolved, and a later independently grounded structural arrival. Neither relation is allowed to name tonal function or cadence class, and strong contradictory evidence remains visible.

Multi-scale phrase-role arbitration now requires strict temporal nesting and a strictly broader formal scale before roles can coexist hierarchically. Local ending can coexist with larger continuation, prolongation, or delayed resolution without becoming a same-scale contradiction. A canonical nested-local-close-inside-global-continuation candidate is only derived when both inner and outer roles are independently cross-domain grounded.

The first bounded real-corpus pressure gate now runs the existing label-blind YM2612 harmonic probe against the complete Angel Island Zone Act 1 corpus fixture. It requires measurable real execution evidence while explicitly keeping key, functional tendency, cadence class, and therefore phrase syntax blocked until persistent-part voice leading and cross-part phrase-arrival evidence exists.

YM2612 persistent-part pressure now also records candidate-support reuse without equating overlap with conflict: adjacent recurrence links may compose into explicit recurrence-link components, while persistent identity remains blocked pending shared-graph arbitration. The corpus gate spans four distinct YM2612 works (Sonic 3 & Knuckles, Aa Harimanada, Battle Golfer Yui, and Toki), requires both a positive recurrence-chain case and a zero-candidate case, and preserves per-fixture observations, trajectory candidates, link components, support relations, and cross-trajectory boundary targets without consulting platform identity or promoting phrase syntax. A second bounded gate audits the complete 22-file Aa Harimanada set to measure whether recurrence-chain concentration is local to one work or repeated across the soundtrack without turning that corpus-level behavior into musical identity.

A separate automatic SPC runtime-corpus gate now exercises the pinned SNESAPU forensic runtime on four preregistered creator-blind SPC fixtures from distinct soundtracks. It requires lossless runtime capture, replay/materialization agreement, balanced strong-versus-rejected persistent-part transitions, and at least one real three-gesture motif profile while keeping cross-source-family equivalence, phrase role, and creator identity explicitly blocked. The real runtime exposed that SPC700 callbacks and accurate DSP callbacks share a nominal clock rate but can cross a frame boundary with a small raw-tick backstep because the CPU may legally lag the DSP catch-up point. The recorder therefore preserves cross-lane backsteps explicitly while still rejecting time reversal inside either producer lane; causal order remains owned by trace indices and RAM-write serials. Materialized trace events now retain that producer clock lane explicitly. The first full automatic pass also exposed a shared execution-graph cost defect: append-only dense node/edge identities were being recovered by repeated linear scans, so real runtime graph construction could become quadratic long before musical arbitration. The canonical graph now uses those existing dense IDs for direct identity and adjacency lookup, while the corpus gate keeps its original four-fixture/three-second musical acceptance criteria, adds a per-fixture execution budget, and preserves partial progress artifacts on failure. This is the first heterogeneous source-family pressure lane for the phrase-syntax frontier rather than another YM2612-only probe.

The bounded SPC gate now also preserves an internal extractor progress sidecar at playback-block and phase boundaries. This separates capture cost from replay/materialization cost and feature-extraction cost without changing the captured facts or the musical acceptance rule; a killed fixture therefore leaves a last-known execution phase and bounded runtime counts rather than only an outer timeout.

The first phase-instrumented real fixture identified the remaining timeout upstream of replay: not one 4096-scalar-sample capture block completed within 120 seconds. Re-entry into the pinned SNESAPU source showed that forensic ordering v1 pre-synchronized the DSP before every CPU memory access, including native $F3 DSP-register accesses that already synchronize themselves. Under the accurate DSP path that can turn the second synchronization into a zero-clock run and also fragments DSP execution at irrelevant reads. Ordering v2 now distinguishes ordinary APURAM writes, echo-overlap reads, and SPC700 register accesses; the provenance contract version advances with that correction while the corpus and musical acceptance criteria remain unchanged.

The SPC persistent-part branch now has a completed cross-voice falsification chain rather than a permissive handoff heuristic. Same event-time BRR identity, temporal adjacency, and source-relative pitch continuity were first shown to be non-selective across physical voices, so unresolved cross-voice hypotheses remain capped below the shared trajectory-link threshold. The 31-cue creator-blind panel then reduced 2,078 confounded pairwise bundles to 34 bidirectionally unique cross-voice associations, 14 with strong same-voice flanks, and 14 that remained safely inside the full 0.5-second capture horizon. Exact edge geometry showed that 12 of those 14 belong to synchronized directed voice-swap cycles. The remaining two were both contested by strong ordinary same-voice predecessor/successor links. Full-graph data association therefore leaves **zero uncontested cross-voice handoffs** on this panel. No handoff evidence type or confidence increase is earned by runtime similarity or local graph uniqueness alone.

Pinned N-SPC driver source adds an independent boundary rather than a rescue. Cube and Quintet ordinary music drivers both load eight logical sequence lanes and map them to fixed S-DSP voice-register bases, so a cross-voice candidate cannot be promoted as one recovered driver track dynamically changing hardware slots. A genuine musical handoff would need independent evidence of an authored relation between distinct driver tracks, or another comparably independent musical/sequence witness. The durable experiment and claim boundary live in `research/validation/spc-cross-voice-handoff-null.md`.

The active problem remains broader real-corpus pressure: testing whether the positive phrase-role distinctions survive heterogeneous source families, styles, loop structures, and ambiguous arrivals without overfitting the synthetic witnesses that introduced them.

Target role vocabulary still has to be earned by evidence such as:

```text
ending
continuation
new-phrase onset
reroute
return
prolongation
delayed authentic resolution
nested local close inside global continuation
```

For Ionian `V → VI`, the discriminating question remains what happens after the VI arrival and what independent evidence makes it function as an ending, continuation, reroute, or nested event at different scales.

## Next implementation sequence

1. **Phrase-role evidence objects — implemented.** `model/phrase_role_evidence.h` preserves role candidate, temporal scope, formal scale, independent support provenance, explicit same-scale incompatibility, and cross-scale coexistence without establishing a cadence class.
2. **Continuation evidence — implemented.** `model/phrase_continuation_evidence.h` derives independent continuation support from persistent-part continuation, ordered motif transformation, sustained harmonic process, and cadence-independent cross-boundary continuity while rejecting one-sided/weak/broken witnesses.
3. **New-phrase and return evidence — implemented.** `model/phrase_reentry_evidence.h` preserves low-confidence boundary-backed re-entry candidates and adds a canonical cross-domain path: grounded boundary + performed persistent-part re-onset for new phrase, then a materialized earlier-to-later phrase recurrence/varied recurrence for return. Recurrence alone cannot bootstrap formal syntax or outrun its own confidence.
4. **Prolongation and delayed-resolution relations — implemented.** `model/harmonic_span_relation.h` preserves contiguous surface harmony while representing non-adjacent prolongation or delayed-resolution candidates from independent evidence domains; it projects those relations into phrase-role evidence without establishing tonal function or cadence class.
5. **Multi-scale arbitration — implemented.** `model/phrase_role_scale_arbitration.h` preserves strictly nested cross-scale roles and derives the canonical nested-local-close-inside-global-continuation candidate only from independently grounded inner and outer hypotheses.
6. **Real-corpus pressure — active.** Stress phrase syntax across heterogeneous source families and musical contexts, including ambiguous arrivals, loops, and transformed returns.

## Parallel priorities

Continue pressure on orchestration/role grammar, time-varying performance, whole-work and soundtrack-scale structure, heterogeneous corpus behavior, creator grammar/attribution, and rendering/transformation. These tracks may advance when they provide discriminating evidence, but they do not replace the active phrase-role frontier.

## Validation rule

A promotion requires more than code existence:

```text
explicit evidence object
+ provenance / uncertainty
+ discriminating executable test
+ independent corpus pressure where applicable
→ candidate shared capability
```

Keep source correctness, semantic correctness, build success, corpus behavior, and perceptual/listening quality as separate evidence states.
