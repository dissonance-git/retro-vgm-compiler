# Namco PCM voice semantics

## Question

What device-level structure can VGM Compiler recover from Namco C140 and C352 execution before making any claim about persistent musical parts, instruments, notes, or source-driver semantics?

This pass uses the existing real corpus controls:

- `super-world-court-c140`
- `super-world-stadium-95-c352`
- `ponpoko-namco-wsg` as a converted-representation negative control

The goal is not another chip taxonomy. The goal is a time-bearing voice episode whose identity remains tied to exact device evidence.

## Pinned observatories

- ValleyBell/libvgm `61fc6725644886abc3168e240e4e51588d74bdf7`
- mamedev/mame `954cbbe49ff16c1bb9617e93a08db2d05e051e61`
- VGM specification 1.71 fields and commands for C140/C352

These are mechanism observatories. They do not prove that a particular commercial soundtrack used any reconstructed higher-level driver unless independent provenance establishes that link.

## C140 device surface

The C140 exposes 24 PCM voices. A voice register block carries at least:

- left and right gain
- 16-bit frequency coordinate
- bank
- mode
- sample start
- sample end
- sample loop
- key-on state

A key-on begins a new physical voice lifetime and snapshots the relevant sample bounds into runtime voice state. The renderer then advances a fractional sample position from the frequency coordinate, resolves the banked sample address, decodes the PCM representation, applies interpolation, and routes the result through left/right gain.

Therefore the strongest directly earned C140 object is not a note and not an instrument. It is approximately:

```text
C140 physical voice episode
=
voice slot
+ key-on generation
+ bank/address mapping
+ start/end/loop region
+ frequency trajectory
+ decode mode
+ gain trajectory
+ lifetime
```

The sample region may contribute evidence toward an instrument identity later, but:

```text
sample region != instrument
physical voice != persistent musical part
frequency coordinate != note spelling
```

### C140 clock provenance is not optional

libvgm contains explicit compatibility repair for old C140-family VGM headers whose clock field actually stores an old sample-rate-like value. For C140 it recognizes the historical `21390` case and otherwise multiplies sub-1-MHz values by 576. The C219 path has its own historical special case (`44100`) and the same sub-1-MHz repair class.

Consequently an anonymous numeric `clock` is insufficient evidence. A C140 trajectory must retain whether the value is:

- the raw VGM header coordinate;
- a compatibility-normalized device clock;
- or an emulator/core-local timing coordinate.

Do not derive a universal C140 playback-rate formula until that coordinate conversion is explicit in our own parser.

## C352 device surface

The C352 exposes 32 physical voices and four output buses. The mature MAME/libvgm implementations expose a substantially richer voice state than a generic PCM channel:

- front gain pair
- rear gain pair
- 16-bit frequency coordinate
- flags
- waveform bank
- sample start
- sample end
- sample loop
- current sample position
- fractional phase counter
- key-on/key-off/busy state
- loop history/direction state

The flag surface includes behavior for:

- forward looping
- reverse playback
- bidirectional/reverse looping
- linked/long-format addressing
- noise generation instead of PCM
- mu-law decode
- interpolation/filter selection
- phase inversion on output routes
- key-on/key-off and loop-history state

This is enough to require a richer physical episode object:

```text
C352 physical voice episode
=
voice slot
+ key-on generation
+ waveform address identity
+ playback-rate trajectory
+ decode/noise mode
+ direction/loop trajectory
+ four-way gain/routing trajectory
+ phase-routing state
+ lifetime
```

Again, none of these fields alone is a persistent musical part.

## C352 timing coordinates

The VGM 1.71 header contains two separate C352 timing coordinates:

- `0xD6`: clock divider stored divided by four; typical effective divider 288
- `0xDC`: C352 input clock; typical value 24,192,000 Hz

MAME's VGM player reconstructs the divider as:

```text
effective_divider = header_byte * 4
```

and passes that divider separately from the input clock into the C352 device.

MAME's current C352 core advances the 16-bit fractional counter by `freq` once per core sample and fetches a new source sample when that counter overflows. Under the current VGM playback convention, the exact source-sample fetch rate is therefore:

```text
source_sample_fetch_hz
= (input_clock_hz / effective_divider) * (freq / 65536)
```

This formula describes the current VGM/emulator coordinate system. It must not be silently renamed a historical hardware output rate.

The source comments in both MAME and libvgm describe the hardware output rate as `input_clock / (288 * 2)` for the common divider, while libvgm currently runs the emulation core at `clock / 288` and explicitly retains a TODO about outputting at roughly 43 kHz and fixing sample reading/interpolation. Those statements are not necessarily contradictory: at the typical `freq = 0x8000`, a core running at `clock / 288` consumes source samples at exactly `clock / 576`.

That gives us two useful and distinct coordinates:

```text
renderer/core step rate
= input_clock / divider

source sample fetch rate
= renderer/core step rate * freq / 65536
```

At `freq = 0x8000`:

```text
source sample fetch rate
= input_clock / (divider * 2)
```

This distinction should be protected explicitly rather than collapsed into a field named merely `frequency` or `sample_rate`.

## Why this matters musically

Periodic tone generators gave us a route from integer device coordinates to nominal oscillator Hz. C140/C352 show that sampled instruments require a different lower-level invariant:

```text
sample identity
+ source-sample traversal
+ traversal rate
+ loop/direction state
+ gain/routing trajectory
+ lifetime
```

Only above that layer should the interpreter ask whether repeated voice episodes form one persistent musical part, whether a sample region behaves as one instrument, or whether a playback-rate trajectory supports a performed pitch claim.

This prevents several false equivalences:

```text
same sample != same musical part
same voice slot != same musical part
same playback rate != same note without sample tuning evidence
same start address != same instrument without bank/address context
same C352 output != native C352 source provenance
```

## Real-corpus tests

### 1. Super World Court / C140

Recover time-bearing per-voice trajectories containing:

- key-on/key-off boundaries
- bank
- start/end/loop
- frequency
- left/right gain
- mode

Split a physical episode whenever a new key-on replaces the previous voice lifetime, even if the same slot is reused immediately.

Do not infer note spelling or persistent part identity yet.

### 2. Super World Stadium '95 / C352

Recover time-bearing per-voice trajectories containing:

- key-on/key-off/busy transitions
- wave bank/start/end/loop
- frequency
- derived VGM-coordinate source-sample fetch rate
- front/rear gain
- reverse/loop/link/noise/mu-law/filter/phase flags
- loop-direction/history transitions where observable

This should become the primary stress test for sampled-device voice episodes.

### 3. Ponpoko / converted WSG -> C352

Run the exact same C352 trajectory extractor over `ponpoko-namco-wsg`.

The extractor should correctly describe what the retained VGM commands make the C352 representation do while preserving the corpus provenance that the music originated on three-voice Namco WSG hardware and was converted for VGM playback.

A successful C352 trajectory extraction must therefore NOT promote:

```text
retained C352 execution
-> native C352 historical source
```

This is an adversarial provenance control for the entire sampled-device layer.

## Earned common structure

Across C140 and C352, the following common lower-level object is now justified enough to test:

```text
sampled physical voice episode
=
exact device/source identity
+ physical slot
+ generation/lifetime
+ sample-address identity
+ playback-rate trajectory
+ decode mode
+ loop/direction trajectory where supported
+ gain/routing trajectory
```

The common object should remain descriptive, not prescriptive. Device-specific fields and timing conventions stay attached to the source-specific evidence.

Do not move this into a universal model merely because two Namco chips share it. Pressure-test it against SegaPCM, MultiPCM, RF5C164, OKIM6295, QSound, and later PS1 SPU/N64/NDS sampled execution first.

## Next tests

1. Implement the C352 VGM-coordinate playback-rate helper with explicit input clock and divider provenance.
2. Add a C352 register/voice episode capture over `super-world-stadium-95-c352`.
3. Run the same capture over converted `ponpoko-namco-wsg` and verify provenance remains converted-representation evidence.
4. Add C140 episode capture, but fail closed on ambiguous legacy clock coordinates until raw-header versus normalized-clock state is explicit.
5. Pressure-test the sampled-voice episode shape against SegaPCM and MultiPCM before promoting any field into `model/`.

## Stop conditions

Stop rather than guess if:

- a clock coordinate cannot be classified as raw VGM, compatibility-normalized, or core-local;
- a bank/address mapping depends on board behavior not represented by the source;
- sample tuning/root-key evidence is absent;
- a physical voice is being promoted to a persistent part only because the slot stayed constant;
- a converted corpus control is being treated as native hardware provenance.

Correction outranks coherence.
