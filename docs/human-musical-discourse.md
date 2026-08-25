# Human musical discourse

VGM Compiler should understand music deeply enough to discuss it naturally, analytically, and technically without changing the evidence underneath the wording.

Human musical language is a **discourse projection** over evidence-bearing musical, acoustic, perceptual, historical-source, and technical claims.

```text
source / execution / synthesis evidence
        ↓
performance + structure + acoustic/perceptual analyses
        ↓
comparison across time / parts / sections
        ↓
discourse projection
        ├─ listener
        ├─ critic
        ├─ composer / musician
        ├─ theorist / musicologist
        ├─ producer
        ├─ mixing / mastering engineer
        └─ forensic / technical
```

The evidence model does not change when the register changes.

```text
natural musical description
!= simplified technical telemetry

metaphorical language
!= unsupported language
```

A phrase such as `it opens up here` is useful when the system can identify the musical or acoustic changes that support it.

## Discourse modes

Modes are viewpoints, not fixed user identities. A conversation can switch modes sentence by sentence.

### Listener

Focus on audible change, salience, energy, motion, contrast, memorable moments, and relationships to what came before.

Useful language can include:

```text
it opens up here
the bass starts pushing harder
everything drops out for a moment
that high part sneaks in
the section feels suspended
this return feels darker than the first one
```

Theory terminology is optional unless it clarifies the answer.

### Critic

Critical discourse combines description, comparison, interpretation, cultural/style context, production observation, and evaluation. Evaluation remains separate from source truth.

```text
observed behavior
!= judgment about that behavior
```

A dense arrangement may be described accurately before anyone decides whether it feels rich, crowded, exciting, or excessive.

### Composer / musician

Creator-facing discourse emphasizes shape, pacing, contrast, tension/release, gesture, thematic identity, part interaction, playability, space, and expressive intent.

Intent is a documentary claim:

```text
musical effect
!= creator intent
```

Claims about intention require interviews, notes, source comments, correspondence, or comparable evidence.

### Theorist / musicologist

Theory-facing discourse can address:

- tonal center, key, mode, and pitch collection;
- chord spelling, root, quality, inversion, and alteration;
- chord tones versus figuration;
- harmonic function and tonal hierarchy;
- tonicization and modulation;
- harmonic rhythm and progression;
- prolongation, preparation, substitution, and resolution;
- cadence and phrase function;
- voice leading and contrapuntal relations;
- motif, schema, transformation, repetition, and form;
- style/corpus scope and competing analytical readings.

These labels are analyses over lower evidence, not bytes hidden inside the source.

```text
simultaneous pitches
!= chord spelling
!= harmonic function
!= listener expectation
```

Competing analyses remain available when the evidence does not uniquely separate them. Western functional harmony is one analytical framework, not a universal definition of musical structure.

### Producer

Producer discourse is intervention-oriented: lift, energy, momentum, hook, density, impact, contrast, making room, thinning, filling, foregrounding, and letting a section breathe.

The system should connect those goals to concrete musical and production relationships without pretending there is only one technical route to the result.

### Mixing / mastering engineer

Engineering discourse connects perception to controllable signal relationships. Useful vocabulary families include:

```text
POSITION
forward • back • close • distant • centered

WIDTH / SPACE
wide • narrow • open • boxed-in • dry • wet • deep • shallow

SPECTRAL CHARACTER
bright • dark • warm • harsh • smooth • airy • dull • muddy • brittle

MASS / BODY
thin • thick • full • hollow • weight • body

TRANSIENT / FORCE
punch • snap • bite • impact • rounded

COHESION
blend • separate • poke out • mask • fight

DYNAMICS / MOTION
breathe • pump • choke • clamp • open up • flatten • jump out

CLARITY / DETAIL
clean • clear • defined • smeared • blurred • crowded
```

These descriptors are many-to-many perceptual concepts, not aliases for single DSP parameters.

### Forensic / technical

Technical discourse exposes exact machinery when the question calls for it:

- source commands and bytes;
- driver state and control flow;
- register values;
- patch/sample identity;
- envelopes and modulation;
- voice allocation;
- timing and clock mappings;
- synthesis and routing state;
- acoustic measurements;
- analytical features with provenance.

This is the deepest explanatory register, not the mandatory surface form of every answer.

## Discourse acts

Mode and act are independent axes.

```text
DESCRIBE     what is happening
COMPARE      how two moments or versions differ
INTERPRET    how a pattern functions
EVALUATE     whether it succeeds under some criterion
DIAGNOSE     what causes a musical or audible problem
DIRECT       what should change
EXPLAIN      what mechanism produces an effect
REPORT INTENT what documented creators say they intended
```

The same vocabulary can carry different evidence obligations depending on the act.

## Metaphor families

Human musical language repeatedly organizes sound through experiential domains:

```text
MOTION       push, pull, rush, drag, glide, climb, fall, settle, circle, arrive
SPACE        open, close, widen, narrow, forward, back, crowded, surrounding
FORCE        heavy, light, punchy, soft, press, snap, bite, land
MATERIAL     thick, thin, smooth, rough, glassy, fuzzy, brittle, smeared
LIGHT        bright, dark, glowing, murky, vivid, pale
BODY         breathe, choke, pulse, tense, relax, loose, tight
SOCIAL       answer, interrupt, support, shadow, follow, step back
NARRATIVE    build, drop, return, turn, stall, resolve, reset, come home
```

A metaphor is grounded by a support bundle, not a universal feature threshold.

## Many-to-many rule

```text
one technical change
→ several reasonable human descriptions

one human description
← several possible technical causes
```

For example, `the chorus opens up` might be supported by some combination of more upper-register activity, greater part count, lower masking, wider routing, longer sustain, brighter timbre, stronger ambience, or a redistribution of spectral density.

Do not implement a phrase dictionary such as:

```text
if stereo_width > threshold:
    say "opens up"
```

Instead, discourse rendering conceptually receives:

```text
claim / comparison
+ supporting observations
+ confidence
+ discourse mode
+ discourse act
+ requested detail
→ natural description
```

The wording is a projection. The support bundle carries the evidence.

## Progressive disclosure

The default conversational pattern is descent on demand.

```text
What happens here?
→ It opens up and starts pushing harder.

What changed?
→ A higher part enters, the bass gets more pointed, and the texture spreads.

What exactly causes that?
→ source / driver / envelope / routing explanation with provenance
```

Theory questions use a parallel route:

```text
What is it doing harmonically?
→ analytical reading + meaningful alternatives

What supports that?
→ pitches + bass motion + meter + duration + voice leading + formal context

What produced those notes?
→ executable source and synthesis evidence
```

## Default register

Unless the user requests another register, VGM Compiler should support a knowledgeable listening-companion voice:

- musically informed without unnecessary jargon;
- comfortable with ordinary metaphors;
- able to discuss melody, bass, rhythm, harmony, tonal center, texture, timbre, groove, production, and form naturally;
- able to move into harmonic, contrapuntal, motivic, and formal analysis when relevant;
- willing to preserve alternatives when interpretation is uncertain;
- able to descend immediately into exact evidence.

## Evidence rules

1. Natural wording must not strengthen the evidence status of the underlying claim.
2. Metaphor may summarize a support bundle but remains traceable to that bundle.
3. Evaluation is not objective source truth.
4. Creator intent requires documentary evidence.
5. Listener-response claims preserve listener/model context when it matters.
6. Culture-, genre-, era-, and community-specific vocabulary is not universalized.
7. Technical evidence remains available beneath grounded natural description.
8. Competing descriptions and analyses may coexist when the evidence supports them.
9. Theory labels retain analytical framework, style/corpus scope, lower evidence, and meaningful alternatives.

## Validation

A human-facing analysis should pass three directions of pressure testing:

```text
LANGUAGE
Does it sound like natural knowledgeable discourse about the music?

EVIDENCE
Can every material claim descend into supporting musical, acoustic, documentary, or executable evidence?

ANALYSIS
When theory is used, are assumptions and competing valid readings represented honestly?
```

The target is simple:

> **Speak naturally about music while preserving the exact reasons each claim is supportable.**
