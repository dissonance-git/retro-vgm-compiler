# Realtime musical spatial DSP

## Scope

This document owns VGM Compiler's **internal causal realtime spatial-state contract**. It does not own Omniphony presentation policy, renderer topology, ABI details, or listening aesthetics. Those begin at [`omniphony-realtime-spatial-path.md`](omniphony-realtime-spatial-path.md).

Normal playback is streaming and causal. Whole-track reverse compilation, cached future positions, and current-block PCM choosing its own current presentation are outside this contract.

```text
source-specific execution / isolated source evidence
→ neutral spatial source block
→ prepare current block from past-only state
→ renderer handoff
→ complete current block
→ acoustic observation
→ weak musical-role hypotheses
→ identity-aware memory for later blocks
```

## State and evidence

`model/spatial_source.h` is the format-neutral source boundary. Physical lanes are transport, not automatically persistent parts, musical roles, auditory streams, or authored geometry.

The realtime path may retain bounded causal state including source identity/generation, evidence-backed persistent-part identity, recent role hypotheses, cue provenance, rolling acoustic statistics, role smoothing/expiry, and spatial-control smoothing.

`model/realtime_musical_spatial_observer.h` measures bounded source/scene facts. Measurements such as energy share, low-band ratio, edge density, or shared-wet share remain observations, not musical verdicts. Unknown samples are excluded rather than fabricated as silence.

`model/realtime_musical_role_hypothesis.h` may nominate weak roles such as `foundation`, `foreground`, `transient_accent`, and `environmental_layer`. Acoustic evidence is confidence-capped and cannot silently outrank stronger source-supported evidence.

## Identity law

Continuity prefers an evidence-backed persistent-part ID; otherwise it is bounded by `source_id + generation`. Hardware lane number is never the fallback musical identity.

Lane reuse must not donate observer, role, or smoothing state to an unrelated source. Persistent musical continuity may cross physical episodes only when explicit evidence earns it.

## Causality law

`model/realtime_musical_spatial_frontend.h` has two phases:

- `prepare_block()` runs before current audio is rendered and may use current boundary evidence plus memory learned from completed earlier audio. It does not inspect current-block PCM.
- `complete_block()` runs only after the block has rendered and may analyze that completed PCM and update memory for later blocks.

Therefore:

```text
same completed history before block N
→ same prepared musical state for block N
```

Within a block, the same evidence prefix through frame N must produce the same control prefix through frame N. Later events may change targets only from their event boundary forward.

Musical time advances once per audio block, not once per source lane.

## Control law

`model/realtime_spatial_scene_dsp.h` emits bounded frame spans with source evidence, smoothed start values, targets, and recurrence coefficients. Presentation tendencies such as foundation, foreground, diffuse, width, vertical affinity, and confidence are derived controls, not authored coordinates.

No per-sample future coordinate cache is required.

## Failure law

Malformed evidence fails closed and must not invent repaired history. If a valid audio block has already happened but post-render analysis is unusable, stream time still advances and the block simply contributes no new learning. Invalid audio-clock input cannot advance time.

## Validation obligations

Executable tests should defend allocation-free bounded operation, prepare/complete causality, exact event boundaries, once-per-block time, identity isolation, evidence-backed continuity, deterministic capacity behavior, malformed-timeline handling, and no promotion of inferred tendencies into authored geometry.

Current project priority belongs in [`vgm-compiler-roadmap.md`](vgm-compiler-roadmap.md), not here.
