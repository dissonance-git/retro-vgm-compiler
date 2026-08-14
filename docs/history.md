# Technical lineage

Game Music Interpreter carries forward several implementation ideas that were pressure-tested in earlier game-audio experiments and later refined under stricter evidence rules.

This document records the useful technical lineage, not a product or naming chronology.

## Durable ideas retained

Earlier work established practical value in keeping source/device state visible during playback rather than treating final stereo PCM as the only useful object.

Mechanisms that survived into the current architecture include:

- VGM register shadowing and timed device state;
- YM2612 and SN76489 live-state inspection;
- SPC/S-DSP voice telemetry;
- OPN/OPM/OPL-family device distinctions;
- persistent source identifiers where the source actually supports them;
- realtime adapter boundaries;
- explicit confidence and provenance;
- reference-versus-enhanced rendering controls.

The important architectural lesson was not any one old implementation. It was that useful musical reasoning requires preserving the route from encoded source through execution and synthesis rather than trying to reconstruct meaning only from the final mix.

## Ideas deliberately not inherited as law

Some early experiments used semantic-role heuristics, hardware-channel identity, or spatial behavior more aggressively than the evidence justified. Those experiments remain useful negative controls.

The current model therefore rejects several shortcuts:

```text
hardware channel != persistent musical part

register activity != authored score

source position != authored 3D scene

implementation fingerprint != composer identity

plausible reconstruction != recovered historical source
```

A mechanism survives only when it can be justified by source evidence, independent implementations, real corpus controls, listening tests, or a combination appropriate to the claim.

## Current continuity

The present implementation extends the same technical questions upward:

```text
exact source object
        ↓
program / driver execution
        ↓
device and synthesis state
        ↓
performed musical trajectories
        ↓
persistent parts and auditory organization
        ↓
harmony / form / style / attribution hypotheses
        ↓
human musical explanation
```

The project can replace an old mechanism without erasing the evidence that motivated it. Corrections are part of the lineage.

## Historical source preservation

When an older repository, source release, emulator revision, driver dump, manual, or corpus object matters to a current claim, preserve the exact upstream commit/hash and its evidential role in `docs/upstreams.md`, `imports/MANIFEST.md`, the corpus manifest, or the relevant research case.

Do not keep obsolete implementation behavior alive merely to preserve continuity. Preserve provenance; keep only mechanisms that still survive testing.
