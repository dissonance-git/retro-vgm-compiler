# VGM Compiler roadmap

## Mission

VGM Compiler is a provenance-aware compiler and musical reasoning system for digital game music. Its job is not merely to decode formats, render sound chips, or export MIDI. It should recover enough of the musical object that analysis, reconstruction, attribution, porting, explanation, re-realization, and search become projections of the same evidence-carrying understanding.

```text
many native musical representations
            ↓
source-specific execution / semantics
            ↓
provenance-preserving musical model
            ↓
understanding / verification passes
            ↓
many useful projections and realizations
```

See `architecture.md` for the semantic/evidence contract and `musical-understanding.md` for the north star.

## Implemented semantic surface

The repository contains working machinery for:

- exact VGM/VGZ command and Genesis device-state reconstruction;
- SPC snapshot/runtime/sample/voice evidence;
- PSF-family and xSF effective-object reconstruction with platform-specific boundaries;
- source/driver/toolchain provenance and dialect/revision distinctions;
- persistent-part hypotheses that do not equate physical channels with musical identity;
- performed pitch motion, articulation, motif shape, and source-backed role evidence;
- motif recurrence and transformation classification;
- phrase-boundary evidence, cross-part phrase consensus, and phrase regions;
- tonal-center, key-class, pitch-class-collection, and diatonic chord-degree hypotheses;
- harmonic verticalities, transitions, harmonic rhythm, voice leading, and bass/harmony interaction;
- cadential arrivals with provenance-bound degree evidence;
- Ionian authentic, PAC/IAC, half, and leading-tone-resolution morphology candidates;
- independent non-cadence-derived formal-closure evidence;
- `V → VI` deferred-authentic-resolution and deceptive-close candidates;
- phrase-scale cadence arbitration that preserves local-close/global-continuation conflicts rather than forcing one label;
- section, counterpoint, imitation, orchestration, and creator-grammar models;
- source-native enhanced rendering and Omniphony handoff contracts;
- semantic projection/round-trip experiments and a broad immutable real corpus.

Every arrow between these layers is an inference boundary. Missing evidence remains visible.

## Active frontier: positive phrase-role evidence

The compiler preserves multiple cadence-scale interpretations when local closure and larger-scale continuation are both grounded. The active problem is **positive syntax**: accumulating independent longer-range evidence that explains what the phrase does after an arrival.

Target phrase-role vocabulary must be earned by evidence such as:

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

For Ionian `V → VI`, the discriminating question is:

> What happens after the VI arrival, and what independent evidence makes it function as an ending, continuation, reroute, or nested event at different scales?

## Next implementation sequence

1. **Phrase-role evidence objects**
   - represent role hypotheses independently of cadence class;
   - retain temporal scope and formal scale;
   - retain support provenance and incompatible alternatives.

2. **Continuation evidence**
   - ground continuation of persistent parts through or after an arrival;
   - model motivic continuation and sequence;
   - model sustained harmonic process;
   - use phrase-boundary evidence that is independent of cadence labels.

3. **New-phrase and return evidence**
   - model re-onset after a boundary;
   - connect recurrence/return to previously grounded material;
   - admit orchestration/register reset or transformation only when independently supported;
   - distinguish continuation of one phrase from a new phrase after local closure.

4. **Prolongation and delayed-resolution relations**
   - connect local harmonic events across intervening material without flattening phrase syntax;
   - determine whether a later authentic arrival belongs to the same larger process or a new one.

5. **Multi-scale arbitration**
   - allow one event to be locally closing and globally continuational;
   - require explicit scale/scope rather than one global Boolean.

6. **Real-corpus pressure**
   - add matched synthetic controls first;
   - freeze source-backed real passages before final labels are admitted;
   - treat disagreement as information about missing evidence.

Final cadence-class establishment remains downstream of phrase-role evidence rather than chord morphology alone.

## Near-term musical priorities

### Orchestration and role grammar

Strengthen foreground/accompaniment, bass, inner voice, counterline, pad, ostinato, percussion, doubling, role transfer, register, density, texture, and timbral-form relations.

The target is relational:

```text
retained motif
+ changed bass
+ widened register
+ different role assignment
+ thicker texture
→ transformed return
```

not a bag of independent feature counts.

### Time-varying performance

Continue moving beyond onset-only descriptions. Preserve glides, vibrato, bends, ornaments, modulation, dynamics, articulation, release, and sustained movement as trajectories where the source supports them.

### Whole-work integration

Make local phrase, harmonic, motif, timbral, and orchestration analyses cooperate at cue scale: hierarchy among returns, contrasts, transitions, loops, transformations, and closure.

### Soundtrack-scale integration

Model thematic families, recurring formal/orchestration strategies, cue-function relations, exceptions, version/port relations, and collaborator/toolchain boundaries across a soundtrack.

### Heterogeneous corpus execution

Move the strongest shared relations through materially different source families. Freeze creator-blind structural observations before attribution labels are admitted.

## Creator grammar and attribution

Attribution is a downstream stress test of genuine understanding.

The strongest controls compare:

```text
same creator / different soundtrack
same soundtrack / different creator
same driver / different creator
same creator / different platform
same work / different representation
```

Keep composition, arrangement/programming, patch/sample design, driver/toolchain, and final realization separate. Technical resemblance may strengthen a role-scoped hypothesis without silently becoming composer proof.

Sonic 3 / Sonic & Knuckles is an adversarial integration testbed because it combines version divergence, mixed credits, ROM/SMPS evidence, VGM execution, cross-soundtrack controls, and unresolved role boundaries.

## Rendering and transformation

The reference renderer remains the scientific control. Enhanced rendering improves the same source-native instrument or realization only when the preservation contract supports the change.

Spatial presentation is orthogonal to source-native enhancement.

Backend validation should increasingly use semantic round trips:

```text
source A → model → target B
                 ↓
            re-analyze B
                 ↓
              model'
```

A successful port or re-realization need not preserve bytes or timbre unless that contract requires them. It preserves named musical obligations and exposes intentional losses.

## Future applications

Downstream goals include:

- high-quality symbolic transcription;
- real-synth realization;
- cross-format and cross-platform porting;
- semantic remastering;
- musical decompilation and source reconstruction;
- representation-independent musical search;
- composer/arranger/programmer grammar;
- musical genealogy and influence analysis;
- version archaeology and lost-version reconstruction;
- adaptive-music reconstruction;
- music debugging and linting;
- semantic deduplication;
- arrangement reduction and expansion;
- hardware counterfactuals;
- integrated end-user tooling once the semantic core is trustworthy.

## Validation pattern

A compiler this interpretive needs adversarial evidence.

```text
native source → execution → hide native semantics → recover upward → compare
```

```text
representation A → model
representation B → model
compare without assuming equivalence
```

```text
model → backend → re-analyze → compare declared obligations
```

A disagreement is an experiment. Correction outranks a convenient narrative.

## Priority order

Unless a discriminating test requires otherwise:

1. establish positive phrase-role and longer-range syntax evidence;
2. strengthen orchestration, texture, dynamics, counterpoint, and longer-range harmony;
3. integrate local analyses into whole-work models;
4. integrate whole works into soundtrack-scale models;
5. execute those relations over heterogeneous real corpora;
6. pressure-test representation invariance and creator invariance;
7. improve human musical explanation and role-aware attribution from the same evidence;
8. promote mature projections into reusable backends;
9. use synth realization, porting, and semantic round trips as verification surfaces;
10. build integrated end-user tooling after the compiler core is trustworthy.

The default research question is:

> **What uncertainty most limits musical understanding now, and what experiment would discriminate among the remaining explanations?**
