# GBA MP2K execution and rendering boundary

## Question

Why is Game Boy Advance MusicPlayer2000 / m4a / MP2K unusually useful to Game Music Interpreter?

Because it exposes, in one open research surface:

- structured sequence semantics;
- runtime/game-state-dependent control flow;
- multiple logical music-player instances;
- software PCM mixing above limited hardware PCM output;
- known commercial driver variants;
- high-quality independent playback implementations;
- and an explicit higher-quality replacement mixer that preserves the surrounding music engine.

Pinned/documentary observatories used here:

- `loveemu/agbinator` `ec56b1ecfebddd29131f4934f65a94dfabcaf65d`
- `loveemu/vgmdocs` `6c3c455071d81457e7a8cfec378a2edb8125ce7b`
- `ipatix/agbplay` `0960aadec72dddbefc144216886d86bef220a0bb`
- `ipatix/gba-hq-mixer` `2bd31435101e4c3a4af568a6ed33d7cc9f1a6da9`

This does not make MP2K representative of every GBA engine. It is one unusually well-observed lineage.

## MP2K is a software synthesis system, not merely a sequence format

The GBA standard driver combines:

```text
multitrack sequence execution
+ instrument/bank lookup
+ software PCM voice mixing
+ optional Game Boy-compatible PSG/wave/noise channels
+ reverb/echo processing
+ final hardware output
```

The exact musical result therefore depends on both sequence state and mixer/runtime state.

A static ROM parser is not enough to reproduce the performed trajectory.

## Sequence bytes do not define one fixed performance

MP2K includes ordinary musical controls such as:

- wait/delta-time;
- note/tie/note-off;
- instrument;
- volume;
- pan;
- pitch bend and bend range;
- LFO speed and delay;
- modulation depth/type;
- micro-tuning;
- tempo;
- transpose;
- patterns, repeats, and jumps.

More importantly, the sequence language includes `MEMACC`, which can read/write a shared RAM area and perform conditional branches from runtime memory state.

Therefore:

```text
sequence bytes
+ initial game/runtime state
+ memory mutations through time
-> executed event path
```

and not:

```text
sequence bytes -> one immutable linear note stream
```

A flat MIDI extraction can be a useful projection of one path, but cannot be the canonical source representation when alternative branches remain reachable.

## Game state can be musically causal

When sequence control flow depends on shared memory, external runtime state can decide which musical events execute.

This means the provenance graph may need a causal edge such as:

```text
game/runtime state variable
-> sequence branch decision
-> later note/control events
```

The game state itself is not automatically a musical object, but it can be an exact cause of musical execution.

This is an important bridge between "adaptive music" and ordinary sequence interpretation: adaptivity may be implemented inside the music bytecode rather than only by the game choosing separate tracks.

## BGM and SFX are runtime roles, not intrinsic sequence classes

The MP2K documentation notes that the engine does not fundamentally distinguish BGM data from SFX data. Both are sequence+bank combinations.

Games commonly maintain several music-player instances, with one often used for music and others for effects.

Therefore:

```text
sequence object != BGM
sequence object != SFX
```

The stronger relation is:

```text
sequence object
+ player instance
+ runtime invocation/context
-> current functional role
```

This supports the broader project law that role belongs to execution/context rather than being guessed from file shape.

## Logical tracks are not final mixer voices

MP2K sequence tracks exist above the software PCM mixer and the Game Boy-compatible hardware channels.

The driver can configure the number of software PCM channels, process multiple voices, and combine them into the hardware-facing output buffers.

Therefore:

```text
sequence track
!= software mixer voice
!= GBA hardware PCM channel
!= persistent musical part
```

The GBA is an especially good adversary against `hardware channel = musical voice` because many software voices are mixed before reaching only two hardware PCM output channels.

## Output hardware is a severe information bottleneck

For MP2K PCM, many software-synthesized sources are collapsed into a final low-resolution stereo output path.

A downstream capture of the two PCM hardware channels therefore loses the separability available in the software mixer state.

This is the inverse of simple PSG/chip cases where hardware voice slots remain separately observable.

For Game Music Interpreter, the best evidence route is consequently:

```text
sequence/player state
-> software voice state
-> software mix buffer
-> hardware PCM output
```

not merely:

```text
hardware PCM output -> infer all voices
```

## The reference renderer has implementation artifacts

The standard MP2K mixer processes 8-bit PCM and combines voices using constrained low-precision buffers and CPU budget.

`ipatix/gba-hq-mixer` documents a replacement mixer designed specifically to reduce the resulting noise by using a higher-precision internal buffer and quantizing only at the final stage.

The reported mechanism is straightforward DSP:

```text
standard path:
voice processing and accumulation at low precision
-> repeated/cascaded quantization error

HQ path:
higher-precision internal accumulation
-> one later down-quantization
-> lower audible quantization noise
```

General audio-engineering literature independently supports quantization/roundoff error as a computational artifact and warns that cascaded quantizers can imprint additional errors.

The device/project evidence remains stronger for the exact MP2K implementation.

## This is a source-native enhancement control

The HQ mixer is unusually valuable because it changes a lower implementation constraint while leaving the higher music engine intact.

Conceptually:

```text
same sequence
same bank/sample material
same note/control logic
same driver-facing voice semantics
same arrangement

but

different internal mixer precision/implementation
```

This gives Game Music Interpreter a practical identity-preservation experiment:

```text
reference MP2K realization
vs
higher-precision MP2K realization
```

Ask which properties remain invariant and which audible defects disappear.

This is much stronger than inventing an arbitrary "remaster" from final audio.

## Historical-intent boundary

A higher-quality mixer does **not** prove that every original low-resolution artifact was unwanted by the composer or programmer.

The correct claim is narrower:

```text
implementation ceiling can be relaxed
while preserving higher source/driver semantics
```

Historical intent requires separate evidence.

However, the existence of commercial modified MP2K lineages and `gba-hq-mixer`'s inspiration from reverse-engineered Golden Sun mixer behavior show that higher-quality realization within the same broad engine family is technically coherent, not merely hypothetical.

## agbplay gives reference/enhanced parameter controls

`ipatix/agbplay` exposes several rendering choices that make it useful as a mechanism observatory:

- different PCM resampling algorithms;
- game-specific samplerate/master-volume settings;
- multiple reverb implementations, including Camelot variants;
- strict/smoothed/polyphonic handling for compatible PSG behavior;
- optional simulation of known engine bugs;
- track muting/soloing and split rendering.

Some options deliberately move away from exact hardware/driver behavior.

That makes the necessary distinction explicit:

```text
reference configuration
!= analysis/isolation configuration
!= enhanced rendering configuration
```

Every render used as evidence must retain which configuration produced it.

## A revealing bug toggle

`agbplay` documents a `simulate-cgb-sustain-bug` option and notes that disabling the bug may make certain songs sound closer to intended musical behavior in some cases.

This is an excellent philosophical pressure test.

The interpreter should preserve both:

```text
historically executed behavior
```

and:

```text
counterfactual corrected behavior
```

without silently replacing one with the other.

A correction candidate needs an explicit reason, source evidence, and preserved reference path.

## GSF consequence

If GSF is added to the xSF family, it should enter as:

```text
GSF effective executable/ROM object
-> identify sound-driver lineage
-> execute game/driver state
-> recover logical player/track/voice state
-> software synthesis/mixer state
-> GBA hardware output
```

not as:

```text
GSF -> generic GBA notes
```

MP2K can be the first strong GSF control because the ecosystem is unusually rich, but GAX, MusyX, Krawall, and other drivers must remain independent pressure tests.

## Highest-information next tests

1. Add a small GSF corpus with one MP2K title and at least one non-MP2K title.
2. For MP2K, execute one sequence containing ordinary static control flow and one containing `MEMACC` conditional behavior.
3. Record sequence branch provenance so one note can be traced back to the runtime condition that made it reachable.
4. Compare logical track count, software voice count, and final two-channel PCM output through the same interval.
5. Render one bounded cue under reference MP2K mixing and higher-precision mixing; compare identity-preserving invariants and quantization/noise changes.
6. Keep any "composer intended" claim blocked unless independent historical evidence exists.

## Stop conditions

Stop rather than guess if:

- a GSF file is treated as a generic note container;
- MP2K is treated as the only GBA driver;
- BGM/SFX role is inferred solely from sequence structure;
- a conditional sequence is flattened without recording which branch executed;
- final hardware PCM channels are promoted to source voices;
- an enhanced mixer replaces the reference renderer rather than remaining a controlled alternative;
- higher quality is equated automatically with historical intent.

Correction outranks coherence.
