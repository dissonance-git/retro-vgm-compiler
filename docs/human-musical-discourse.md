# Human musical discourse

This document owns the durable contract for projecting VGM Compiler evidence into natural musical language. Research pressure and discourse observatories belong under `research/music/`.

The wording may change with audience and purpose. The evidence underneath may not.

```text
source / execution / synthesis evidence
→ musical / acoustic / perceptual relations
→ support bundle + alternatives + confidence
→ discourse projection
```

```text
natural language != weaker evidence discipline
metaphor != unsupported claim
evaluation != observation
musical effect != creator intent
```

## Modes and acts

Modes are viewpoints, not fixed user identities:

```text
listener
critic
composer / musician
theorist / musicologist
producer
mixing / mastering engineer
forensic / technical
```

Discourse acts are a separate axis:

```text
DESCRIBE
COMPARE
INTERPRET
EVALUATE
DIAGNOSE
DIRECT
EXPLAIN
REPORT INTENT
```

A conversation may switch mode or act sentence by sentence without changing the evidence model.

## Register law

### Listener / critic

Lead with audible relationships: salience, motion, contrast, energy, recurrence, density, texture, groove, space, and what changed from the previous moment.

Evaluation remains contextual. Accurate identification of density, brightness, tension, or contrast does not decide whether the result is good.

### Composer / musician / producer

Emphasize shape, pacing, part interaction, hook, gesture, tension/release, foreground/background, density, register, contrast, and intervention goals.

Do not turn an inferred musical effect into documented creator intention.

### Theorist / musicologist

Expose analytical assumptions and meaningful alternatives. Tonal center, chord spelling, function, voice leading, cadence, phrase role, motif, form, and style are analyses over lower evidence rather than hidden source bytes.

Western functional harmony is an analytical framework, not a universal definition of musical structure.

### Mixing / mastering

Perceptual descriptors such as `wide`, `bright`, `punchy`, `forward`, `muddy`, `open`, or `smeared` are relational concepts. They may depend on several musical and acoustic causes at once and must not be implemented as one-feature aliases.

### Forensic / technical

Expose exact source commands, driver state, register values, patch/sample identity, envelopes/modulation, allocation, clocks/time mappings, synthesis/routing, and measurement provenance when the question needs them.

This is the deepest explanatory register, not the mandatory first response.

## Many-to-many language

One technical change can support several natural descriptions; one natural descriptor can arise from several different technical relationships.

Do not implement phrase dictionaries such as:

```text
if stereo_width > threshold:
    say "opens up"
```

Conceptually the renderer consumes:

```text
claim / comparison
+ support bundle
+ confidence / alternatives
+ discourse mode
+ discourse act
+ requested detail
→ wording
```

Metaphors such as motion, space, force, material, body, social relation, or narrative are useful only when a support bundle grounds them.

## Progressive disclosure

Default to the smallest useful explanation while keeping deeper evidence reachable.

```text
what happens?
→ natural musical description

what changed?
→ musical / acoustic relations

why?
→ source / driver / synthesis evidence + provenance
```

Theory follows the same pattern:

```text
analytical reading
→ supporting pitches / bass / meter / duration / voice leading / form
→ executable source and synthesis evidence
```

## Evidence rules

1. Natural wording may not strengthen the evidence state of the underlying claim.
2. Metaphor summarizes support; it does not replace support.
3. Evaluation requires a criterion/context.
4. Creator intent requires documentary evidence.
5. Listener-response claims retain listener/model/cultural context when material.
6. Culture-, genre-, era-, and community-specific vocabulary is not universalized.
7. Competing valid descriptions/analyses may coexist.
8. Theory labels retain their framework/scope and meaningful alternatives.
9. Exact technical evidence remains reachable beneath grounded description.

## Default voice

Unless requested otherwise, support a knowledgeable listening-companion register: musically literate, comfortable with ordinary metaphor, low on unnecessary jargon, willing to preserve uncertainty, and able to descend immediately into technical or theoretical detail.

## Validation

A human-facing analysis should survive three independent checks:

```text
LANGUAGE
natural for the requested mode/act?

GROUNDING
can every material claim descend into support?

CALIBRATION
does wording preserve uncertainty, alternatives, evaluation context,
theory assumptions, and documentary boundaries?
```

> **Speak naturally about music while preserving the exact reasons each claim is supportable.**
