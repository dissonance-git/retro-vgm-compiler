# SNESAPU dependency patches

These guarded patches consolidate the mature causal-source work that previously lived in `dissonance-git/vgmspc` into VGM Compiler's dependency boundary.

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

When that upstream identity and preparation chain are proven, VGM Compiler can use the highest-supported source representation while leaving the live game trajectory and downstream S-DSP performance machinery authoritative.

There are two verified-source rungs:

```text
exact upstream/original waveform
    -> 64-tap source-domain reconstruction at the live game phase

exact prepared pre-BRR game-grid PCM
    -> replace the sixteen decoded samples of the matching BRR block
```

The first preserves more upstream information when the original production waveform and exact preparation map are proven. The second is the safer lower rung when only the exact post-preparation/pre-BRR game-grid source is known.

### Pre-BRR block path

The patched lower source path becomes:

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

This removes BRR loss while leaving the later S-DSP performance machinery authoritative. The block provider is deliberately low-frequency: the callback runs once per BRR block, not for every output sample.

### Pre-BRR parent/child transport

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

For a local track named `music.spc`, Enhanced optionally looks for:

```text
music.spc.prebrr
```

A missing sidecar is normal. Playback simply uses the 48 kHz/Sinc BRR reconstruction rung. An invalid sidecar is not silently accepted as historical evidence.

The sidecar format is defined by `components/spc/snesapu_prebrr_packet.h`, and contains only already-approved prepared game-grid PCM plus the SRCN / first-BRR-block mapping needed for playback.

## Highest rung: verified upstream waveform at the live phase

The studio-source seam replaces only the historical `pInter` waveform value when one exact upstream source is admitted. Everything downstream stays SNESAPU truth: NON/noise, envelope, PMON feedback, voice routing, echo/FIR/feedback and master volume.

Apply the SNESAPU-side hot-loop seam after the pre-BRR provider:

```text
python patches/snesapu/apply_studio_source_provider.py <spcplay-root>
```

Then, after the pre-BRR parent/child transport has been applied to the foobar tree, upgrade that transport to SPCP v3:

```text
python patches/snesapu/apply_studio_source_transport.py <foo_snesapu-root>
```

The roots differ intentionally: the first patch targets the editable SPCPlay/SNESAPU DLL source, while the second targets the x64 foobar parent plus 32-bit `spcplayer` child.

SPCP v3 appends one optional `.studiosrc` packet after the lower-rung pre-BRR packet. The child:

- validates the packet once before audio starts;
- byte-compares its exact compressed BRR witness with the actual 64 KiB SPC RAM image;
- reconstructs only already-approved upstream PCM and coordinate-map objects;
- prepares the 64-tap / 16,384-phase FIR table before playback;
- installs child-local callbacks, never per-sample IPC.

The mixer callback also carries the live effective SRCN, live loop BRR pointer and DIR page on every restored sample. This matters because pinned SNESAPU can refresh DSP SRCN, reapply Script700 NoteChange, and re-read the loop pointer at END+LOOP without changing the DIR page. Any such live remap invalidates the top rung immediately and falls back to historical interpolation for the remainder of that key-on.

NON/noise voices bypass the studio FIR because the pinned mixer discards the interpolated waveform and substitutes noise immediately afterward. Ordinary long source interiors use the same 64-tap reconstruction law through a contiguous dot-product fast path; startup, END and loop-seam neighborhoods retain explicit virtual topology handling.

For `music.spc`, Enhanced optionally looks for:

```text
music.spc.studiosrc
```

### Authoring `.studiosrc` without hand-entered playback geometry

Use:

```text
python tools/spc_studio_source_sidecar.py \
  music.spc.studiosrc.json \
  music.spc.studiosrc
```

This tool is deliberately a **freezer, not a discoverer**. It does not search sample libraries, resample audio, fit gains, infer loop points, or promote provenance. Its manifest must already contain the exact top-rung admission state, the approved source coordinate map, content hashes and nonzero source identities.

The upstream PCM input is raw little-endian IEEE-754 float32. It is copied byte-for-byte into the packet after finite-value validation, so the final evidence boundary performs no hidden decode, normalization, resampling or bit-depth conversion.

Crucially, these fields are **not accepted from hand-authored manifest geometry**:

```text
first BRR address
first..END BRR extent
terminal END/LOOP state
loop BRR block ordinal
compressed BRR witness
```

They are derived from the actual SPC snapshot using the selected/current DIR page and SRCN, including 16-bit RAM wrap. A live loop pointer that is not exactly one of the witnessed BRR block starts rejects authoring. The manifest's source-coordinate map must then close exactly over that snapshot-derived game extent and loop.

The manifest also pins SHA-256 for both the snapshot BRR witness and raw upstream float PCM. These hashes are authoring assertions. Runtime content binding remains the exact BRR witness comparison in the child; `.studiosrc` is a trusted evidence packet, not a hostile-input cryptographic signature format.

A missing `.studiosrc` file is normal. The ladder falls through to exact pre-BRR data when available, then topology-aware BRR reconstruction, then SNESAPU's 48 kHz 8-point Sinc path, then the protected historical reference.

## Finding and approving original samples

Discovery remains separate from playback and from sidecar freezing:

```text
BRR sample
↓ decode
robust candidate retrieval
↓
possible original sample(s)
↓
fit explicit historical preparation transform
↓
waveform / trim / gain / resample / loop validation
↓
independent lineage evidence
↓
what exact representation is proven?
├─ exact upstream/original + exact coordinate map
│    ↓
│  approved top-rung source
│    ↓
│  tools/spc_studio_source_sidecar.py
│    ↓
│  .studiosrc
│
└─ exact prepared game-grid/pre-BRR PCM only
     ↓
   approved pre-BRR source
     ↓
   tools/spc_prebrr_sidecar.py
     ↓
   .prebrr
```

Current pieces:

```text
tools/spc_original_sample_candidates.py
    distortion-tolerant candidate ranking only

components/spc/spc_sample_lineage_verification.h
    transformed-source vs decoded-game validation

components/spc/spc_original_sample_bank.h
    ambiguity-safe approved-source lookup

components/spc/snesapu_brr_playback_topology.h
    snapshot-derived DIR / BRR END / loop topology and witness

components/spc/snesapu_studio_source_packet_builder.h
    C++ packet builder from already-approved restoration candidates

tools/spc_studio_source_sidecar.py
    strict snapshot-bound .studiosrc freezer

components/spc/snesapu_prebrr_provider.h
    exact BRR-block -> prepared original PCM mapping

components/spc/snesapu_prebrr_packet.h
    setup-time lower-rung parent/child transport
```

The SciSpace literature pass is the reason candidate retrieval and historical admission stay separate. Robust fingerprints can survive filtering, resampling and compression well enough to find plausible ancestors. Generative audio super-resolution can synthesize plausible missing bandwidth. Neither fact establishes that invented or merely similar high-frequency content was present in the original production sample.

So normal Enhanced uses an upstream/pre-BRR source only when identity and preparation are evidenced. Generative bandwidth extension remains an optional research/listening experiment, not source truth.

## Deeper reconstruction fallback

When no upstream original can be established, VGM Compiler still has the earlier source-domain reconstruction machinery:

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
