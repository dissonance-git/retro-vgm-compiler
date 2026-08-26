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

## Active frontier: positive phrase-role evidence

The compiler can preserve multiple cadence-scale interpretations when local closure and larger-scale continuation are both grounded. The active problem is **positive syntax**: accumulating independent longer-range evidence that explains what a phrase does after an arrival.

Target role vocabulary must be earned by evidence such as:

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

For Ionian `V → VI`, the discriminating question is what happens after the VI arrival and what independent evidence makes it function as an ending, continuation, reroute, or nested event at different scales.

## Next implementation sequence

1. **Phrase-role evidence objects** that preserve temporal scope, formal scale, support provenance, and incompatible alternatives independently of cadence class.
2. **Continuation evidence** from persistent-part continuation, motivic sequence, sustained harmonic process, and phrase-boundary evidence independent of cadence labels.
3. **New-phrase and return evidence** for re-onset, recurrence, transformation, and the difference between one phrase continuing and a new phrase beginning after local closure.
4. **Prolongation and delayed-resolution relations** connecting harmonic events across intervening material without flattening phrase syntax.
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
