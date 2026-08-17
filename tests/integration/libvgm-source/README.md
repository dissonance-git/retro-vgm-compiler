# libvgm source-capture integration tests

These tests exercise Retro VGM Compiler mechanisms against the exact external libvgm semantics they mirror. They are intentionally separate from the dependency-free root core suite.

The first regression was mined from `dissonance-git/vgmspc` at source commit `f3368941213c841fdb59f601196401aef30c257a`. It preserves a subtle but important contract of libvgm's default `RSMODE_LINEAR` implementation: linear upsampling pre-generates one native sample during `Resmpl_Init()`, and a subsequent chip reset can restart the device while that resampler history remains. It also checks segmented execution, equal-rate copy behavior, downsampling, and libvgm volume scaling.

Configure against the same libvgm checkout that will receive `patches/libvgm/apply_source_capture.py`:

```text
cmake -S tests/integration/libvgm-source -B build/libvgm-source-tests -DLIBVGM_ROOT=/path/to/libvgm
cmake --build build/libvgm-source-tests
ctest --test-dir build/libvgm-source-tests --output-on-failure
```

A canonical component workflow should run this test after the libvgm source-capture patch has been applied and before the foobar VGM component is built. The test does not own or vendor libvgm.
