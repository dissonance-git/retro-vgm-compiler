# Human musical discourse observatory

This research owner records evidence and pressure tests for how people describe music. The durable VGM Compiler discourse contract lives in [`../../docs/human-musical-discourse.md`](../../docs/human-musical-discourse.md). This file does not define a second rendering policy.

## Research question

Given grounded evidence about a musical passage, which properties of human musical discourse must a renderer preserve so its language is natural without becoming less truthful?

The working boundary is:

```text
source / execution / synthesis evidence
        ↓
performance / structure / acoustic / perceptual analysis
        ↓
support bundle + comparison context
        ↓
discourse projection
```

The projection changes wording and emphasis. It does not change the evidence status underneath.

## Evidence-backed observations

### Metaphor is structural, not decorative

Musical discourse repeatedly organizes sound through experiential domains such as motion, space, force, material, light, body, social interaction, and narrative. Expertise changes vocabulary and precision but does not remove metaphor.

Useful pressure examples include:

```text
MOTION       push, pull, climb, fall, circle, arrive
SPACE        open, close, widen, crowded, distant
FORCE        heavy, light, punch, bite, land
MATERIAL     thick, thin, smooth, rough, brittle
LIGHT        bright, dark, glowing, murky
BODY         breathe, choke, tense, relax
SOCIAL       answer, interrupt, support, shadow
NARRATIVE    build, drop, return, stall, resolve
```

These terms are many-to-many summaries. `Open`, `punchy`, `dark`, or `breathing` can each be supported by several different musical or acoustic relationships. A fixed phrase-to-feature dictionary would therefore create false precision.

### Register follows purpose

Listeners, critics, composers, performers, producers, engineers, theorists, musicologists, and forensic investigators often foreground different evidence from the same passage. These are discourse modes rather than immutable user identities.

A single supported change such as greater part count, upper-register activity, wider spatial distribution, and sharper bass articulation might naturally be projected as:

```text
listener:   it gets bigger here
producer:   the return has more lift
engineer:   the return opens up and the low end has more bite
theorist:   the return strengthens through register, texture, and phrase context
forensic:   exact voice / patch / timing / routing changes
```

The evidence does not become different because the register does.

### Description and evaluation are different claims

Critical and production discourse often combines observation with judgment. A dense arrangement may be accurately identified before anyone decides whether it is rich, exciting, cluttered, or excessive.

```text
observed musical behavior
!= evaluative judgment
```

Evaluation therefore needs its own criterion or context rather than being smuggled into a supposedly factual label.

### Musical effect and creator intent are different claims

Creator-facing language commonly talks in terms of goals, shape, contrast, tension, gesture, space, momentum, and arrival. Those concepts are useful for describing effects or intervention goals, but historical intent requires documentary evidence.

```text
musical effect
!= documented creator intention
```

Interviews, notes, correspondence, source comments, or comparable provenance can support intent. Pattern resemblance alone cannot.

### Perceptual descriptors are relational

Engineering and production vocabulary often names a relationship rather than a scalar measurement:

```text
forward / back
wide / narrow
bright / dark
thin / full
punch / rounded
blend / separate
breathe / clamp
clear / smeared
```

A descriptor such as `bright` may depend on spectral balance, register, harmonic structure, masking, dynamics, arrangement, playback context, and learned convention. A universal threshold such as `spectral_centroid > X -> bright` would erase that context.

### Progressive disclosure matches natural explanation

Human conversation commonly begins with a meaningful summary and descends only when the listener asks why.

```text
what happens?
→ natural musical description

what changed?
→ concrete musical / acoustic relations

why did that happen?
→ source / execution / synthesis evidence
```

This gives VGM Compiler a useful presentation pressure: deep evidence should remain reachable without forcing the deepest vocabulary into every first answer.

## Implementation consequences under test

The evidence supports testing a renderer with an input shape such as:

```text
claim / comparison
+ support bundle
+ confidence / alternatives
+ discourse mode
+ discourse act
+ requested detail
→ wording
```

Important consequences:

1. no independent `human_language` truth layer is required;
2. metaphor should point back to a support bundle;
3. mode and discourse act are separate axes;
4. competing analyses may produce competing valid descriptions;
5. listener/model/cultural context stays explicit where it changes interpretation;
6. natural language should not strengthen an uncertain lower-level claim;
7. the renderer should be able to descend from summary to exact evidence on demand.

## Discriminating tests

A useful discourse system should survive three independent pressures:

```text
NATURALNESS
Would a knowledgeable person plausibly say this in the requested register?

GROUNDING
Can every material statement descend into a support bundle?

CALIBRATION
Does the wording preserve uncertainty, alternatives, evaluation context,
and documentary boundaries?
```

Adversarial cases are especially valuable:

- two different technical causes that invite the same ordinary descriptor;
- one technical change that supports several natural descriptions;
- passages where a vivid metaphor is tempting but weakly supported;
- analytical alternatives that sound equally natural but carry different assumptions;
- creator-like effects with no documentary evidence of creator intent;
- culture- or genre-specific vocabulary that would be unsafe to universalize.

## Evidence anchors

The research pressure comes from several mature bodies of work rather than one preferred vocabulary:

- conceptual-metaphor studies of musical motion, force, space, body, material, and narrative;
- timbre-semantics and psychoacoustic studies showing context-sensitive perceptual descriptors;
- music criticism and musicological discourse analysis;
- production, mixing, and mastering discourse where perceptual goals are mapped to many possible interventions;
- embodied and ecological accounts of musical perception;
- conversation and explanation patterns that move between summary, comparison, interpretation, and mechanism.

These sources motivate representation and evaluation choices. VGM Compiler's own grounded examples and human evaluation determine which rendering behavior becomes a project contract.

## Promotion boundary

Research belongs here when it changes an experiment, falsifier, or evidence requirement. Once a discourse rule is stable enough to govern the project, it belongs only in [`../../docs/human-musical-discourse.md`](../../docs/human-musical-discourse.md). Later evidence should pressure-test that owner rather than grow a peer manual beside it.
