# Mutable sample-memory semantics

## Question

When a sampled voice reads from mutable memory, is `sample address + playback rate` sufficient to identify the source that is actually realized through time?

No.

Across materially different game-audio systems, a running voice can observe later writes to its source memory, but the moment at which those writes become audible depends on the device's fetch, prefetch, block-decode, and local-cache behavior.

The stronger common relation is:

```text
source descriptor/address
+ source-memory mutation history
+ device memory-observation schedule
+ traversal state
+ decode/interpolation state
-> realized sample trajectory
```

This is a cross-platform execution property, not a format tag or musical interpretation.

## Evidence roles

Pinned implementation observatories used in this pass:

- `ValleyBell/libvgm` `61fc6725644886abc3168e240e4e51588d74bdf7` — RF5C68
- `stenzek/duckstation` `5fd366809053fe287291de7a39752c4d5d5b146b` — PlayStation SPU
- `melonDS-emu/melonDS` `d3cd6164deb1f217d4b262d18af3ef9b97e536c8` — Nintendo DS SPU

The scholarly/literature pass found general sampling/wavetable work that explicitly treats waveform memory and buffering/cache layers as separate parts of real-time synthesis. That literature supports the generic buffering concept. The exact game-hardware observation laws below come from the device implementations and should not be replaced by generic sampler theory.

## RF5C68: direct live-RAM observation

The RF5C68 core stores sample data in a mutable 64 KiB RAM array.

During rendering, each active channel directly reads:

```text
chip->data[(channel.address >> 11) & 0xffff]
```

for the next sample byte. The same device interface exposes direct memory writes and block RAM writes.

Therefore, for an unread address:

```text
RAM write before sample fetch
-> next fetch can observe new byte
```

without requiring a new channel start, new sample address, new loop address, or new playback-rate register.

The voice's programmed source coordinates can remain unchanged while its realized waveform changes underneath them.

This proves:

```text
sample address != immutable sample identity
```

on RAM-backed devices.

### End-marker consequence

RF5C68 also uses sample byte `0xFF` as an end/loop marker.

A memory write can therefore change not only waveform amplitude but the future control path of the voice:

```text
ordinary sample byte <-> 0xFF end marker
```

So mutable source memory can alter:

- waveform data;
- termination timing;
- loop behavior.

The source-memory mutation timeline is causally upstream of voice lifetime itself.

## PlayStation SPU: block observation and decoded-sample cache

The PlayStation SPU presents mutable sound RAM, but a voice does not read one source byte per output sample.

DuckStation's voice path performs:

```text
current ADPCM block address
-> copy 16-byte ADPCM block from SPU RAM
-> decode block into 28 PCM samples
-> interpolate/play from decoded local block
```

The renderer reads the complete ADPCM block from the live SPU RAM only when `voice.has_samples` is false. After decoding, output is produced from the voice-local decoded sample buffer until the next block boundary.

The SPU independently supports CPU/manual/DMA writes into the same sound RAM.

Therefore a mutation's visibility is boundary-dependent:

```text
write before block fetch
-> can affect decoded block

write after block fetch but before all 28 decoded samples are consumed
-> does not retroactively alter the already-decoded local block

write before a later block fetch or later loop refetch
-> can affect that future realization
```

This introduces a precise intermediate object:

```text
fetched encoded block generation
-> decoded local block generation
```

rather than a timeless `sample in RAM` identity.

### Decode state also matters

Because PlayStation ADPCM decoding carries predictor history between samples/blocks, changing one encoded block can influence more than its literal nibble values.

The realized trajectory depends on:

```text
encoded bytes
+ decoder history
+ block flags
+ interpolation state
```

A memory hash alone is not a performed voice.

## Nintendo DS: FIFO/read-ahead observation

The Nintendo DS sound channels add another distinct memory-observation law.

For PCM8/PCM16/ADPCM channels, melonDS models a channel FIFO fed from ARM7-visible source memory.

The fetch path reads memory in bursts using `ARM7Read32`. At channel start, the FIFO is filled twice. During playback, samples are consumed from the FIFO, and more source data is fetched when the FIFO level falls to its refill threshold.

Therefore:

```text
source memory
-> burst fetch
-> channel FIFO
-> sample decoder/playback
```

A CPU write to source memory is not necessarily visible to the next audible sample if the corresponding bytes are already buffered.

This creates a read-ahead window:

```text
write to not-yet-fetched source region
-> future FIFO refill can observe mutation

write to bytes already present in FIFO
-> current buffered copies remain unchanged
```

The observation schedule depends on:

- source address;
- FIFO read offset;
- FIFO level;
- burst/refill timing;
- loop/repeat behavior;
- sample format.

### ADPCM makes the buffered boundary richer

For DS ADPCM, the FIFO also supplies decoder header/state and nibble data. Looping restores saved ADPCM decoder state as well as moving the data position.

Thus:

```text
same mutable bytes
+ different prior fetch/decode state
```

can produce different future trajectories.

## Cross-platform correction

The previous sampled-voice model included:

```text
source descriptor identity
+ source-memory generation where mutable
+ traversal state
```

That is necessary but still incomplete.

A stronger common lower-level model is now:

```text
sampled physical voice episode
=
exact device/source identity
+ physical slot
+ activation/lifetime generation
+ source descriptor identity
+ source-memory object
+ source-memory mutation timeline where mutable
+ device memory-observation schedule
+ fetched/prefetched source generation
+ traversal state
+ playback-rate evidence
+ decode/interpolation state
+ end/loop/direction semantics
+ gain/routing/send trajectory
```

Not every device needs every field.

ROM-backed devices can collapse the memory-mutation dimension when the source is genuinely immutable for the execution under study.

## Important distinction: memory version vs observed version

A global memory generation counter is not enough.

Suppose memory is edited at time `t` while a device has already prefetched the relevant source bytes.

Then:

```text
current global memory generation = new
voice's currently observed source generation = old
```

until the device crosses the relevant fetch/refill/decode boundary.

The evidence model therefore needs to distinguish:

```text
memory mutation time
```

from:

```text
voice observation time
```

This is the same general discipline already encountered in register/device state:

```text
programmed cause != instantaneous performed state
```

but applied to sampled source data itself.

## Memory observation can affect voice identity boundaries

Mutable memory can change more than timbre.

Depending on the device, a source mutation may alter:

- sample amplitude;
- encoded decoder trajectory;
- loop/end marker;
- loop target content;
- future voice termination;
- future repeated-cycle waveform.

Therefore a `sample identity` cannot always be modeled as a static address range.

A more defensible identity object is something like:

```text
source descriptor
+ source-memory provenance
+ observed byte/block generations over the episode
```

A later musical instrument identity may group multiple such realizations, but that grouping is a higher claim.

## Negative controls

Several sampled devices in the existing research surface are normally ROM-backed rather than RAM-backed, including representative SegaPCM, C140/C352, MultiPCM, MSM6295, and QSound paths.

For those controls, a source descriptor may resolve to immutable ROM bytes during ordinary execution.

The common model must therefore make mutable-memory observation optional rather than forcing RAM-version machinery into every PCM device.

This distinction is useful:

```text
ROM-backed source
-> stable source bytes under ordinary playback

RAM-backed source
-> source bytes may be time-bearing execution state
```

## Literature relation

The research pass found established digital-sampling/wavetable work that separates waveform memory from playback and describes cache/buffer management as an independent real-time synthesis concern.

That supports the generic principle:

```text
stored waveform != immediately observed playback data
```

The exact fetch granularities in this document remain device-specific evidence from RF5C68, PlayStation SPU, and Nintendo DS implementations.

Do not use generic sampler literature to infer a retro chip's exact prefetch or cache law.

## Highest-information next tests

### 1. SNES SPC/S-DSP pressure test

Determine whether BRR source mutation becomes visible per block, per decode buffer, or through another schedule.

This would add a fourth materially different RAM-backed platform.

### 2. Loop mutation test

For RF5C68, PS1 SPU, and DS, construct synthetic sequences that:

1. start a looping sampled voice;
2. mutate source bytes after first observation;
3. continue playback through the next loop;
4. record the first output coordinate at which the mutation becomes visible.

This directly estimates the device observation boundary.

### 3. End-condition mutation

On RF5C68, change a future ordinary sample byte to/from `0xFF` while playback is active and verify the resulting loop/termination trajectory.

This tests whether memory mutation can alter episode lifetime, not only waveform content.

### 4. PS1 block-cache discriminator

Mutate bytes in the currently decoded 16-byte ADPCM block after it has been fetched.

Expected model:

```text
already decoded 28-sample block remains stable
future refetch observes mutation
```

### 5. DS FIFO visibility measurement

Mutate source memory at controlled offsets ahead of the current DS FIFO read position and measure which offsets are already shielded by prefetch.

The result should be expressed in source-byte/fetch coordinates, not merely milliseconds.

## Stop conditions

Stop rather than guess if:

- a RAM address is called a stable sample identity;
- a memory write is assumed audible immediately without device fetch evidence;
- a global RAM generation is equated with the generation currently observed by every active voice;
- a decoded/prefetched block is retroactively changed because backing memory changed;
- ROM-backed and RAM-backed devices are forced into one memory-mutation policy;
- decoder/interpolation history is discarded when encoded bytes change;
- source mutation is treated as only a timbre effect when it can also change loops or termination.

Correction outranks coherence.
