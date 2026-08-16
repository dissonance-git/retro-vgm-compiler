# Sonic 3 harmonic / tonal / formal testbed

Status: active executable research lane  
Primary executable entry point: `python tools/sonic3_testbed.py vgm-harmonic-probe`  
Shared-model scope: tonal center, structural pitch collection, key class, chord degree, functional tendency, cadence, tonal-region relation, voice leading, bass/harmony interaction, phrase/form relations

## Purpose

Sonic 3 & Knuckles is now the primary real-corpus pressure test for the harmonic layer as well as the attribution layer.

The question is not:

> Can we print chord names from a VGM?

It is:

> Can execution evidence be transformed, with explicit uncertainty, into the same tonal and formal relationships a composer would recognize, while preserving the difference between sounding surface, persistent musical parts, structural harmony, key, function, cadence, modulation, and form?

The desired ladder is:

```text
VGM / SMPS execution
        ↓
operator-aware performed pitch
        ↓
persistent musical parts
        ↓
simultaneous and arpeggiated harmonic evidence
        ↓
structural pitch collection
        ↓
tonal center
        ↓
key / mode where the theory supports it
        ↓
scale degree / chord relation
        ↓
functional tendency
        ↓
cadential arrival / cadence class
        ↓
local tonal regions
        ↓
tonicization / modulation / return
        ↓
phrase and section harmonic trajectory
        ↓
whole-song form
        ↓
creator-level harmonic grammar across independent soundtracks
```

Every arrow is an inference boundary. No lower object is allowed to silently rename itself as a higher object.

## 1. Current executable VGM surface lane

`tools/sonic3_harmonic_probe.py` provides the first real-corpus bridge.

It reads only Genesis VGM execution and corpus-directory identity. It does **not** read curated Sonic 3 attribution hypotheses, artist/composer tags, or external target-control people.

The executable path is currently:

```text
VGM YM2612 writes
    ↓
active ordinary full-key FM state
    ↓
shared-model nominal FNUM/BLOCK frequency equation
    ↓
static operator-network periodicity
    ↓
performed-pitch hypothesis
    ↓
12-TET projection with cents residual
    ↓
physical-channel surface verticality
    ↓
triad / root / inversion surface hypotheses
    ↓
root-motion and quality-transition statistics
    ↓
provisional center+mode ranking
    ↓
provisional functional-shape counts
```

This is deliberately called a **surface harmonic probe**.

It does not bypass the shared C++ model's stronger gates.

### YM2612 pitch contract

The probe mirrors the same nominal-frequency equation as `genesis_nominal_pitch.h`:

```text
nominal_hz = clock * FNUM * 2^BLOCK / (144 * 2^21)
```

It then mirrors the static operator-network periodicity reasoning used by `ym2612_fundamental_hypothesis.h`:

- OPN MULT register `0` means `0.5x`;
- values `1..15` mean `1x..15x`;
- the static multiplier lattice can support a common periodicity when all operators are fully keyed;
- detune invalidates the simple rational periodicity projection;
- active LFO phase modulation with nonzero FMS invalidates one static performed-pitch value;
- a lower common periodicity with no direct carrier is preserved as a missing-fundamental-style hypothesis with the same confidence ceiling used by the shared model.

The lane therefore does not use a generic `FNUM → nearest MIDI note` shortcut.

### Stable state grouping

All VGM writes at one source tick are processed before the next-duration segment is evaluated.

This prevents split low/high FNUM writes, same-tick key-off/key-on changes, and multi-channel chord attacks from becoming false intermediate harmonic states.

Partial operator-key masks are excluded from the clean surface because release-envelope contribution is not reconstructed there.

## 2. What the surface lane may report

The current output may report:

- performed-pitch projection coverage;
- unresolved performed-pitch time by reason;
- duration-weighted pitch-class surface;
- polyphonic and triadic surface duration;
- major/minor/diminished/augmented triad duration;
- unambiguous triad roots;
- inversion surface;
- root-motion transitions;
- chord-quality transitions;
- provisional center/mode rankings;
- provisional predominant→dominant and dominant→tonic **shapes** under the best surface ranking;
- cross-soundtrack nearest neighbors in a transposition-insensitive surface-harmonic feature space.

These are useful observables and challenger hypotheses.

They are not yet the final musical interpretation.

## 3. What remains intentionally blocked

The surface lane must continue to report the following shared-model promotions as blocked:

```text
resolved tonal center
resolved key / mode
resolved functional tendency
cadence class
tonicization
modulation
whole-song tonal form
composer grammar
```

until their actual evidence contracts are met.

The main reasons are:

### Physical channel is not persistent musical part

A melody, bass, inner voice, pad, or countermelody can migrate between channels. A physical YM2612 channel can also change musical role.

Therefore:

```text
YM2612 channel 2
!= musical bass part
```

Persistent-part recovery must sit between execution and structural voice leading.

### Sounding pitch collection is not structural pitch collection

The surface can contain:

- passing tones;
- neighbors;
- suspensions;
- anticipations;
- arpeggiation;
- echo-like duplication;
- pitch envelopes;
- voice stealing;
- transient overlap;
- ornamental chromaticism.

Therefore:

```text
all sounding pitch classes
!= key-defining scale collection
```

The shared key gate correctly refuses a surface-performance collection.

### One surface evidence family cannot ground tonal center strongly

Pitch distribution, triad roots, and low-note support can all be downstream views of the same physical verticalities.

They must not be counted as independent votes merely because they have different names.

The exploratory tonal candidate therefore retains the single-support ceiling instead of being promoted to a resolved center.

### Function requires transition identity

`G major in a provisional C-Ionian field` can look like degree 5.

That is not yet the same statement as:

```text
V with dominant function
```

The shared functional layer requires a resolved key, compatible chord-degree evidence, reliable transition motion, and stronger voice/phrase evidence before confidence can rise.

### Cadence requires phrase arrival

A `V → I` surface transition in the middle of a loop is not automatically a cadence.

PAC/IAC classification additionally requires the inversion and final soprano evidence defined by the shared cadence model.

### Modulation requires a region, not a local chord

A foreign chord or short center excursion is not automatically a modulation.

The shared tonal-region model distinguishes:

```text
contrasting center
nested tonicization candidate
sequential modulation candidate
return candidate
```

and requires persistence, structural arrival, phrase partition, structural collection, and independent evidence according to the relation type.

## 4. Sonic 3 experimental rings

The harmonic lane should use the same intervention logic as the attribution program.

### Ring A — retail execution inventory

Run the surface harmonic probe across the 58 Sonic 3 final/beta/prototype VGM/VGZ fixtures.

Freeze JSON before consulting curated track-attribution hypotheses.

Questions:

- Which tracks have high performed-pitch projection coverage?
- Which tracks are dominated by unresolved FM periodicity because of detune/PM/missing-fundamental conditions?
- Which tracks expose stable triadic surface evidence?
- Which tracks are sparse, arpeggiated, pedal-heavy, chromatic, or otherwise hostile to simple vertical chord inference?
- Which root-motion and chord-quality-transition patterns recur?
- Where does the surface mode ranking become ambiguous?

A track that defeats the probe is useful evidence about the missing representation.

### Ring B — prototype → final intervention

Prototype/final pairs are a natural causal experiment.

For each paired musical work compare:

```text
surface pitch-class field
triad-quality distribution
root-motion distribution
harmonic-transition sequence
provisional tonal ranking
FM performed-pitch coverage
```

Then ask which differences are more plausibly:

- compositional rewrite;
- arrangement rewrite;
- added/removed inner voice;
- bass rewrite;
- FM programming difference;
- patch/operator change that alters performed-pitch inference;
- merely realization-level.

The same work identity must remain explicit.

### Ring C — driver confound with Sonic 3D Blast

The Sonic 3D Blast prototype/final driver lineage is a strong implementation control because its driver is closely related to the Sonic & Knuckles environment.

A harmonic feature that follows the driver into unrelated music is suspicious as composer evidence.

A feature such as:

```text
root-motion habit
phrase-level tonal excursion
cadential timing
bass/harmony interaction
harmonic rhythm
```

that survives across different drivers and soundtracks is more composition-facing than a feature tied to one patch bank or control idiom.

This control becomes much stronger once persistent parts and phrase regions are executable on the corpus.

### Ring D — cross-soundtrack creator controls

The existing Genesis control corpus around Maeda, Sawada, Nagao, Setsumaru, Takaoka, Hikichi and other plausible contributors should receive the same blind harmonic extraction.

Do not train creator rules from Sonic 3 alone.

The future creator-facing question is:

> Does a harmonic behavior recur across independent soundtracks associated with one creator after transposition, driver, instrumentation, and soundtrack-local context are varied?

Candidate harmonic grammar dimensions include:

- preferred root-motion distributions;
- modal/tonal field behavior;
- harmonic rhythm;
- predominant/dominant preparation habits where theory-scoped;
- cadence vocabulary;
- deceptive/non-closure habits;
- pedal-bass use;
- bass motion under retained upper harmony;
- tonicization/modulation frequency and destination;
- phrase-end harmonic density;
- harmonic behavior during A→A′ transformations;
- return/retransition design.

No one item is a composer fingerprint by itself.

### Ring E — S&K Collection MIDI ↔ SMPS ↔ VGM

The Sonic & Knuckles Collection material provides an unusually valuable same-work representation triangle:

```text
PC symbolic/MIDI-like representation
        ↕
SMPS / Mega Drive authored sequence evidence
        ↕
VGM execution
```

This is where we can test whether a harmonic relation survives representation change.

Desired checks include:

- same structural pitch collection despite changed voicing;
- same root motion despite instrumentation change;
- preserved versus altered bass line;
- added Mega Drive inner voices;
- changed inversion without changed chord identity;
- changed surface verticality caused by arpeggiation while the authored harmony remains equivalent;
- cadence/form relation surviving realization change.

### Ring F — hidden SMPS teacher

SMPS source is the strongest local teacher for closing the current VGM surface gap.

The benchmark should eventually be:

```text
SMPS source available to evaluator
        ↓
execute to VGM
        ↓
hide SMPS source from interpreter
        ↓
recover performed pitches / persistent parts / harmony / phrase / form
        ↓
compare recovered interpretation to source-derived structure
```

The hidden teacher can adjudicate disagreements such as:

- whether two physical-channel notes belong to one continuing musical part;
- whether a sounding chromatic tone is structural or ornamental;
- whether a pitch change is rearticulation or continuous control;
- whether an arpeggio represents one harmony;
- whether a repeated section is authored repetition or execution reuse;
- whether an apparent cadence aligns with an authored phrase/loop boundary.

This is more useful than teaching the interpreter the answer directly because it measures what the downstream evidence can reconstruct.

## 5. Blind protocol

The harmonic lane belongs to **musical blind mode**.

Allowed during extraction:

- VGM bytes;
- VGM header clocks;
- device execution state;
- corpus routing identity;
- predeclared target/control membership;
- source-relative timing;
- prototype/final relationship only when the experiment explicitly freezes that relation in advance.

Forbidden during blind extraction:

- curated Sonic 3 composer labels;
- external `target_control_people` tags;
- documentary attribution conclusions;
- ROM strings or reconstructed source labels that reveal identity shortcuts;
- filenames used as composer labels;
- known track attribution injected into feature selection.

Freeze the musical output before unblinding.

## 6. Surface harmonic neighbor space

The first executable cross-track signature intentionally avoids absolute tonic pitch.

Current coordinates include normalized:

- triad-quality duration;
- directed root-motion interval;
- chord-quality transition;
- best mode-shape scores independent of tonic class;
- provisional functional-shape ratios.

Thus a transposed version of the same simple harmonic grammar can remain nearby without sharing the same absolute key.

This space is exploratory.

```text
harmonic neighbor
!= same composition
!= same arranger
!= same programmer
!= same composer
```

Its purpose is to nominate comparisons for deeper controlled analysis.

## 7. Required output discipline

Every track-level JSON object should expose both observations and blocked promotions.

A useful output shape is:

```text
performed-pitch coverage
surface pitch-class duration
surface triads
root-motion transitions
provisional tonal candidates
provisional functional shapes
shared-model promotion status
claim boundary
```

The blocked-promotion object is not boilerplate. It is an executable research result.

If a Sonic 3 track repeatedly produces:

```text
key blocked because structural pitch collection is missing
```

then the next engineering task is not "improve key guessing."

It is:

> build the structural pitch/figuration bridge.

Likewise, if key becomes grounded but cadence remains blocked, the missing object is phrase/arrival evidence rather than a more aggressive cadence classifier.

## 8. Validation

Synthetic self-test:

```bash
python tools/sonic3_harmonic_probe.py --self-test
```

Regression:

```bash
python -m unittest tests/vgm/test_sonic3_harmonic_probe.py
```

Primary corpus lane:

```bash
python tools/sonic3_testbed.py vgm-harmonic-probe \
  --json artifacts/sonic3-harmonic-blind.json
```

The synthetic regression constructs actual VGM bytes containing a C→F→G→C progression and verifies that the surface lane:

- recovers the expected major triads;
- ranks C Ionian first;
- sees IV→V and V→I shapes;
- **does not** promote the result to resolved key/function/cadence;
- loses performed-pitch coverage when operator detune invalidates the simple periodicity model.

## 9. Next implementation frontier

The highest-value next bridge is:

```text
real Genesis execution graph
        ↓
physical voice episodes
        ↓
strong persistent-part trajectories
        ↓
part-aware performed-pitch observations
        ↓
structural harmonic segmentation / figuration handling
        ↓
structural pitch-class collection
        ↓
shared tonal-center + key + function + cadence models
```

The repository already contains the ingredients for much of this path:

- Genesis semantic execution adapter;
- physical performance episodes;
- Genesis persistent-part evidence;
- persistent-part trajectories;
- part-aware FM analysis features;
- operator-aware performed pitch;
- harmonic verticality;
- tuning projection;
- triad hypotheses;
- voice leading;
- bass/harmony interaction;
- phrase consensus;
- cadential arrival;
- tonal center;
- structural pitch-class collection adapters;
- diatonic key class;
- chord degree;
- Ionian functional tendency;
- cadence class;
- tonal-region relations.

Sonic 3 should now be used to connect those objects end to end rather than adding another parallel theory stack.

## 10. Success criterion

A successful Sonic 3 harmonic analysis will eventually be able to say something like:

```text
This phrase establishes one tonal area through independent bass, structural-pitch,
and harmonic-arrival evidence. The next phrase preserves the motif but moves into a
locally grounded contrasting area. The return restores the earlier center while
recomposing the inner voice. The apparent V→I surface event at tick X is not treated
as a cadence because it occurs inside the phrase; the later identity-grounded arrival
is the stronger closure. The same preference for this root-motion / bass / cadence
configuration recurs across two independent soundtracks by candidate composer Y and
survives a driver/platform confound.
```

That is composer-level musical understanding.

A list of chord labels is not.
