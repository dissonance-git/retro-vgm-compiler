# libvgm source-capture integration tests

These tests exercise Retro VGM Compiler mechanisms against the exact external libvgm semantics they mirror. They are intentionally separate from the dependency-free root core suite.

## Linear source-resampler continuity

The first regression was mined from `dissonance-git/vgmspc` at source commit `f3368941213c841fdb59f601196401aef30c257a`. It preserves a subtle but important contract of libvgm's default `RSMODE_LINEAR` implementation: linear upsampling pre-generates one native sample during `Resmpl_Init()`, and a subsequent chip reset can restart the device while that resampler history remains. It also checks segmented execution, equal-rate copy behavior, downsampling, and libvgm volume scaling.

## Nuked-OPM channel-additivity falsifier

`nukedopm_channel_additivity_falsifier.cpp` tests a tempting but currently forbidden route for YM2151 enhancement: render eight synchronized Nuked-OPM replicas with exactly one channel unmuted in each replica, then add those eight stereo outputs and treat them as independent enhanced source lanes.

The test drives one normal eight-channel Nuked-OPM instance and eight single-channel replicas with the same deliberately hot algorithm-7 patch. Every emulator receives the same register writes and advances the same hidden FM state. The only difference is the core's channel mute gate before its shared mixer.

The scientific hypothesis under attack is exact additivity:

```text
full_nuked_output == sum(single_channel_nuked_outputs)
```

The executable succeeds only when it finds at least one sample where that equality fails. A mismatch is evidence that the downstream shared mixer/DAC prevents the solo-render construction from serving as an exact eight-source decomposition. If the witness unexpectedly sums exactly, the executable fails and the witness must be strengthened; exact equality on one fixture still would not prove universal separability.

This is a negative/admission-boundary test. It does **not** claim that Nuked-OPM is unsuitable as a whole-chip high-fidelity candidate. It prevents a whole-chip candidate from being mislabeled as eight independent enhanced sources without an exact decomposition proof.

## Running the integration suite

Configure against the same libvgm checkout that will receive `patches/libvgm/apply_source_capture.py`:

```text
cmake -S tests/integration/libvgm-source -B build/libvgm-source-tests -DLIBVGM_ROOT=/path/to/libvgm
cmake --build build/libvgm-source-tests
ctest --test-dir build/libvgm-source-tests --output-on-failure
```

A canonical component workflow should run this suite after the libvgm source-capture patch has been applied and before the foobar VGM component is built. The tests do not own or vendor libvgm.
