# SPC component

`components/spc/` owns SPC/SNESAPU source-family semantics: snapshot facts, executable S-DSP continuation, runtime source/voice evidence, event-time sample identity, source transport, and source-native reconstruction.

It does not own project status, the shared musical ontology, Omniphony presentation, or dependency-patch procedure. Use [`../../docs/architecture.md`](../../docs/architecture.md), [`../../docs/source-native-enhanced-rendering.md`](../../docs/source-native-enhanced-rendering.md), [`../../docs/omniphony-realtime-spatial-path.md`](../../docs/omniphony-realtime-spatial-path.md), and [`../../patches/snesapu/README.md`](../../patches/snesapu/README.md).

## Evidence ladder

```text
SPC snapshot
→ controlled executable continuation
→ runtime S-DSP/source events
→ bounded physical voice episodes
→ source-relative performance evidence
→ persistent musical identity only when independently supported
```

A static snapshot does not contain a finished performance model.

Keep these distinctions explicit:

```text
saved register slot
!= live voice episode
!= persistent musical part

SRCN / directory reference
!= stored BRR object
!= semantic instrument identity

physical voice number
!= authored track identity

runtime similarity
!= cross-voice handoff proof
```

Event-time source identity must remain tied to the RAM generation and executable state that actually produced it. A capture or semantic gap terminates unsupported continuity rather than inventing a clean transition.

## Current cross-voice rule

The current real-corpus falsification chain found no uncontested cross-voice handoff in its admitted panel. Therefore local BRR identity, timing, pitch continuity, graph uniqueness, or physical-slot adjacency alone cannot raise a cross-voice relation into persistent-part identity.

The detailed evidence and null boundary live only in [`../../research/validation/spc-cross-voice-handoff-null.md`](../../research/validation/spc-cross-voice-handoff-null.md). This README keeps only the durable consequence.

## Implementation owners

The tree is the inventory. Stable responsibilities include:

```text
snapshot / RAM / BRR owners
  exact saved and event-time source evidence

runtime capture / trace / replay
  executable continuation and provenance

voice / performance / part adapters
  conservative lifting from physical execution into musical evidence

source transport / storage
  bounded causal handoff of captured source planes

pre-BRR / studio-source machinery
  evidence-gated higher-quality source realization

spcplayer/
  reference/host integration boundary
```

Versioned transport/wire names are compatibility contracts when consumed outside one translation unit. Do not rename them merely for cosmetic cleanup.

## Reference and enhancement boundary

The protected historical/reference path remains authoritative unless a stronger source representation is explicitly admitted.

The reconstruction ladder is conceptually:

```text
verified upstream/original source at the live game phase
→ verified prepared pre-BRR source
→ exact BRR trajectory reconstructed at playback rate
→ normal SNESAPU reconstruction
→ protected reference
```

Each rung is evidence-labelled and reversible. Upstream similarity is not admission. Generative restoration is not source truth.

Precise dependency patch order, transport versions, and sidecar authoring requirements live in [`../../patches/snesapu/README.md`](../../patches/snesapu/README.md), not here.

## Spatial handoff

VGM Compiler preserves dry source evidence, authored route/send state, shared historical wet fields, persistent-part evidence where earned, and causal presentation inputs. Omniphony owns final scene realization.

A shared S-DSP echo return stays one historical feedback field. It must not be cloned into fictional per-voice wet stems.

See [`../../docs/omniphony-realtime-spatial-path.md`](../../docs/omniphony-realtime-spatial-path.md).

## Validation

Keep these states separate:

```text
snapshot/source correctness
runtime continuation correctness
source-transport/ABI correctness
shared-model inference
real-corpus falsification
private foobar delivery
physical listening
```

Current unresolved SPC work belongs only in [`../../docs/vgm-compiler-roadmap.md`](../../docs/vgm-compiler-roadmap.md). Git history owns completed experiment chronology.
