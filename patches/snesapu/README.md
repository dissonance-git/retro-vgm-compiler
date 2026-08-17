# SNESAPU dependency patches

These guarded patches consolidate the mature causal-source work that previously lived in `dissonance-git/vgmspc` into Retro VGM Compiler's dependency boundary.

They target the editable SPCPlay / SNESAPU source tree. They are intentionally not part of the dependency-free model build.

## Source capture and pre-BRR restoration seam

Apply the SNESAPU/DSP patches in order:

```text
python patches/snesapu/apply_source_capture.py <spcplay-root>
python patches/snesapu/upgrade_source_capture_v2.py <spcplay-root>
python patches/snesapu/apply_prebrr_provider.py <spcplay-root>
```

The first two add exact source/control capture. `apply_prebrr_provider.py` adds the earlier restoration seam used only when a verified original/pre-BRR sample exists.

All scripts use exact singular source replacements. If the pinned upstream layout changes, patching fails instead of guessing a new hot-loop insertion point.

### SRCE v2 contract

The resulting SNESAPU producer exposes one source/control block beside each protected reference PCM block:

```text
0..7    dry voice audio
8..15   effective per-sample left coefficient for voices 0..7
16..23  effective per-sample right coefficient for voices 0..7
24      final shared wet contribution, left
25      final shared wet contribution, right
```

The dry voice tap is after interpolation/noise selection and envelope processing, before voice-local stereo gain. The wet pair is one shared S-DSP echo field and is captured after the current EVOL arithmetic. It must never be cloned into eight fictional per-voice wet stems.

`components/spc/snesapu_source_transport_v2.h` is the dependency-free consumer-side contract for these 26 planes.

The gain planes are control truth, not extra audio objects. The wet planes already contain their EVOL gain trajectory, so downstream evidence must mark that route arithmetic as preapplied.

## Independent Enhanced playback

Apply the foobar preference plus source-reconstruction policy with:

```text
python patches/snesapu/apply_enhanced_component.py \
  <foo_snesapu/foobar2000/foo_snesapu>
```

This composes:

```text
apply_enhanced_ui.py
apply_enhanced_runtime.py
```

`Enhanced` and `Spatial` are independent saved controls. Enhanced defaults off, so the protected existing synthesis path remains the default.

### Normal Enhanced playback is one 48 kHz source-domain path

The pinned SPCPlay/SNESAPU renderer has a useful distinction that is easy to miss from the preferences dialog.

Its interpolation choices include `INT_SINC`, documented by the source as an **8-point sinc** interpolator. More importantly, `SetDSPOpt` normally sets the DSP execution rate to the configured output rate and recalculates source pitch for that rate. Only the separate `DSP_ECHOFIR` compatibility/actual-emulation mode forces the DSP back to 32 kHz and then activates the final sampling-rate converter when the requested output rate is higher.

Normal Enhanced playback is therefore standardized as:

```text
Enhanced = on
final playback rate = 48 kHz
```

and the baseline path selects:

```text
DSP execution / voice reconstruction at 48 kHz
+ 8-point sinc source interpolation
+ the same sequence / BRR / pitch / envelope / routing state
```

rather than:

```text
historical 32 kHz final stereo
-> generic 48 kHz upsampling
```

`apply_enhanced_runtime.py` expresses exactly that bounded intervention. It leaves the user's stored reference output-rate field intact, but while Enhanced is active it forces the live DSP/source path to 48 kHz, selects `INT_SINC`, and explicitly clears `DSP_ECHOFIR` so Enhanced cannot accidentally collapse back to a 32 kHz completed-bus resample.

The fixed 48 kHz rate is intentional. The final product path is 48 kHz, so a routine 96 kHz intermediate would roughly double per-sample reconstruction work and then discard everything above the final Nyquist limit. A 96 kHz run remains useful for offline/research A/B tests where we want to measure whether oversampling materially changes aliasing or nonlinear edge cases. It is not the normal playback contract.

The more important quality gain is **where** reconstruction occurs. Verified upstream sources use the longer 64-tap source-domain sampler at the exact live game trajectory before the downstream S-DSP performance machinery. That is qualitatively different from merely asking the finished stereo bus to carry a larger sample-rate number.

## Preferred Enhanced rung: verified original sample before BRR

The better target is earlier than the normal BRR decoder:

```text
original production / sample-library waveform
        ↓
historically verified preparation
(trim + resample + gain + loop + intentional filtering)
        ↓
16-bit game sample grid
        ↓
BRR encoder
        ↓
SPC RAM
```

When that upstream identity and preparation chain are proven, Retro VGM Compiler prepares the original waveform onto the **exact game sample grid before BRR quantization**, then replaces only the sixteen decoded samples that one 9-byte BRR block would have produced.

The patched SNESAPU path becomes:

```text
SPC execution
        ↓
BRR block requested
        ↓
verified pre-BRR block available?
  no  -> exact historical BRR decoder
  yes -> prepared original 16-sample block
        ↓
SNESAPU interpolation
pitch / PMON
ADSR / GAIN
VxVOL routing
EON / echo / FIR / feedback
MVOL
        ↓
output
```

This is the desired form of "the original samples before the hardware compressed them." It removes BRR loss while leaving the later S-DSP performance machinery authoritative.

The block provider is deliberately low-frequency: the callback runs once per BRR block, not for every output sample.

### Parent/child transport

The historical x64 foobar component runs the 32-bit SNESAPU code in `spcplayer.exe`, so verified replacement data has to cross that process boundary at setup time.

After the DSP patch above, apply:

```text
python patches/snesapu/apply_prebrr_transport.py <foo_snesapu-root>
```

where `<foo_snesapu-root>` contains both:

```text
foobar2000/foo_snesapu/
spcplayer/
```

This patch:

- upgrades the private SPCP startup packet to version 2;
- appends one bounded pre-BRR packet after the SPC and optional Script700 payload;
- parses and copies that data once inside the child;
- installs a fixed realtime BRR-block callback;
- never sends archival paths or corpus metadata to the child.

For a local track named:

```text
music.spc
```

Enhanced optionally looks for:

```text
music.spc.prebrr
```

A missing sidecar is normal. Playback simply uses the 48 kHz/Sinc BRR reconstruction rung. An invalid sidecar is not silently accepted as historical evidence.

The sidecar format is defined by:

```text
components/spc/snesapu_prebrr_packet.h
```

and contains only already-approved prepared game-grid PCM plus the SRCN / first-BRR-block mapping needed for playback.

## Finding and approving original samples

The search pipeline is intentionally split from playback:

```text
BRR sample
↓ decode
robust candidate retrieval
↓
possible original sample(s)
↓
fit explicit historical preparation transform
↓
waveform/loop/trim validation
↓
independent lineage evidence
↓
approved pre-BRR source
↓
prepared game-grid PCM
↓
.prebrr sidecar
```

Current pieces:

```text
tools/spc_original_sample_candidates.py
    distortion-tolerant candidate ranking only

components/spc/spc_sample_lineage_verification.h
    transformed-source vs decoded-game validation

components/spc/spc_original_sample_bank.h
    ambiguity-safe approved-source lookup

components/spc/snesapu_prebrr_provider.h
    exact BRR-block -> prepared original PCM mapping

components/spc/snesapu_prebrr_packet.h
    setup-time parent/child transport
```

The SciSpace literature pass is the reason candidate retrieval and historical admission stay separate. Robust fingerprints can survive filtering, resampling and compression well enough to find plausible ancestors. Generative audio super-resolution can synthesize plausible missing bandwidth. Neither fact establishes that invented or merely similar high-frequency content was present in the original production sample.

So normal Enhanced uses a pre-BRR source only when identity and preparation are evidenced. Generative bandwidth extension remains an optional research/listening experiment, not source truth.

## Deeper reconstruction fallback

When no upstream original can be established, Retro VGM Compiler still has the earlier source-domain reconstruction machinery:

```text
components/spc/spc_enhanced_reconstruction.h
components/spc/spc_enhanced_native_interval.h
components/spc/spc_sample_restoration.h
components/spc/spc_upstream_sample_reconstruction.h
components/spc/snes_spc_enhanced_source_hook_bridge.h
```

That machinery can evaluate decoded-BRR source trajectories at sub-32-kHz phases without pretending an unknown original master has been recovered.

The intended quality ladder is therefore:

```text
best: verified upstream/original waveform at the exact live game phase
 ↓
verified exact pre-BRR prepared game-grid source
 ↓
exact BRR source trajectory reconstructed at 48 kHz
 ↓
SNESAPU 48 kHz 8-point Sinc path
 ↓
protected historical reference
```

Each rung is reversible and evidence-labelled. A 96 kHz render is a research comparison of a rung, not an extra evidence rung by itself.

## Separation of concerns

```text
reference source capture != enhanced source reconstruction
spatial presentation      != synthesis enhancement
```

The foobar shell may independently select synthesis quality and Omniphony presentation:

```text
Enhanced OFF + Spatial OFF -> protected reference stereo
Enhanced OFF + Spatial ON  -> protected synthesis + Omniphony
Enhanced ON  + Spatial OFF -> enhanced source-native stereo
Enhanced ON  + Spatial ON  -> enhanced source-native audio + Omniphony
```
