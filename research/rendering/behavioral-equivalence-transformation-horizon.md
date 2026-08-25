# Behavioral equivalence under transformation horizon

Status: **exact finite cross-project control**

## Question

Can Helix's recent obligation-relative partition-refinement work reveal a useful game-audio distinction that is invisible under current-output comparison but becomes necessary under future playback/control transformations?

The transfer is intentionally narrow. It does not import Helix terminology or architecture wholesale. It tests one established formal idea, finite behavioral equivalence, against one exact game-audio device law.

## Inputs

Helix controls consulted:

- `HELIX-SEMANTIC-CONTROL-REACHABILITY-028`, especially the transformation-horizon refinement at commit `41c180c9adac4fa1ea803634b8431e368a31a771`;
- the obligation-scope/bisimulation-cost control at `fe823008733bdf2d52039f2c9fc433bd7c31c600`;
- `PNP-INFO-SUFF-001` at `f77233e6317c01194fcc0d7fd7d541ce5be6bfbb`, where currently dormant information becomes load-bearing under later insertions;
- `GRAPH-CLOSURE-AMPLIFICATION-003` at `831cf4bab55c1ca31572720a3143fe9dd44abcd4`, which separates direct change, semantic consequence, and maintenance burden.

Device law:

- ares Famicom APU at `b80f67d38312648d197762121c3a27b02c0887db`;
- two NES pulse channels are combined through one nonlinear pulse DAC lookup/formula depending on the sum of the two pulse amplitudes.

This control compares that historical joint realization against a common but incorrect isolation abstraction in which each pulse is passed independently through the same DAC curve and the two resulting stems are summed afterward.

## Finite state machine

State:

```text
architecture ∈ {joint, split}
pulse A amplitude ∈ {0..15}
pulse B amplitude ∈ {0..15}
```

Total states:

```text
2 × 16 × 16 = 512
```

Observation:

```text
joint: P(A + B)
split: P(A) + P(B)
```

where the ares pulse-DAC law is represented exactly with rational arithmetic.

Action alphabet:

```text
A := 0..15
B := 0..15
```

for 32 labeled assignments.

The initial partition groups states with exactly equal current output. Each refinement round additionally requires equivalent successor classes for every labeled assignment. This is ordinary finite Moore-machine / bisimulation-style partition refinement.

## Exact result

`tools/nes_pulse_behavioral_equivalence.py` produces:

```text
horizon   equivalence classes   partition entropy   class-size range
H0              151              6.674785 bits          1..18
H1              511              8.996094 bits          1..2
H2              512              9.000000 bits          1..1
H3              512              9.000000 bits          1..1
```

The stable quotient is therefore the full 512-state system under this action/observation contract.

For the 256 pairs with identical `(A,B)` coordinates but different architecture:

```text
225 pairs differ at H0
 30 pairs first differ at H1
  1 pair first differs at H2
```

The unique pair still merged at H1 is:

```text
joint, A=0, B=0
split, A=0, B=0
```

Why?

From silence, any single labeled amplitude assignment can activate at most one pulse. With only one pulse active:

```text
P(A + 0) = P(A) + P(0)
P(0 + B) = P(0) + P(B)
```

so the historical joint mixer and the split-stem abstraction remain behaviorally indistinguishable through every one-step continuation.

Two assignments can activate both pulses. The witness

```text
A := 1
B := 1
```

produces:

```text
joint  P(2)       ≈ 754.51389049
split  P(1)+P(1) ≈ 763.68396694
```

and the hidden architecture becomes observable.

## Discovery

The important result is not merely that the NES mixer is nonlinear; that was already established.

The new result is:

```text
current acoustic equality
!=
behavioral equivalence under future musical/control transformations
```

and, more specifically:

```text
an incorrect internal decomposition can be exactly indistinguishable
for the present output and every one-step continuation,
yet become distinguishable only at a longer transformation horizon
```

This gives VGM Compiler a precise criterion for deciding whether an internal distinction may be compressed in a derived view:

> Two execution states may be merged only relative to a declared observation obligation and admitted transformation family.

That criterion is stronger than waveform equality, endpoint equality, or current-note equality.

## Consequence for stems

A stem system needs at least two different contracts:

### Diagnostic/isolation stem

Useful for listening to one source in isolation.

It may deliberately break exact recombination and may use a counterfactual per-source realization.

### Reconstruction-preserving contribution

Must preserve enough joint device state that admitted future continuations still reproduce the historical mixed behavior.

For the NES pulse pair, independently DAC-processed stems cannot satisfy this second contract under arbitrary future amplitude assignments.

So:

```text
isolatable contribution
!=
composable historical contribution
```

This complements the previously established failures from shared feedback, cross-resource modulation, shared noise, and nonlinear output mixing.

## Consequence for enhancement

The same formalism gives source-native enhancement a sharper guardrail.

An enhancement may alter an implementation coordinate while preserving a chosen obligation. The required retained identity depends on what transformations must remain legal afterward.

Examples:

```text
render-only obligation
may quotient more internal detail

seek/loop obligation
must retain dormant loop/history state

interactive remix obligation
must retain coupling state exposed by future simultaneous activity

reconstruction-preserving enhancement
must retain every distinction that some admitted continuation can expose
```

This turns "preserve identity" from a global slogan into an explicit behavioral contract.

## Relation to Helix closure results

`GRAPH-CLOSURE-AMPLIFICATION-003` also transfers usefully, but no new audio theorem is claimed from it.

A local device-state change can have a large semantic consequence through a connected feedback or routing graph while exact recomputation can remain confined to the dependency-connected region. This suggests dependency-local invalidation for future analysis/render caches, but it should be tested separately against real effect/runtime graphs before implementation.

## Literature boundary

A literature pass found established work connecting behavioral equivalence, bisimulation, automata minimization, and minimal realization, including systems with distinct internal and external time scales. That supports the formal method itself.

No literature result was found in this pass applying this exact horizon-refinement control to NES pulse mixing or game-music stem identity. Therefore this document claims only a new **project result**, not a new general theorem or first-in-literature result.

## Claim boundary

This is an exact finite control for:

- two idealized architectures;
- the ares NES pulse-DAC equation;
- amplitudes `0..15`;
- arbitrary labeled assignments to either pulse amplitude;
- exact output equality as the observation law.

It does not establish that all audio identity partitions stabilize at horizon two, that every emulator implements the analog NES output identically, or that behavioral equivalence should replace device-native provenance.

The canonical source/device state remains primary. Any quotient is a rebuildable derived view tied to its obligation and transition contract.

Correction outranks coherence.
