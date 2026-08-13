# Musicological authorship attribution

## Status

Research input for attribution of anonymous, uncertain, disputed, or misattributed music.

This case asks a narrower question than generic composer classification:

> How do musicologists and computational-music researchers decide whether an anonymous or disputed work was actually composed by a particular historical composer?

The durable answer is **not** `find the nearest stylistic fingerprint`.

Authorship attribution is an evidence-convergence problem in which historical possibility, source criticism, transmission, material evidence, internal musical analysis, and computational style analysis constrain one another while preserving the possibility that none of the proposed composers is correct.

This is directly relevant to VGM Tooling's attribution work. Classical authorship research provides a mature precedent for separating technical/style similarity from historical attribution and for treating de-attribution or unresolved authorship as successful outcomes when the evidence does not support a name.

---

## 1. Attribution starts by asking who could have written the work

A classifier can rank any labels it is given. Musicology first asks whether those labels are historically plausible.

Candidate generation may use:

- date or date range of the source/work;
- geography and institutional context;
- court, church, publisher, workshop, performer, or patron networks;
- genre and liturgical/dramatic function;
- instrumentation and available performers;
- known chronology of a composer's career;
- repertory circulation and transmission routes;
- documentary references, catalogues, inventories, correspondence, payment records, title pages, or early attributions;
- known students, associates, copyists, publishers, schools, and local traditions.

The important law is:

```text
historical candidate generation
        ↓
style discrimination among plausible candidates
```

not:

```text
all famous composers
        ↓
nearest style wins
```

The 2025 systematic survey by Federico Simonetta makes this distinction unusually explicit. In the disputed Bach-fugue studies, historical and musicological work first narrowed the candidate set to J. S. Bach, W. F. Bach, and J. L. Krebs, later adding Johann Peter Kellner, before statistical discrimination. The survey criticizes the early computational Josquin work for using a broad curated composer corpus without equally careful candidate narrowing.

For VGM Tooling this is the same guardrail already needed by Sonic 3 attribution:

```text
technical / musical similarity
!= historical possibility
```

A candidate should not enter merely because a model can compare it.

---

## 2. External/source evidence and internal/style evidence are different coordinates

Traditional attribution commonly combines two broad families of evidence.

### External / source-critical evidence

Potential coordinates include:

- earliest surviving witness;
- named attribution in each witness;
- independence or dependence among witnesses;
- source chronology;
- provenance and ownership;
- manuscript or print workshop;
- copyist/scribe identity;
- handwriting and paleography;
- paper stock and watermarks;
- ink, ruling, layout, notation habits, and other codicological features;
- publisher/printer practices;
- corrections and later additions;
- conflicting names in parallel sources;
- whether a composer's name was added later rather than copied with the music;
- transmission paths among manuscripts/prints;
- documented access by the proposed composer or circle.

These facts can establish that a source is early, late, independent, derivative, reliable, commercially motivated, or otherwise biased without proving the composer by themselves.

### Internal musical evidence

Potential coordinates include:

- melodic interval and contour habits;
- rhythmic vocabulary and phrase rhythm;
- harmonic vocabulary and progression;
- cadence types and placement;
- contrapuntal procedures;
- imitation and motivic construction;
- voice leading;
- texture and scoring;
- register and range;
- formal proportions and formal function;
- treatment of text, chant/cantus firmus, schema, quotation, or borrowed material;
- ornamentation and figuration;
- dissonance treatment;
- instrumental idiom;
- recurring compositional strategies rather than isolated surface gestures.

The strongest musicological arguments often connect several of these rather than relying on a single fingerprint.

Peter Wright's 2019 proposed attribution of an anonymous Credo to John Dunstaple is a useful model because the argument explicitly combines **structural, stylistic, textual, and transmissional** links to securely attributed Dunstaple works. The point is methodological: style alone was not treated as sufficient.

Alexander Silbiger's 2023 study of the work traditionally called Froberger's *Toccata XX* demonstrates the opposite but equally important result. Eight early sources distribute the piece among anonymity and three named composers, and the surviving evidence does not securely identify one author. The appropriate output is therefore uncertain authorship, possibly `school of Froberger`, rather than forcing the most famous candidate.

---

## 3. Source reliability itself must be modeled

An attribution written on an old page is evidence, not automatically truth.

A source witness should carry at least:

```text
source identity
source date / range
provenance
scribe / printer / workshop when known
attribution as written
whether attribution is contemporary or later
relationship to other witnesses
known copying / editorial behavior
reliability for this repertoire
variant readings
uncertainty
```

Several witnesses repeating one attribution may represent one copied assertion rather than several independent confirmations.

```text
10 derivative witnesses
!= 10 independent witnesses
```

Likewise, an anonymous early source can sometimes outrank a much later named source if the later attribution entered during transmission.

The Josquin repertory is an especially strong warning. His fame encouraged pieces to circulate under his name, so a famous attribution can itself be a historical confound. The attribution problem is partly a source-transmission problem before it is a classification problem.

For older repertories, material/source evidence can include watermarks, handwriting, scribal practice, notation, and source genealogy. These should remain separate from compositional style because they may identify a **copyist or provenance route** without identifying the composer.

```text
scribe identity
!= composer identity
```

---

## 4. Exact and near musical correspondences can be stronger than global style similarity

Musicologists often investigate local correspondences that have plausible historical meaning:

- shared unusual motifs;
- distinctive contrapuntal solutions;
- repeated schemas or cadential constructions;
- reuse of material from another securely attributed work;
- highly specific text-setting decisions;
- matching formal plans;
- related compositional errors/corrections;
- transformation of a known model;
- quotation or parody technique;
- close links among works known to circulate together.

The evidentiary question is not merely `how similar are the two pieces?` but:

> Is the correspondence distinctive, historically plausible, and more expected under common authorship than under shared period/style/tradition?

This suggests a useful distinction for VGM Tooling:

```text
global style fingerprint
local distinctive correspondence
historical transmission relation
```

These should not be collapsed into one similarity score.

---

## 5. Computational stylometry is a supporting witness

The 2025 TISMIR systematic review, covering 58 peer-reviewed composer-identification/attribution studies, is the strongest current methodological summary.

The literature has used many representations and models, including:

- hand-designed musicological/style features;
- pitch, interval, chord, rhythm, texture, and voice-leading statistics;
- n-grams and substring matching;
- information-theoretic measures;
- Markov/language models;
- distance and nearest-neighbor methods;
- discriminant analysis and classical statistical classifiers;
- support-vector methods;
- random forests / ensembles / AutoML;
- neural and representation-learning approaches.

The transferable lesson is not that one architecture wins. Attribution reliability depends at least as much on **what corpus was built, which candidates were chosen, which editorial confounds survived, and how evaluation was performed**.

### GitHub observatories

#### `josquin-research-project/jrp-scores`

A particularly valuable corpus for VGM Tooling because it contains computationally usable Humdrum scores for Renaissance composers and an explicit anonymous repertoire.

It also preserves important editorial distinctions. For example, editorially supplied accidentals are marked separately from accidentals actually present in the source, and interpreted modern metric structures are distinguished from original mensural notation. That is exactly the kind of distinction attribution research needs to prevent an editor's decisions from masquerading as a composer's fingerprint.

Use as:

```text
historical symbolic corpus
+ anonymous/disputed pressure surface
+ encoding-provenance lesson
```

not as an automatically authoritative composer label set.

#### `DIDONEproject/music_symbolic_features`

Reproducible benchmarking code for jSymbolic, musif, and music21 feature families. Its datasets include Josquin versus La Rue and Haydn/Mozart/Beethoven string quartets.

This is useful for testing whether different feature families recover the same attribution signal and for detecting when an apparent fingerprint is extractor-specific.

#### `DIDONEproject/musif`

A reusable symbolic music feature-extraction library intended for computational musicology, including music21 integration and corpus analysis.

Use it as an observatory for feature definitions and extraction methodology, not as a canonical VGM feature schema.

#### `DDMAL/jSymbolic2`

A broad symbolic feature extractor designed for MIR, theory, and musicology. Its separation of feature definitions from extracted values remains especially useful for attribution because every claimed fingerprint must specify what was measured.

#### `cuthbertLab/music21`

Broad musicological analysis environment for pitch, harmony, counterpoint, streams, corpus analysis, Roman-numeral theory, and transformations. Useful for independent symbolic projections and feature construction.

#### OMR / writer-identification datasets

`OMR-Research/muscima-pp` and CVC-MUSCIMA include writer-identification research. Their modern handwritten material does not solve historical composer attribution, but the task boundary is useful:

```text
written-hand / scribe fingerprint
!= compositional fingerprint
```

Historical manuscript analysis may combine both coordinates when appropriate.

---

## 6. Confounds can make a composer classifier look much better than it is

The 2025 systematic review emphasizes several attribution-specific confounds.

### Period / school / genre leakage

A model separating Bach from Chopin may mostly be separating historical periods.

A meaningful attribution test should prefer candidate composers who were realistically confusable:

```text
same or nearby period
same/similar genre
similar instrumentation
related school/geography when historically plausible
```

The hard question is whether the model distinguishes **individual compositional strategy** rather than broad cultural categories.

### Editorial leakage

Different collected editions can encode:

- accidentals differently;
- articulations differently;
- ornaments differently;
- repeats or variants differently;
- metadata conventions differently;
- normalization practices differently.

A classifier may learn `editor/publisher` instead of `composer`.

### Performance / recording leakage

For compositional authorship, a recording contains performer, room, instrument, engineering, and performance-practice signatures. Those may be useful evidence for other questions, but they are confounds if the claim is intrinsic compositional authorship.

VGM Tooling has an unusual advantage here: exact executable source state can expose composition/arrangement/driver/sound-programming coordinates separately instead of asking final audio to carry all authorship evidence.

### Fragment leakage and related-work leakage

Movements, themes, arrangements, or duplicated editions from the same work must not be split carelessly across train and test sets. Otherwise the model may recognize the work rather than the composer.

---

## 7. Unknown authorship is an open-set problem

One of the most important weaknesses in historical computational attribution is the closed-set assumption:

```text
one of these candidates must be correct
```

For genuine unknown works the correct answer may be:

```text
none of these candidates
unknown member of the same school
collaborative / adapted work
insufficient evidence
```

Therefore VGM Tooling should treat attribution as an **open-world hypothesis problem** even if a particular experiment uses a finite comparison set.

Conceptually:

```text
candidate A probability / support
candidate B probability / support
candidate C probability / support
residual / unknown support
model epistemic uncertainty
source uncertainty
conflicting evidence
```

The Simonetta review specifically recommends moving toward open-set reasoning and highlights Bayesian uncertainty as a useful path. High epistemic uncertainty can signal that the questioned work lies outside the stylistic region represented by the candidate corpus.

A low-confidence `unknown` is a better scientific result than a confident wrong name.

---

## 8. Validation must match the actual attribution question

For a computational attribution claim, require at least:

1. **Musicologically plausible candidate set**
   - justified independently of the classifier.

2. **Comparable corpus**
   - similar period, genre, medium, scale, and source conditions where possible.

3. **Transparent encoding/editorial protocol**
   - identify normalized, supplied, or editorial material.

4. **Held-out validation**
   - questioned work must not participate in model selection.

5. **Grouped leakage control**
   - related movements/editions/arrangements cannot leak across folds.

6. **Class-balanced evaluation**
   - prefer balanced accuracy and/or MCC where appropriate rather than raw accuracy alone.

7. **Cross-validation**
   - curated corpora are often too small for one lucky hold-out split.

8. **Sensitivity analysis**
   - rerun under alternative encodings, feature families, candidate sets, and editorial treatments.

9. **Negative/control composers**
   - include historically plausible near-neighbors and deliberately implausible controls where useful.

10. **Open-set / abstention behavior**
    - model must be allowed to reject all candidates.

11. **Interpretability**
    - identify which musical coordinates drove the result and whether they are plausibly authorial.

12. **Independent convergence**
    - computational style evidence should be compared with source/transmission/historical evidence rather than counted twice as if independent.

The review's strongest warning is worth preserving as project law:

> A work should not be treated as re-attributed because one model produced a high score. Strong re-attribution requires multiple meticulous studies across data collections, candidate definitions, tests, and musicological evidence.

---

## 9. Attribution is multi-coordinate, not one composer fingerprint

VGM Tooling already separates creative roles because executable game music can distribute composition and realization across several people.

The classical attribution pass strengthens that model.

A useful evidence vector is:

```text
HISTORICAL POSSIBILITY
when / where / network / chronology / access

SOURCE / TRANSMISSION
witnesses / copyists / printers / provenance / conflicting attributions

COMPOSITION
melody / rhythm / harmony / counterpoint / form / motifs

SCORING / ARRANGEMENT
voicing / instrumentation / register / texture

NOTATIONAL / EDITORIAL
source notation / supplied material / edition interventions

DISTINCTIVE CORRESPONDENCE
rare local constructions / reuse / quotation / related models

COMPUTATIONAL STYLE
feature distributions / sequence models / learned representations

NEGATIVE EVIDENCE
things expected for a candidate that are conspicuously absent or incompatible
```

For VGM add the existing executable coordinates:

```text
SOUND PROGRAMMING
DRIVER / TOOLCHAIN
PATCH / SAMPLE DESIGN
RENDERING
```

Evidence may point to different people on different coordinates.

```text
same composition style
+ different implementation fingerprint
```

can suggest arrangement/programming by another contributor rather than invalidate the composition attribution.

---

## 10. Candidate status should be graded and reversible

Avoid one Boolean `composer = X` field.

Useful claim states include:

```text
documented / source-attested
strongly supported attribution
probable attribution
plausible candidate
school / circle / workshop relation
conflicting attribution
traditional attribution under challenge
de-attributed
anonymous / unresolved
excluded candidate
```

Each claim should carry:

```text
candidate person / group
role being attributed
work/version scope
evidence coordinates
supporting sources
counterevidence
model/theory identity where applicable
confidence
historical/cultural scope
open-set state
provenance
```

A later discovery must be able to revise the attribution without rewriting the music or its lower evidence.

---

## 11. Direct transfer to VGM Tooling and Sonic 3

The mature musicological pattern is extremely close to the attribution architecture VGM Tooling needs.

```text
CLASSICAL UNKNOWN WORK
source witnesses
+ historical candidate narrowing
+ internal musical style
+ transmission
+ computational stylometry
→ cautious authorship claim

SONIC 3 UNKNOWN / DISPUTED CONTRIBUTION
prototype/final source witnesses
+ documented personnel / chronology
+ composition fingerprint
+ arrangement/sound-programming fingerprint
+ SMPS / driver / patch / sample evidence
+ cross-track correspondences
+ independent historical evidence
→ cautious role-relative attribution claim
```

The transferable laws are:

1. **candidate generation precedes ranking;**
2. **similarity is evidence, not identity;**
3. **source provenance can outweigh style;**
4. **different creative coordinates can have different authors;**
5. **editorial/toolchain artifacts must not masquerade as style;**
6. **none-of-the-above must remain possible;**
7. **de-attribution is a valid positive result;**
8. **strong claims require convergence across genuinely independent evidence.**

This gives the Sonic case a historical musicology analogue rather than treating game-music attribution as an invented problem.

---

## 12. Implementation pressure

No new universal graph ontology is justified yet. The current provenance-aware model can represent the first executable controls using competing attribution hypotheses, external annotations, source objects, musical-structure/style features, role-relative evidence, confidence, and provenance.

The next bounded attribution regression should demonstrate:

```text
same unknown work evidence
        ↓
plausible candidate set from external/history evidence
        ↓
separate style/source/toolchain evidence bundles
        ↓
several candidate hypotheses + explicit unknown hypothesis
        ↓
new evidence changes ranking
        ↓
exact lower musical/source evidence remains unchanged
```

A second regression should prove that two evidence coordinates can disagree without being averaged into nonsense:

```text
composition evidence → candidate A
copyist/toolchain evidence → candidate B
```

The result may be collaboration, copying, arrangement, adaptation, mistaken source attribution, or unresolved conflict. It must not automatically choose whichever coordinate has the larger numeric score.

---

## Primary repository observatories checked

- `josquin-research-project/jrp-scores`
- `DIDONEproject/music_symbolic_features`
- `DIDONEproject/musif`
- `DDMAL/jSymbolic2`
- `cuthbertLab/music21`
- `humdrum-tools/humdrum-data`
- `OMR-Research/muscima-pp` / CVC-MUSCIMA as a writer-identification boundary case

## Primary literature anchors

- Simonetta, Federico (2025), **Style-Based Composer Identification and Attribution of Symbolic Music Scores: A Systematic Survey**, *Transactions of the International Society for Music Information Retrieval* 8(1), 213–235. DOI `10.5334/tismir.240`.
- Backer, Eric & Peter van Kranenburg (2005), **On musical stylometry: a pattern recognition approach**, *Pattern Recognition Letters* 26(3), 299–309. DOI `10.1016/j.patrec.2004.10.016`.
- van Kranenburg, Peter (2006/2008 line of work), **Composer attribution by quantifying compositional strategies** and subsequent disputed-Bach attribution analysis.
- Brinkman, Alexander, Daniel Shanahan & Craig Sapp (2016), **Musical stylometry, machine learning, and attribution studies: A semi-supervised approach to the works of Josquin**.
- McKay et al. (2018), follow-up computational attribution work on Josquin-related corpora with stronger validation controls.
- Wright, Peter (2019), **A New Attribution to Dunstaple**, *Music & Letters* 100(2), 196–232. DOI `10.1093/ml/gcz031`.
- Silbiger, Alexander (2023), **Who Wrote Froberger's Toccata XX? Questions of Author Attribution in Early Keyboard Music**, *Journal of Seventeenth-Century Music* 29(1).
- Cumming, Julie, Cory McKay, Julie Stuchbery & Ichiro Fujinaga (2018), **Methodologies for creating symbolic corpora of Western music before 1600**, ISMIR 2018.
- Simonetta et al. (2023), **Optimizing Feature Extraction for Symbolic Music**, ISMIR 2023.

## Result

The durable attribution law is:

> **Treat authorship as a historically constrained, provenance-bearing hypothesis supported by converging independent evidence. Never turn nearest style, nearest source, or nearest tool fingerprint into identity by itself.**
