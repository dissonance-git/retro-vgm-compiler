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

## Active frontier: prolongation and delayed-resolution relations

Continuation evidence now has executable adapters for persistent-part trajectories that cross an arrival, ordered motif transformations, sustained reliable harmonic-transition chains, and explicit cross-boundary continuity from phrase-boundary analysis. Cadence-derived boundary evidence is kept visible but excluded from continuation support.

New-phrase and return evidence now separates formal re-entry from recurrence. A grounded structural boundary can support a new-phrase onset candidate without naming the material, while a return requires both independent boundary evidence and a sufficiently strong earlier-to-later motif relation. Recurrence without a grounded boundary, rhythm-only echoes, and pre-boundary reappearances are rejected rather than promoted into formal syntax.

The active problem is now representing prolongation and delayed resolution across intervening material without flattening phrase syntax or treating local surface motion as proof that a larger harmonic process ended.

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
3. **New-phrase and return evidence — implemented.** `model/phrase_reentry_evidence.h` separates grounded structural re-entry from recurrence identity: post-boundary material can support a new-phrase onset candidate, while return evidence requires both a grounded boundary and an earlier-to-later motif recurrence/transformation across that boundary.
4. **Prolongation and delayed-resolution relations — active.** Connect harmonic events across intervening material without flattening phrase syntax.
5. **Multi-scale arbitration** allowing one event to be locally closing and globally continuational with explicit scope.
6. **Real-corpus pressure** across heterogeneous source families and musical contexts.

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
