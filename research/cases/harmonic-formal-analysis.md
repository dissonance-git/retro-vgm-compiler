# Harmonic and formal analysis

## Status

Research input for the `musical_structure` layer.

This case narrows the earlier music-cognition/musicology work around the user's desired upper musical layer:

> **keys, tonal centers, chords, progressions, harmonic rhythm, modulation, cadence, voice leading, motivic/harmonic hierarchy, and form**

The target is not merely to identify when a texture changes or when a new lead enters. Those are useful lower structural/perceptual observations. A musicological account should be able to explain **what the pitch/rhythm organization is doing and how local events participate in larger musical relations**.

---

## 1. Section detection is not the ceiling

A useful listening timeline may say:

```text
intro
→ groove establishes
→ lead enters
→ texture expands
→ return
```

That remains valuable, but it is not equivalent to a music-theory analysis.

The same span may support a higher account such as:

```text
home tonal area
→ predominant motion
→ dominant preparation
→ local tonicization
→ cadence
→ return / recomposition in the home key
```

or, in music not well described by common-practice function:

```text
pitch center / collection A
→ contrasting collection or harmonic field
→ recurring progression / schema
→ transformed return
→ large-scale pitch/form relation
```

The durable distinction is:

```text
section / energy / texture description
!= harmonic / contrapuntal / formal analysis
```

Both should remain aligned to the same lower evidence.

---

## 2. Harmony is a stack of inference problems

Do not define harmonic understanding as `simultaneous notes → chord label`.

A stronger ladder is:

```text
EXACT / DERIVED PERFORMANCE EVIDENCE
pitch trajectories
onset / duration / release
bass motion
persistent-part hypotheses
metric / rhythmic position
        ↓
HARMONIC SEGMENTATION
which span is one harmonic event?
which changes are ornamental / figurational?
        ↓
CHORD-TONE / FIGURATION ANALYSIS
structural tones
passing / neighbor / suspension / anticipation / other elaboration
        ↓
CHORD REPRESENTATION
root
spelling
quality / type
inversion
alteration / extension
        ↓
TONAL CONTEXT
global center / key
local center / key
tonicization
modulation
mode / collection where applicable
        ↓
HARMONIC RELATION
function where the theory supports it
preparation
prolongation
substitution
cadential relation
        ↓
PROGRESSION + HARMONIC RHYTHM
ordered/nested chord relations
rate and placement of harmonic change
metric relation
recurring schema
        ↓
VOICE LEADING / COUNTERPOINT
motion among persistent parts
common tones
linear patterns
contrapuntal dependencies
        ↓
PHRASE / CADENCE / HIERARCHY
open/closed spans
preparation and resolution over longer windows
nested harmonic dependency
        ↓
FORM
repetition / variation
formal function
harmonic anchor points
motivic relations
large-scale tonal path
```

These are analytical dependencies, not a mandatory serial runtime pipeline.

Later context may revise an earlier interpretation while leaving the exact pitched events untouched.

---

## 3. Surface notes are not automatically chord tones

Finkensiep, Ericson, Klassmann & Rohrmeier's 2025 paper *Chord Types and Figuration: A Bayesian Learning Model of Extended Chord Profiles* provides a particularly useful pressure test.

Its model explicitly distinguishes:

```text
chord tones
!= figuration tones
```

and learns chord-specific profiles for both. The results show that typical figuration patterns differ by chord type and style, supporting the larger conclusion that harmony and figuration cannot always be solved independently.

For VGM Tooling this matters enormously because executable game music contains many ways to create misleading vertical pitch sets:

- rapid arpeggiation;
- staggered channel attacks;
- programmed echo/delay;
- ornamentation;
- suspension-like held notes;
- pitch envelopes;
- retriggers;
- short passing notes;
- channel reuse;
- voice stealing;
- bass anticipation;
- loops whose control boundaries do not coincide with harmonic boundaries.

Therefore:

```text
all pitches active at tick t
!= chord at tick t
```

A harmonic analysis needs duration, order, bass, meter, voice leading, persistent identity, and surrounding context.

Do not throw away a pitch merely because one chord hypothesis calls it `non-chord`. Preserve the event and attach the figurational interpretation as a higher hypothesis.

---

## 4. Harmonic segmentation is itself a hypothesis

Before asking `what chord is this?`, ask:

> **What time span should be treated as one harmonic event?**

Possible evidence includes:

- bass change;
- entry/release of structurally important tones;
- metric position;
- duration;
- accent;
- phrase location;
- voice-leading resolution;
- stable pitch collection;
- repetition across parallel phrases;
- known authored/driver grouping when available.

Several segmentations may coexist.

```text
same exact notes
→ slow harmonic rhythm reading
→ faster surface-chord reading
```

This matters for game music because an arpeggiated or sparsely voiced harmony may never present its complete pitch collection simultaneously.

---

## 5. Key is hierarchical, not one global string

A single `key = X` field is insufficient.

A tonal piece may have:

```text
global tonic / tonal center
local key regions
tonicizations
modulations
modal mixture / altered regions
ambiguous pivots
returns whose confirmation occurs later
```

Useful analysis state therefore includes:

```text
global tonal-center hypothesis
local tonal-center hypothesis
time span
relation to parent region
supporting pitch / cadence / bass / progression evidence
confidence
theory/model scope
competing alternatives
```

The computational music-theory literature on hierarchical key structure, including Pitch Scapes work by Lieck & Rohrmeier, reinforces the need to represent tonal organization at multiple timescales rather than selecting one key independently frame by frame.

For non-functional or non-Western repertoires, replace the specific `key` assumptions with the appropriate pitch-center, collection, mode, tuning, or tradition-specific analysis. Failure of a Roman-numeral model must never become `no harmonic structure`.

---

## 6. A chord symbol is not harmonic function

Keep these separate:

```text
pitch content
→ chord representation
→ relation to local tonal context
→ harmonic function
→ hierarchical role
→ listener expectation / tension
```

For example, a sonority can be identified reliably as a dominant-seventh pitch structure while its function remains ambiguous because:

- the local tonic is uncertain;
- it is a secondary dominant;
- it participates in a sequence;
- it is prolongational rather than cadential;
- it is a passing/neighboring sonority;
- the repertoire's theory does not assign it common-practice dominant function.

Roman numerals are therefore theory-scoped analytical claims, not aliases for pitch-class sets.

---

## 7. Progressions are temporal and hierarchical

`DCMLab/harmonic-syntax-in-time-code`, accompanying Harasim, O'Donnell & Rohrmeier's 2019 *Harmonic Syntax in Time*, demonstrates that harmonic grammar improves when rhythmic/grouping structure is modeled with it.

Their combined model integrates:

```text
hierarchical harmonic syntax
+
hierarchical rhythmic information
+
grouping
+
meter
```

and outperforms the single-domain models on the annotated jazz corpus.

The direct lesson for VGM Tooling is:

```text
chord sequence alone
!= progression understanding
```

A useful progression representation must retain:

- order;
- duration;
- harmonic rhythm;
- metric position;
- grouping/phrase context;
- preparation/resolution relations;
- nesting/prolongation where supported.

`I - ii - V - I` is a useful surface summary. A deeper account asks which spans prepare or prolong others, where closure occurs, and which intermediate sonorities are structural versus elaborative.

---

## 8. The Jazz Harmony Treebank is a high-value hierarchy teacher

`DCMLab/JazzHarmonyTreebank` contains expert hierarchical analyses of jazz chord sequences rather than only flat chord labels.

Its structure gives VGM Tooling a concrete external example of:

```text
chord leaves
→ nested harmonic constituents
→ phrase/reference structure
```

The treebank also preserves key, meter, chord onset positions, turnaround behavior, alternate readings/comments, and more than one tree analysis where applicable.

Use it as a hierarchy/annotation observatory, not as a universal grammar for game music.

The transferable obligation is:

> **A harmonic span can function as a unit whose role depends on a larger span.**

---

## 9. Multiple analyses are a feature, not a bug

`MarkGotham/When-in-Rome` is especially useful because it is a meta-corpus of encoded expert functional analyses and can preserve alternative analyses of the same work.

That matches VGM Tooling's evidence law:

```text
same performance evidence
→ analysis A
→ analysis B
```

without rewriting the source.

A music-theory model should therefore report something closer to:

```text
READING A
local key: G
chord: V/V
function: dominant preparation
confidence: 0.63

READING B
local key: D
chord: V
function: local dominant
confidence: 0.31
```

than pretending one label is exact merely because it won a decoder.

Different analytical systems may also legitimately disagree because they ask different questions.

---

## 10. ChoCo supplies breadth and notation pressure

`smashub/choco` integrates more than 20,000 timed chord annotations from many repertoires and source types.

Useful distinctions include:

- audio versus symbolic annotations;
- chord versus tonality/modulation annotations;
- time in seconds versus measure/beat;
- Harte/lead-sheet notation versus Roman-numeral notation;
- provenance for annotations;
- genre/style diversity.

The lesson is interoperability without ontological collapse:

```text
normalize enough to compare
!= declare one chord notation universal
```

VGM Tooling should preserve its source-derived musical evidence and project into these analysis vocabularies when useful.

---

## 11. AugmentedNet is a challenger, not an authority

`napulen/AugmentedNet` performs automatic Roman-numeral analysis from symbolic scores with a multitask model.

Its decomposition is useful because it does not hide everything in one opaque class. Relevant coordinates include:

- key;
- scale degree;
- quality;
- inversion;
- root;
- combined Roman-numeral output.

For VGM Tooling:

```text
automatic harmonic model
= external hypothesis generator
!= source truth
```

A future experiment can compare:

```text
VGM source-derived pitch/timing evidence
→ project to symbolic representation
→ AugmentedNet hypothesis

same evidence
→ project to another harmony model
→ alternative hypothesis

human/corpus analysis where available
→ external annotation
```

and retain disagreement explicitly.

---

## 12. music21, hrep, musif and dimcat occupy different observatories

### `cuthbertLab/music21`

Broad symbolic/musicological laboratory for notes, chords, keys, Roman numerals, voices, streams, corpus analysis and transformations.

Use for independent symbolic analysis and validation, not as the executable music ontology.

### `pmcharrison/hrep`

Important because one chord can be represented symbolically, acoustically, and perceptually/sensorily without one representation replacing the others.

This reinforces:

```text
structural chord
!= acoustic realization
!= sensory representation
```

### `DIDONEproject/musif`

Corpus-scale feature extraction for computational musicology, useful for transparent feature definitions and independent measurements.

### `DCMLab/dimcat`

Digital/cognitive musicology tooling for corpus-scale analysis. Useful above the per-piece analysis layer once VGM Tooling can generate comparable provenance-bearing musical objects across a soundtrack corpus.

---

## 13. Form sits above section segmentation

Rohrmeier & Neuwirth's 2025 *A Theoretical Model of Musical Form* is a useful correction to flat section labels.

The model characterizes form through interacting coordinates including:

- segmentation/grouping;
- hierarchical grouping structure;
- meter and hypermeter / rhythmic partitioning;
- repetition and degree of variation;
- formal function;
- schemata;
- harmonic anchor points.

This supports a VGM Tooling target of:

```text
section / phrase observations
+
metrical hierarchy
+
harmonic trajectory
+
repetition / transformation
+
motivic relations
+
formal function
→ formal analysis hypothesis
```

A literal driver loop can be exact while its musical function remains analytical.

Example:

```text
exact execution loop boundary
!= phrase boundary
!= formal return
!= harmonic closure
```

The same loop may cut across a pickup, sustain, delayed echo, or harmonic continuation.

---

## 14. Form and harmony should be able to explain one another

A strong song-level model should support bidirectional constraints:

```text
local harmony
→ supports phrase/cadence/form reading

formal parallelism
→ helps interpret ambiguous local harmony
```

Example:

- a sonority is locally ambiguous;
- the corresponding point in an earlier parallel phrase has a clear function;
- the later phrase is a transformed return;
- that relation raises one harmonic reading without making it exact.

This is one reason whole-song reasoning cannot be implemented as independent labels computed frame by frame.

---

## 15. libaural is the perceptual counterpart, not a duplicate theory engine

The newest libaural work strengthens the lower perceptual prerequisites:

```text
pitch / timbre evidence
→ concurrent grouping
→ persistent auditory objects / streams
→ memory / continuity
→ selective invariance
→ heard world
```

That helps VGM Tooling answer questions such as:

- which tones are heard together;
- whether a sustained tone remains perceptually attached to one part;
- whether an arpeggio is heard as one harmonic object;
- whether a melody migrates between physical channels while remaining one stream;
- whether masking makes a nominally present pitch perceptually weak.

But the division remains useful:

```text
libaural
what auditory organization does this acoustic evidence support?

VGM Tooling musicology
what musical/theoretical relations do these performed/heard events support?
```

The two can challenge each other without collapsing.

---

## 16. Evidence law for theory outputs

For harmony:

```text
exact source/device pitch state
        ↓
derived performed pitch events
        ↓
candidate harmonic segmentation
        ↓
chord-tone / figuration hypothesis
        ↓
chord spelling/root/inversion hypothesis
        ↓
key / tonal-center hypothesis
        ↓
function / progression hypothesis
        ↓
hierarchical / cadence / form hypothesis
        ↓
listener expectation / tension hypothesis
```

Do not skip levels merely because a model emits the final label.

Every theory object should retain:

```text
time span
supporting events/parts
theory/model identity
style/cultural/corpus scope
confidence
evidence status
provenance
alternatives
```

---

## 17. Cross-cultural and style scope

No default VGM model may silently equate `music theory` with common-practice Western harmony.

The analysis system must be able to say:

```text
this Roman-numeral model is not applicable here
```

without saying:

```text
this music has no structure
```

Possible alternative frameworks may involve:

- modal / scalar-center analysis;
- pitch-class or set relations;
- jazz/pop harmonic conventions;
- blues-derived syntax;
- loop-based harmonic fields;
- transformational relations;
- non-Western modal/tuning/rhythmic theories;
- repertoire-specific schema models;
- mixed/hybrid analyses.

Game music is especially likely to cross these boundaries inside one soundtrack.

---

## 18. Immediate implementation pressure

No new universal semantic layer is justified. `musical_structure` and provenance-bearing `musical_relation` hypotheses remain sufficient for the first controls.

The first synthetic regression should prove this ladder can coexist over unchanged lower evidence:

```text
exact performed notes
        ↓
2 candidate harmonic segmentations
        ↓
2 chord-tone / figuration readings
        ↓
2 chord/key readings
        ↓
2 progression/function readings
        ↓
2 higher phrase/form readings
```

The test should explicitly include:

1. a passing/neighbor tone that must not force a fake chord change;
2. an arpeggiated chord whose pitches are never all simultaneous;
3. a local tonicization that does not automatically become a global modulation;
4. a later cadence that retrospectively changes the preferred earlier reading;
5. a repeated formal span whose second occurrence changes voicing while retaining harmonic identity;
6. a non-functional control for which Roman-numeral analysis is marked `not_applicable` rather than failed/zero.

Then use a bounded Sonic corpus cue as real pressure once source-derived pitches and part continuity are strong enough.

The first real-corpus success criterion is not `print chord names`. It is:

> **Produce a defensible time-aligned harmonic account in which each key/chord/progression/form claim can descend into exact executable musical evidence and competing readings survive when they should.**

---

## Primary GitHub observatories checked

- `cuthbertLab/music21`
- `MarkGotham/When-in-Rome`
- `smashub/choco`
- `napulen/AugmentedNet`
- `DCMLab/JazzHarmonyTreebank`
- `DCMLab/harmonic-syntax-in-time-code`
- `DCMLab/probabilistic_harmony_model`
- `DCMLab/mozart_piano_sonatas`
- `DCMLab/ABC`
- `DCMLab/dimcat`
- `pmcharrison/hrep`
- `pmcharrison/voicer`
- `DIDONEproject/musif`
- `DDMAL/jSymbolic2`
- `humdrum-tools/humlib`

## Primary literature anchors

- Finkensiep, Christoph, Petter Ericson, Sebasian Klassmann & Martin Rohrmeier (2025), **Chord Types and Figuration: A Bayesian Learning Model of Extended Chord Profiles**, *Music & Science* 8. DOI `10.1177/20592043241291661`.
- Harasim, Daniel, Timothy J. O'Donnell & Martin Rohrmeier (2019), **Harmonic Syntax in Time: Rhythm Improves Grammatical Models of Harmony**, ISMIR 2019, pp. 335–342. DOI `10.5281/zenodo.3527812`.
- Harasim, Daniel, Christoph Finkensiep, Petter Ericson, Timothy J. O'Donnell & Martin Rohrmeier (2020), **The Jazz Harmony Treebank**, ISMIR 2020, pp. 207–215. DOI `10.5281/zenodo.4245406`.
- Lieck, Robert & Martin Rohrmeier (2020), **Modelling Hierarchical Key Structure With Pitch Scapes**, ISMIR 2020, pp. 811–818. DOI `10.5281/zenodo.4245558`.
- Rohrmeier, Martin & Markus Neuwirth (2025), **A Theoretical Model of Musical Form**, ISMIR 2025, pp. 326–333. DOI `10.5281/zenodo.17706401`.
- Gotham et al. (2023), **When in Rome: A Meta-corpus of Functional Harmony**, *Transactions of the International Society for Music Information Retrieval*. DOI `10.5334/tismir.165`.
- Nápoles López, Néstor, Mark Gotham & Ichiro Fujinaga (2021), **AugmentedNet: A Roman Numeral Analysis Network with Synthetic Training Examples and Additional Tonal Tasks**, ISMIR 2021. DOI `10.5281/zenodo.5624533`.

## Result

The durable theory law is:

> **Hear local events first, but reason over them hierarchically. A chord is not a pitch set, a progression is not a chord list, a key is not one global label, and form is not a list of section boundaries.**
