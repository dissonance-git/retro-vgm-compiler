# deepSTRF as an auditory teacher for Game Music Interpreter

## Boundary

Pinned research fork:

```text
dissonance-git/deepSTRF
develop @ a047af472b235f8a544f3cccb37a3cefa886713d
```

This fork already absorbed the durable libaural research line. It should therefore replace `libaural` as the active artificial-hearing research reference.

The repositories should not collapse into one product:

```text
Game Music Interpreter
source truth + driver/device execution + isolated musical sources
+ online musical interpretation
+ source-authoritative causal fixtures

        ↓ fixtures / obligations / compressed findings

deepSTRF
rich auditory teachers + neural datasets + recurrent challengers
+ psychoacoustic mechanisms + adversarial hearing tests
+ mechanism compression research

        ↓ only mechanisms that survive independent obligations

Game Music Interpreter realtime path / Omniphony-facing state
small causal mechanisms suitable for continuous playback
```

Do not embed the research environment merely because it contains useful models. The deepSTRF fork itself treats the final runtime ear as potentially much smaller than the research repository.

## What transfers immediately

The strongest current contribution is not a neural network checkpoint. It is a set of experimentally sharpened state laws.

### 1. Keep identity claim layers separate

```text
source / execution identity
!= acoustically observable continuity
!= perceptual auditory-object identity
!= persistent musical part
```

Game Music Interpreter often has source identity unavailable to ordinary waveform hearing. Use that source truth when present, but do not train the auditory layer to pretend it could have inferred an acoustically invisible source swap.

The runtime contract in `model/realtime_auditory_state_evidence.h` therefore keeps source-semantic support separate from phase, level-trajectory, dropout, spectral and onset continuity evidence.

### 2. Preserve ambiguity at re-entry

Current deepSTRF continuation experiments show that multi-object re-identification behaves like packing in an identity space. When plausible self-change overlaps between-source variation, a forced top-1 answer is not justified.

The realtime model therefore admits a bounded `realtime_identity_hypothesis_set` with no automatic `best()` operation. Multiple plausible auditory-object identities may coexist until stronger evidence separates them.

### 3. Continuity, precision and plasticity use different evidence budgets

A source can remain obviously continuous while its precise pitch estimate degrades. Likewise a stale object model may still identify a continuing source while becoming unsafe to update under masking.

Runtime confidence is therefore split into:

```text
sensory
object
continuity
precision
reidentification
plasticity
```

Do not replace this with one generic confidence scalar.

### 4. Durable memory freezes under weak evidence

The deepSTRF/libaural line found that ungated adaptation can rewrite an old object toward an intruder or toward noise during weak evidence. Re-entry after evidence improves is a better time to reconsider the durable model.

`may_update_durable_auditory_memory()` encodes only the structural rule, not the eventual learned thresholds: durable updates require both continuity ownership and plasticity confidence.

### 5. Fast continuity specialists are promising runtime candidates

The current research bank distinguishes several inspectable continuity coordinates:

```text
phase trajectory
level trajectory
evidence dropout
spectral relation
onset / attack relation
```

The important finding is complementarity, not one magic scalar. A violation of one coordinate means that continuity evidence changed; it does not by itself prove that the hidden source changed.

These specialists are attractive for the Game Music Interpreter/Omniphony path because they can potentially be compressed into small causal state while the heavier deepSTRF models remain external teachers.

## Teacher use

Game Music Interpreter should continue supplying exact counterfactual fixtures to deepSTRF:

```text
known pitch change
known timbre change
known source/channel migration
known masking addition
known routing change
known effect-field change
known continuity-preserving transformation
known hidden source swap when acoustically distinguishable
known hidden source swap when acoustically indistinguishable
```

The hearing system is then scored on what it preserves, not on whether it reproduces Game Music Interpreter's internal machine state.

Useful obligations include:

```text
pitch relation survives synthesis-family change
timbre changes without erasing pitch identity
time/order survives representation
persistent object survives ordinary physical-channel migration
continuity uncertainty rises under masking/dropout
ambiguous re-entry stays ambiguous
weak evidence freezes durable memory
```

## Runtime extraction rule

A mechanism discovered in deepSTRF enters the realtime path only after it passes:

```text
named auditory obligation
→ adversarial counterexample
→ causal/chunked execution test
→ state-size and latency measurement
→ independent source-authoritative fixture
→ compression / ablation
→ same obligation still passes
```

PyTorch, StateNet, torch_amt, ICNet or another rich teacher are not runtime requirements merely because they help discover the mechanism.

## Relation to Omniphony

This research helps Omniphony indirectly by making Game Music Interpreter's semantic sidecar less naive.

The useful progression is:

```text
isolated source audio + source truth
→ auditory continuity / object evidence
→ musical-role and scene memory
→ conservative renderer intent
→ Omniphony
```

Examples:

- a physically reused channel should not yank an unrelated new source into the old role;
- an acoustically continuous part may retain spatial identity across timbral change;
- a masked part may keep its last stable identity while durable updates freeze;
- an ambiguous re-entry should not snap to a confident old object or a new object without evidence;
- a shared effect return can remain environmental glue while source-local roles continue separately.

The aim is still musical: better continuity, separation, depth and scene stability because the system understands what is persisting, changing, disappearing and returning.
