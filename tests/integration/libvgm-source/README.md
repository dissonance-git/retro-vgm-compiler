# libvgm source-capture integration tests

This directory tests VGM Compiler behavior that depends on exact external libvgm semantics. It is intentionally separate from the dependency-free core suite.

## Current contracts

### Linear source-resampler continuity

The resampler test protects libvgm's current linear-resampling lifecycle, including pre-generated history, reset behavior, segmented execution, equal-rate copy, downsampling, and gain-domain behavior.

Its purpose is compatibility with the exact reference source path, not a general resampling quality claim.

### Nuked-OPM channel-additivity falsifier

This test attacks a forbidden shortcut:

```text
whole-chip Nuked-OPM output
?=
sum(eight synchronized single-channel-muted replicas)
```

The executable must find a counterexample to exact additivity for its deterministic fixture. That negative result prevents eight independently muted whole-chip renders from being mislabeled as eight exact enhanced source lanes.

It does **not** reject Nuked-OPM as a whole-chip fidelity candidate. Per-channel source admission would still require a lawful exact decomposition.

### YM2151 renderer-pair diagnostic

The renderer-pair probe compares the protected MAME reference and Nuked-OPM as two whole-chip realizations under the same production register-write contract.

It checks deterministic reset/replay and non-silence, then reports bounded waveform/timing diagnostics such as RMS, gain fit, normalized error, correlation, short-lag alignment, and first-audible frame.

These values are diagnostics only:

```text
numerical similarity
!= perceptual quality
!= hardware fidelity
!= enhanced-source admission
```

A later fidelity decision may add spectral/perceptual measures and external hardware evidence without changing this test's current role.

## Running the suite

Use the exact libvgm checkout that receives the repository's guarded source-capture patches:

```text
cmake -S tests/integration/libvgm-source \
  -B build/libvgm-source-tests \
  -DLIBVGM_ROOT=/path/to/libvgm

cmake --build build/libvgm-source-tests
ctest --test-dir build/libvgm-source-tests --output-on-failure
```

The component materialization/CI route should run this suite after applying the libvgm source-capture patch chain and before building the private foobar VGM component.

These tests do not own or vendor libvgm. Patch procedure belongs in [`../../../patches/libvgm/README.md`](../../../patches/libvgm/README.md).
