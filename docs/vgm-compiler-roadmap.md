# VGM Compiler roadmap

This is the canonical current-state owner for VGM Compiler. It records what is implemented, what is active, and what comes next.

Durable semantic law belongs in [`architecture.md`](architecture.md). The musical target belongs in [`musical-understanding.md`](musical-understanding.md). Detailed experiment evidence stays in `research/`; executable truth stays in code and tests.

> **Current state here. Durable law elsewhere. Git keeps the walk.**

## Implemented surface

VGM Compiler now has working machinery across this chain:

```text
source/container reconstruction
→ device/runtime evidence
→ provenance and source/driver ancestry
→ persistent musical identity and performed pitch/timing evidence
→ motif / phrase / harmony / cadence / formal evidence
→ counterpoint / orchestration / creator-grammar evidence
→ source-native enhanced rendering
→ realtime Omniphony handoff
→ semantic projection and real-corpus controls
```

The exact symbol/test inventory is deliberately not repeated here.

### Phrase syntax foundation

The shared model now has executable owners for:

- phrase-role evidence with explicit scale, support provenance, alternatives, and conflict;
- continuation from independent persistent-part, motif, harmonic-process, and boundary evidence;
- new-phrase/re-entry and return evidence without letting recurrence bootstrap its own boundary;
- long-range prolongation and delayed-resolution candidates that preserve local harmonic transitions;
- multi-scale arbitration that permits nested local closure inside larger continuation, prolongation, or delayed resolution.

These mechanisms are implemented. Their current question is no longer “can the model represent the distinction?” but “does the distinction survive heterogeneous real music without overfitting?”

### Source-family pressure

The VGM/YM2612 lane now exercises real persistent-part and phrase-pressure controls across multiple works rather than one synthetic or Sonic-only witness.

The SPC lane now provides an independent source-family pressure surface through accurate SNESAPU runtime capture, replay/materialization agreement, bounded physical-voice episodes, event-time BRR/RAM identity, persistent-part evidence, and label-blind real-corpus testing.

The SPC runtime path also now preserves the producer clock lane explicitly where CPU and DSP callback clocks can cross a frame boundary without treating legal cross-lane backsteps as same-lane time reversal. Dense execution-graph IDs are used directly for identity and adjacency so corpus construction does not degrade into repeated linear scans.

These are implementation facts, not proof that SPC and VGM share identical source semantics.

### Cross-voice SPC boundary

A 31-cue creator-blind SPC panel falsified the current runtime-only route for promoting persistent musical identity across different S-DSP physical voices.

The strongest current reduction is:

```text
2,078 local source/timing/pitch bundles
→ 34 bidirectionally unique candidates
→ 14 two-sided boundary-safe candidates
→ 12 synchronized voice-swap-cycle candidates
→ 2 remaining candidates with strong same-voice competition
→ 0 uncontested cross-voice handoffs
```

Pinned Cube and Quintet N-SPC driver evidence also shows ordinary fixed logical-track-to-voice routing, so driver-track identity cannot be used as a hidden rescue for those rejected links.

Therefore the current rule remains:

```text
same BRR identity
+ close timing
+ source-relative pitch continuity
+ local graph uniqueness
!= enough evidence for cross-voice persistent identity
```

The detailed falsifier, measurements, and re-entry boundary live only in [`../research/validation/spc-cross-voice-handoff-null.md`](../research/validation/spc-cross-voice-handoff-null.md).

## Active frontier: real-corpus pressure for phrase syntax

The active frontier is broader real-corpus testing of phrase-role distinctions across materially different source families, musical styles, loop structures, phrase lengths, ambiguous arrivals, transformed returns, and nested formal scales.

The target vocabulary remains:

```text
ending
continuation
new-phrase onset
reroute
return
prolongation
delayed resolution
nested local close inside larger continuation
```

The central test is not whether these labels can be assigned. It is whether each role can be supported by independent evidence and remain useful under adversarial alternatives.

Important discriminators include:

- persistent-part continuity across the proposed boundary;
- motif transformation or recurrence that is not circularly derived from the phrase label;
- reliable harmonic process across or beyond an apparent arrival;
- independently grounded arrival/boundary evidence;
- strict formal-scale nesting rather than same-scale contradiction;
- real negative cases where an attractive role must remain unresolved.

For an arrival such as Ionian `V → VI`, the system must determine what happens after the VI arrival and what independent evidence makes the event function as an ending, continuation, reroute, delayed resolution, or nested local event. Local harmonic morphology alone is insufficient.

## Current source-family gates

### YM2612 / VGM

Continue widening persistent-part and phrase-pressure controls across independent works and soundtracks.

The gate should preserve positive and null cases, expose candidate-support reuse and recurrence-link structure, and prevent platform identity or cue labels from becoming musical evidence.

### SPC / SNESAPU

Continue the automatic creator-blind runtime-corpus gate across independent soundtracks.

The gate must preserve:

- lossless admitted runtime capture;
- replay/materialization agreement;
- bounded progress and runtime cost;
- explicit capture/source gaps;
- balanced strong and rejected persistent-part transitions;
- real motif/trajectory evidence;
- the current cross-voice handoff null unless a genuinely independent evidence domain defeats it.

The next positive cross-voice route must cross an evidence-domain boundary. Plausible witnesses include an authored relation between distinct sequence tracks, an independently grounded motif/phrase relation predicting the transfer, or documentary/source evidence encoding the role transfer. Another correlated runtime-similarity feature is not enough.

## Parallel priorities

These may advance when they provide discriminating evidence without replacing the active phrase-syntax frontier:

- orchestration and part-role grammar;
- time-varying performance and persistent identity;
- whole-work and soundtrack-scale structure;
- creator grammar and role-aware attribution;
- source-native enhancement and reconstruction;
- realtime Omniphony source handoff;
- additional heterogeneous source families and drivers.

## Promotion rule

A capability becomes stronger project law only through:

```text
explicit evidence object
+ provenance / uncertainty
+ discriminating executable falsifier
+ independent corpus pressure where applicable
→ candidate shared capability
```

Keep these states distinct:

```text
source/container correctness
!= semantic correctness
!= build success
!= unit/integration success
!= corpus survival
!= package/runtime delivery
!= physical listening quality
```

Do not preserve completed experiment chronology here. Once a result has a durable consequence, keep the consequence in its current owner and let Git/research evidence retain the path that produced it.
