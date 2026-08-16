# Sample-loop realization semantics

## Status

Cross-device research input for sampled-voice execution, sample identity, loop provenance, note lifetime, and musical repetition analysis.

## Central correction

A waveform/sample object does not universally own one intrinsic loop behavior.

Across several unrelated systems, performed looping is distributed among some combination of:

- encoded sample/block flags;
- external loop pointers;
- voice registers;
- runtime mode bits;
- current playback state;
- driver setup.

Therefore:

```text
sample byte identity
!= loop realization identity
```

and:

```text
same sample bytes
can support different performed loop trajectories
```

The common object should preserve sample/source identity separately from the time-bearing traversal/loop policy used by a voice episode.

## 1. Super NES S-DSP / BRR

Pinned observatory:

- `ares-emulator/ares`
- commit `b80f67d38312648d197762121c3a27b02c0887db`
- `ares/sfc/dsp/voice.cpp`

The S-DSP source directory is addressed from:

```text
DIR bank/page
+ SRCN * 4
```

and supplies pointer data used by the current voice.

At playback, BRR block headers carry end/loop behavior, but the address used when the encoded stream reaches its terminal/loop condition is not encoded as a full destination address inside the BRR waveform block itself.

The DSP obtains the relevant pointer from the source directory and, when the BRR terminal condition is reached, redirects playback through that externally supplied address.

The header bits also participate in whether the voice continues looping or transitions toward termination/release.

Thus the realized loop depends on at least:

```text
BRR encoded blocks
+ BRR terminal/loop flags
+ current SRCN
+ current DIR/source-directory mapping
+ directory loop pointer
+ current voice/envelope state
```

Consequently:

```text
BRR bytes alone
!= complete looped sample object
```

Changing the source-directory loop pointer can change the traversal while leaving the BRR bytes unchanged.

## 2. PlayStation SPU ADPCM

Pinned observatory:

- `stenzek/duckstation`
- commit `5fd366809053fe287291de7a39752c4d5d5b146b`
- `src/core/spu.cpp`

PlayStation ADPCM blocks carry three relevant flags:

```text
loop end
loop repeat
loop start
```

The voice also owns an `adpcm_repeat_address` register.

DuckStation's execution path demonstrates the interaction:

1. a decoded block with `loop_start` can set the voice repeat address to the current block address;
2. software can also write the repeat-address register directly;
3. timing/state decides whether the encoded loop-start flag overwrites that software-provided repeat address;
4. when `loop_end` is encountered, the current address is redirected to the voice repeat address;
5. the `loop_repeat`/end behavior determines whether playback continues or terminates/mutes according to the device rules.

Therefore:

```text
PS1 loop destination
!= merely an immutable property of the ADPCM byte stream
```

The complete realization includes encoded block metadata **and** mutable per-voice state.

This is especially important when a sound programmer deliberately writes the repeat address before or during playback.

## 3. Nintendo DS SWAV -> channel registers

Pinned observatory:

- `CyberBotX/NCSF`
- commit `fe1b91afec25fe18a10fe1697f95341e8dd5a44d`
- `NCSFCommon/Channel.cs`

The modern NCSF player, adapted from the Pokémon Diamond decompilation, represents the relevant Nintendo DS channel registers explicitly.

The register surface includes:

```text
RepeatMode
LoopStart
Length
Source
Format
Enable
```

with repeat modes corresponding to manual, infinite loop, one-shot, and a prohibited value.

When a PCM channel is started from an SWAV, the player maps SWAV metadata into device-like state:

```text
SWAV Loop flag
-> RepeatMode = infinite loop or one-shot

SWAV LoopOffset
-> channel LoopStart

SWAV LoopLength
-> channel Length
```

Thus the source file contains a loop description, but the performed loop is realized by channel runtime state.

This distinction matters because:

```text
SWAV metadata
!= current channel playback state
```

Once loaded into a running system, the device/driver state is the immediate cause of traversal.

## 4. Namco C352

See:

- `research/namco-pcm-voice-semantics.md`

The C352 supplies a different geometry again.

Its physical voice state includes:

- sample start/end/loop coordinates;
- current sample position;
- forward/reverse direction state;
- forward looping;
- reverse/bidirectional looping;
- link/long-format behavior;
- key/busy/loop-history state.

Those traversal modes are device/voice semantics applied to waveform memory.

Thus:

```text
waveform memory bytes
!= C352 loop/direction trajectory
```

and even:

```text
same start/end/loop addresses
!= same trajectory
```

when direction/loop flags differ.

## 5. Loop source and loop realization are separate evidence layers

A useful lower route is:

```text
sample/source object
-> loop-related authored/source metadata where present
-> driver/device programming
-> voice traversal state
-> actual source-address trajectory
-> acoustic repetition
```

Each arrow may preserve, reinterpret, or override loop semantics.

For example:

```text
DS SWAV loop metadata
-> DS RepeatMode/LoopStart/Length

PS1 ADPCM loop marker
+ software repeat-address write
-> effective repeat address

SNES BRR terminal flags
+ external directory pointer
-> effective traversal
```

Do not flatten all of these into a field named merely `loop_start`.

## 6. Sample identity should not include one canonical loop by default

A source sample can be reused with:

- no loop;
- one loop region;
- another loop region;
- forward versus reverse traversal;
- a driver-defined release/exit behavior;
- different playback bounds.

Therefore reusable sample identity should primarily describe the source waveform/object and its provenance.

A looped instrument region may be a higher compound object:

```text
sample identity
+ traversal bounds
+ loop destination/policy
+ tuning
+ envelope
+ other synthesis state
```

when source evidence supports that grouping.

This is especially important for future sample-source attribution. A commercial-library waveform can be the historical source even if the game edits or supplies different loop points.

## 7. Loop event is not automatically musical repetition

A low-level sample loop may simply sustain one note.

Conversely, a musical phrase loop may occur entirely above the sample layer while every constituent sample is one-shot.

Therefore:

```text
sample loop
!= note repetition
!= phrase repetition
!= song loop
```

The interpreter must preserve each altitude separately.

A sample-address wrap can support a sustained-timbre explanation, but it is not evidence by itself for formal musical repetition.

## 8. Note lifetime and sample lifetime can diverge

A driver can release a note while a sample/reverb tail continues, or a voice can sustain a note by repeating a tiny loop region many times.

Thus:

```text
sample traversal lifetime
!= authored/performed note lifetime
```

A higher note episode should use driver/key/envelope evidence where available rather than ending or repeating solely at the sample-loop boundary.

## 9. Exact traversal is the strongest common lower object

Rather than normalizing every format into abstract loop metadata too early, recover the actual time-bearing source traversal when execution allows it:

```text
voice episode
-> source address/index through time
-> direction
-> wrap/jump events
-> effective repeat destination
-> termination event
```

This trajectory remains meaningful across:

- block-coded samples;
- linear PCM;
- ADPCM;
- reverse/bidirectional devices;
- mutable RAM-backed source memory.

It also composes naturally with `research/mutable-sample-memory-observation.md`:

```text
where the voice reads
+
when the device observes memory
```

together determine the actual sample data realized.

## 10. Projection/export consequence

An exported WAV sample plus a single loop tag can be useful, but it may be a projection rather than a canonical source object.

Export should state whether the loop came from:

```text
encoded source metadata
external directory/pointer data
voice register state
driver reconstruction
one observed runtime trajectory
heuristic loop detection
```

Do not write one loop point into an exported sample and then back-project it as the intrinsic historical sample definition.

## 11. Enhancement consequence

A source-native enhanced renderer may replace resampling/precision or recover a higher-quality source waveform while preserving the historical loop realization.

That requires treating these independently:

```text
waveform source quality
loop/traversal policy
```

Replacing a waveform with a cleaner upstream source should not silently replace the game's edited loop points or reverse/bidirectional behavior.

Likewise, smoothing a historically imperfect loop requires an explicit intervention rather than being called reference playback.

## 12. Highest-information regressions

### SNES

Use a synthetic BRR source where the same BRR data is referenced by two source-directory entries with different loop pointers.

Expected:

```text
same BRR bytes
!= same source-address trajectory
```

### PS1

Construct cases that distinguish:

1. encoded loop-start chooses repeat address;
2. software repeat-address write wins under the device-specific timing condition;
3. loop-end + repeat continues;
4. loop-end without repeat terminates according to reference behavior.

### Nintendo DS

Use the same SWAV source under looping and one-shot channel configuration where the runtime surface permits it.

Expected:

```text
same waveform
!= same playback lifetime/traversal
```

### C352

Use the same waveform bounds under forward loop and reverse/bidirectional loop modes.

Expected:

```text
same waveform identity and bounds
!= same traversal trajectory
```

## Stop conditions

Stop rather than overclaim if:

- sample bytes are assigned one universal intrinsic loop identity;
- a BRR header is treated as containing the complete SNES loop destination;
- PS1 ADPCM loop flags are interpreted without the voice repeat-address state;
- DS SWAV metadata is treated as identical to live channel register state;
- C352 reverse/bidirectional traversal is flattened into a forward `loop_start/end` pair;
- a low-level sample loop is promoted directly to musical phrase/form repetition;
- an exported sample loop tag is back-projected as canonical source truth without provenance;
- higher-quality source substitution silently changes historically programmed loop behavior.

Correction outranks coherence.
