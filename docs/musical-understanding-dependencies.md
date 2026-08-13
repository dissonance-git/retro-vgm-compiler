# Musical understanding dependencies

## Purpose

VGM Tooling does not only need a list of things it may analyze. It needs an explicit discipline for how one kind of musical understanding may depend on another without flattening uncertainty or replacing lower evidence.

This document records that discipline.

The goal is not to force every source through one serial pipeline. An authored score, validated driver, VGM log, SPC snapshot, rendered waveform, libaural observation, archival source, or external annotation may expose different layers directly. The goal is to make **inferred** claims reveal the evidence and assumptions that allow them to exist.

The central rule is:

> **Higher analysis may summarize lower evidence, but it may not erase, silently repair, or skip the uncertainty of the evidence it depends on.**

This is the reasoning counterpart of the repository's existing provenance law.

---

## 1. The useful shape is a dependency graph, not a ladder

A simple ladder is helpful for orientation:

```text
SOURCE / EXECUTION / SYNTHESIS
        ↓
MUSICAL PERFORMANCE
        ↓
AUDITORY ORGANIZATION
        ↓
PITCH / RHYTHM / PART RELATIONS
        ↓
HARMONY / TONALITY
        ↓
PROGRESSION / VOICE LEADING / HARMONIC RHYTHM
        ↓
CADENCE / PHRASE / MOTIVIC RELATION
        ↓
FORM / HIERARCHY
        ↓
STYLE / WORK / VERSION / AUTHORSHIP ANALYSIS
        ↓
HUMAN DISCOURSE
```

But the real object is a graph.

Examples:

- an MML source can expose notes before acoustic rendering exists;
- a validated driver can expose a logical part more strongly than an audio grouping model can;
- libaural may supply auditory-stream, continuity, masking, timbre, pitch, or foreground/background evidence that a register trace cannot supply by itself;
- an archival source can constrain authorship without depending on an inferred chord progression;
- exact source metadata can identify a work/version while a listener remains uncertain about its form;
- a harmonic analysis may use both exact performed pitches and perceptual grouping evidence when note ownership is ambiguous.

Therefore:

```text
analysis order
!= truth order
!= time order
```

What matters is that every inference keeps its dependency route explicit.

---

## 2. Cross-project contract

The current relationship with libaural and Helix clarifies the boundary.

### libaural

libaural studies the transformation:

```text
digital audio
→ auditory evidence
→ grouping / continuity / masking / pitch / timbre / space
→ persistent heard objects / fields / relations
→ auditory memory / uncertainty
```

Its newest research direction treats hearing as a continuously maintained sensory state whose clock is separate from slower reasoning.

For VGM Tooling, the important transfer is not to copy libaural's implementation. It is to accept auditory evidence as a first-class observation surface when a musical question depends on what is heard rather than only on what the executable source did.

### VGM Tooling

VGM Tooling owns the executable-music route:

```text
source
→ program / driver
→ device / synthesis
→ realized performance
→ musical analysis
→ source-native rendering
```

It can often provide answer keys that ordinary audio research lacks: exact timing, patch/sample identity, physical voice allocation, control trajectories, and sometimes authored or driver-level part identity.

### Helix / Sonic 3 Music Attribution

The Sonic 3 attribution project supplies a live adversarial case for the upper layers.

Its current contract already requires:

```text
composer
!= arranger / implementation author

technical fingerprint
!= composer proof

team credit
!= track-level attribution

one version
!= every version of the work
```

That is the same dependency discipline applied to authorship.

The cross-project loop is therefore bidirectional:

```text
libaural
heard-object / grouping / memory evidence
        ↓
VGM Tooling
musical + executable interpretation
        ↓
Helix attribution cases
historical / candidate / control pressure
        ↓
VGM Tooling
better distinctions and tests
        ↓
libaural
source-authoritative perceptual controls
```

No project becomes another project's ontology merely because evidence crosses the boundary.

---

## 3. Performance comes before theory when theory is inferred

For an executable source, preserve the strongest available performance evidence before assigning music-theory labels.

Useful coordinates include:

```text
onset / offset
pitch or pitch trajectory
duration / gate behavior
articulation
loudness / dynamic trajectory
persistent part hypothesis
instrument / synthesis identity
metrical position when established
```

A hardware channel is not automatically a voice, and a voice is not automatically a persistent part.

If part identity is uncertain, a later chord or voice-leading analysis must be able to say that its note ownership is uncertain too.

---

## 4. Harmonic understanding has internal dependencies

Do not collapse `sounding pitches` directly into `chord`.

A stronger route is:

```text
performed pitch activity
        ↓
harmonic segmentation
        ↓
chord tone / figuration hypotheses
        ↓
chord root / quality / inversion candidates
        ↓
local tonal center / key candidates
        ↓
harmonic function
        ↓
progression
        ↓
harmonic rhythm
        ↓
preparation / prolongation / resolution relations
```

These coordinates answer different questions.

### Sounding pitch is not chord membership

Executable game music frequently contains:

- passing and neighbor tones;
- arpeggiation;
- suspensions and anticipations;
- melodic pedals;
- delayed or staggered attacks;
- held notes across harmonic changes;
- pitch envelopes and portamento;
- rhythmic echo;
- rapid ornamentation;
- channel stealing and retrigger behavior.

Therefore:

```text
pitch active at time t
!= chord tone at time t
```

### Chord is not function

The same vertical sonority can have different functions under different key, phrase, bass, voice-leading, stylistic, or theoretical contexts.

```text
pitch-class content
!= chord spelling
!= root
!= inversion
!= function
```

### Local key is not global key

A piece may tonicize, modulate, imply competing centers, use modes, use chromatic plans, or resist a useful major/minor reading.

Represent local and global tonal claims separately.

### Progression is not a flat chord list

A progression has temporal and relational structure.

Useful questions include:

- which sonorities are structural versus ornamental?
- how long does each harmony remain active?
- what is prepared, prolonged, tonicized, or resolved?
- how do bass motion and upper voices participate?
- does the same chord sequence function differently on a return?

The sequence of chord labels alone is not the complete harmonic account.

---

## 5. Form sits above event and section detection

A section boundary is useful evidence. It is not yet a formal analysis.

Form may depend on several relations at once:

```text
repetition / variation
motivic identity
phrase organization
cadential strength
harmonic arrival / departure
metrical / hypermetrical organization
texture / instrumentation changes
formal function
loop / return behavior
```

This permits distinctions such as:

```text
same source loop point
!= same perceived phrase boundary

same melody
+ changed harmony
→ transformed return

same section label
+ different cadential role
→ different formal function
```

A higher formal analysis should be able to descend into the lower relations that support it.

---

## 6. Auditory organization and musical structure are different evidence routes

libaural makes an important distinction available to VGM Tooling:

```text
physical source
!= musical part
!= auditory stream
```

For example, several physical voices may fuse into one heard object, while one persistent musical part may migrate across synthesis channels.

A theory analysis may therefore use different evidence routes depending on the source:

```text
SOURCE-AUTHORITATIVE ROUTE
validated authored / driver part identity
→ performed events
→ harmony / counterpoint / form

AUDIO-INVERSE ROUTE
rendered audio
→ libaural auditory organization
→ pitch / stream / part hypotheses
→ harmony / counterpoint / form

HYBRID ROUTE
exact executable evidence
+
auditory grouping evidence
→ compare disagreement
→ preserve both until discriminated
```

The hybrid route is especially valuable as a validation experiment.

---

## 7. Authorship depends on the authorial layer in dispute

Classical musicology and the Sonic 3 case now support the same rule.

Before asking whose work an unknown object resembles, ask:

```text
who could historically have produced it?
what exact authorial role is disputed?
what sources transmit the attribution?
which witnesses are independent?
what version is being attributed?
```

Then compare evidence coordinates appropriate to that role.

### Composition

```text
melody
rhythm
harmony
counterpoint / voice leading
form
motivic construction
cadential habits
large-scale tonal planning
```

### Arrangement / sound programming

```text
register
voicing
texture
channel / voice roles
articulation
modulation
machine-specific realization
```

### Driver / toolchain

```text
command grammar
allocation behavior
scheduler behavior
data layout
compiler / driver artifacts
```

### Patch / sample design

```text
FM topology and parameters
sample preparation
loop strategy
waveform choice
```

These fingerprints may agree or disagree because different people can occupy different roles.

The correct output space includes:

```text
candidate A
candidate B
shared / collaborative
school / circle / team
unknown
none of the current candidates
```

`none of the above` is not an analysis failure.

---

## 8. Uncertainty must propagate as dependency, not arithmetic

Do not multiply confidence numbers mechanically through the stack.

A 0.7 chord hypothesis and a 0.8 key hypothesis do not automatically imply a 0.56 functional analysis.

Instead retain:

```text
claim
+ dependencies
+ evidence status of each dependency
+ analytical method / theory
+ assumptions
+ competing interpretations
+ confidence appropriate to this claim
```

Later evidence may strengthen one dependency without changing another.

A new source can also bypass a weak inference. For example, a surviving score may establish the written chord directly even if audio-only chord recognition was uncertain.

---

## 9. Reasoning and hearing have different clocks

libaural's continuous-sensory direction adds a systems constraint.

```text
source / audio clock
!= auditory-state update clock
!= musical-analysis update clock
!= LLM reasoning clock
```

The song should not cease to exist as an organized state because a language model is busy answering a question.

For realtime use, cheap lower layers may update continuously while deeper harmony, form, attribution, or historical reasoning runs at a slower cadence or retrospectively.

A later analysis may revise an earlier interpretation without rewriting the observations that existed at the time.

---

## 10. Human discourse is a projection over the dependency graph

The same underlying evidence can support several legitimate conversational views.

```text
LISTENER
what changed and what stands out

REVIEWER / CRITIC
what the musical result does and how it may be evaluated

COMPOSER / MUSICIAN
gesture, construction, pacing, interaction, thematic and harmonic design

MUSICOLOGIST / THEORIST
key, chord, progression, counterpoint, cadence, motif, form, style, source and attribution

PRODUCER
energy, arrangement, impact, contrast, intervention

MIXING / MASTERING ENGINEER
spectral, spatial, dynamic, transient and masking relations

FORENSIC / TECHNICAL
exact executable / synthesis / timing / provenance evidence
```

These are viewpoints over one evidence field, not different songs.

A useful response can therefore move vertically:

```text
"the return feels more settled"
        ↓
"the phrase closes more strongly"
        ↓
"the dominant preparation resolves into the local tonic"
        ↓
"these pitch / bass / timing relations support that reading"
        ↓
"these exact performance events produced those relations"
        ↓
"these driver / device events produced the performance"
```

Or horizontally:

```text
listener view
↔ musicologist view
↔ producer view
↔ engineer view
```

without changing the underlying evidence.

---

## 11. What this does not justify yet

Do not add a universal new graph node for every theory term merely because the term appears in this document.

The existing `musical_structure`, `musical_relation`, provenance, attributes, source-relative features, and competing-hypothesis machinery remain sufficient until a concrete analyzer or source adapter proves otherwise.

Do not add yet:

- one canonical Western harmony engine;
- one mandatory chord vocabulary;
- one universal key model;
- one universal form theory;
- one universal listener model;
- one composer embedding treated as authorship truth;
- one total ordering that forbids direct evidence from entering at a higher semantic layer.

The dependency structure should become more explicit only where real analyses need it.

---

## 12. Immediate implementation pressure

The next executable analyses should be built in small earned slices.

A useful order for the current Sonic corpus is:

```text
1. stable performed pitch events / control trajectories
2. persistent-part evidence where source support permits it
3. harmonic segmentation that tolerates arpeggiation and held tones
4. chord candidates with explicit non-chord-tone handling
5. local / global tonal-center candidates
6. progression + harmonic-rhythm representation
7. cadence / phrase evidence
8. motif / transformed-return relations
9. formal hierarchy
10. composition-style coordinates for attribution controls
```

At every step:

```text
known source truth
+
held-out controls
+
competing interpretations
+
source-relative availability
→ only the distinction that survives
```

The target is not to make the language model memorize music-theory vocabulary.

The target is to make the evidence structure itself teach the distinction.