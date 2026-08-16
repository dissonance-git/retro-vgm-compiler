# Signed routing phase and linear separability

Status: **exact finite cross-project control**

## Question

Helix `PNP-GEO-012` found a bounded residual where unsigned support topology was identical and the missing obligation-relevant information lived in orientation/phase assignments. Does an analogous distinction exist in game-audio routing where support alone is insufficient even before psychoacoustics?

Yes, in the smallest nontrivial stereo routing graph.

## Device motivation

Game Music Interpreter already has exact device evidence that routing state is not merely a scalar pan coordinate:

- SNES S-DSP voice routes use signed left/right gains;
- Namco C352 exposes four gain routes together with phase-inversion flags;
- QSound uses a richer pan/filter/delay realization and is not reduced here.

This control therefore asks only about a two-source/two-output linear signed-routing subproblem.

## 2 x 2 signed routing

Let two source contributions `A,B` route to `L,R` through a complete support:

```text
          L              R
A   s_AL * g_AL    s_AR * g_AR
B   s_BL * g_BL    s_BR * g_BR
```

with:

```text
s_ij ∈ {-1,+1}
g_ij > 0
```

The unsigned support is identical for every signing: all four routes exist.

The route matrix is

```text
M = [[s_AL g_AL, s_AR g_AR],
     [s_BL g_BL, s_BR g_BR]]
```

and

```text
det(M)
= (s_AL s_BR) g_AL g_BR
- (s_AR s_BL) g_AR g_BL.
```

Define the sign product around the unique `K2,2` cycle:

```text
chi = s_AL s_AR s_BL s_BR.
```

## Exact equal-magnitude enumeration

`tools/signed_routing_phase_control.py` enumerates all `2^4 = 16` edge-sign assignments with equal route magnitudes.

Result:

```text
chi = +1   8 signings   rank 1
chi = -1   8 signings   rank 2
```

So with equal magnitudes, the single cycle-sign bit exactly decides whether the two routing rows/columns collapse onto one linear stereo axis or span the full two-dimensional output space.

For example:

```text
balanced
[[+1,+1],
 [+1,+1]]

det = 0
rank = 1
```

versus

```text
unbalanced
[[+1,+1],
 [+1,-1]]

det = -2
rank = 2
```

Both have exactly the same unsigned route support and absolute gains.

## Positive-magnitude pressure test

The verifier also exhaustively tests every edge magnitude in:

```text
{1,2,3,4}
```

across all 16 signings:

```text
4^4 × 16 = 4,096 cases
```

Result:

```text
unbalanced cycle, chi=-1
singular cases: 0

balanced cycle, chi=+1
singular cases:   256
invertible cases: 1,792
```

This matches the elementary determinant law.

When `chi=-1`, the two determinant terms have opposite signs before subtraction, so their positive magnitudes add. Therefore a fully connected 2x2 route with strictly positive gains and negative cycle product cannot be singular.

When `chi=+1`, the determinant is a difference of positive cross-products and may either vanish or remain nonzero depending on gain magnitudes.

Thus:

```text
negative cycle sign
→ guaranteed linear invertibility
```

for this 2x2 positive-gain setting.

## Polarity-gauge quotient

Independent whole-source and whole-output polarity flips act on the route signs as:

```text
s_ij -> r_i c_j s_ij
```

with `r_i,c_j ∈ {-1,+1}`.

The 16 sign matrices collapse under these gauge operations into exactly:

```text
2 orbits
8 states each
```

and the cycle product `chi` is constant on each orbit and distinguishes the two orbits completely.

So the phase information is not four unrelated sign bits. Relative to source/output polarity gauge, the complete 2x2 support contains one irreducible cycle-sign coordinate.

This is standard signed-graph / signed-matrix structure, not a new theorem.

## Audio consequence

The new Game Music Interpreter result is the device interpretation:

```text
routing support
+ absolute route gains
!=
linear source/output geometry
```

because a phase assignment can change matrix rank while leaving both support and absolute gains untouched.

Therefore a route representation that stores only:

```text
source -> output incidence
+ unsigned gain
```

can destroy a distinction relevant to exact reconstruction and source separation.

This sharpens the previous rule:

```text
native routing != pan
```

into:

```text
unsigned native routing != signed native routing
```

and shows why the difference can be structural rather than merely a polarity cosmetic.

## Separation consequence

For a purely linear instantaneous 2x2 subsystem:

```text
rank 2
```

means the two output mixtures contain enough independent linear information to invert the route matrix in principle.

```text
rank 1
```

means they do not.

This is only a lower-level observability statement. It does **not** imply perceptually clean stems from a real soundtrack, because real devices also contain:

- nonlinear mixing;
- shared feedback state;
- modulation;
- time-varying gains;
- quantization/clipping;
- shared source generators;
- downstream filters/effects;
- noise and incomplete observation.

Still, the signed route matrix can tell us whether the routing stage itself has already collapsed one dimension before those later complications.

## Relation to Helix GEO-012

The transfer is structural, not a cross-domain theorem.

Helix found:

```text
same support topology
+ different orientation/phase assignment
→ different target behavior
```

The audio control finds:

```text
same source/output support
+ same absolute gains
+ different route-sign assignment
→ different matrix rank / invertibility
```

In both cases support is not the complete state and a relative phase/orientation coordinate survives quotienting.

The mathematical machinery differs, and no identity between the two problems is claimed.

## Literature boundary

A literature pass found established signed-bigraph and signed-graph work connecting edge signs, cycle structure, rank, and invertibility. That makes the underlying mathematics prior art.

No claim is made that the cycle-product/rank identity is new mathematics. The project contribution is recognizing it as a useful derived invariant for time-bearing game-audio route evidence.

## Next corpus test

Do not add a new common-model primitive yet.

Instead, extend source-specific trajectory analysis to search real signed-routing states for simultaneous two-source/two-output submatrices and record:

```text
route support
signed gains / phase flags
cycle sign
absolute determinant
condition number where meaningful
active interval
source/device provenance
```

Primary targets:

1. SNES S-DSP signed `VxVOLL/VxVOLR` trajectories;
2. C352 front/rear route pairs and phase flags;
3. negative control on devices exposing only unsigned/hard route bits.

Questions:

- Do commercial tracks actually exercise unbalanced route cycles?
- Are they transient or stable?
- Do they correlate with widening, phase effects, or intentional cancellation?
- Can the invariant improve exact lower-level separation without pretending it is a musical-part label?

## Claim boundary

This control establishes only the stated 2x2 signed linear-routing facts and their relevance to device evidence.

It does not establish perceptual separation, composer intent, a generic n-channel rank theorem, or that every hardware polarity inversion should be quotiented by source/output gauge for every obligation.

Canonical device signs and gains remain exact evidence. The cycle sign is a rebuildable derived coordinate.

Correction outranks coherence.
