# Chrono Cross AKAO structural frontier

## Question

What can VGM Compiler establish about *Chrono Cross* AKAO data from reconstructed PSF1 memory, independent parsers, and the same-game decompilation without collapsing structure, driver execution, and SPU state into one layer?

This pass began from VGMTrans-only structural evidence. A later same-game decompilation materially resolved several previously unknown semantics, so this document has been corrected in place rather than preserving stale uncertainty.

## Evidence roles

Pinned observatories:

- `vgmtrans/vgmtrans` `083f7c71fe773078061eb785573621082c3e0d1c`
- `jdperos/chrono-cross-decomp` `7dcadfc36421c9b26466f7fdbdbaa1a1102219c6`
- ValleyBell `MidiConverters/AKAO2MID.bas` as an independent AKAO converter lineage

The Chrono Cross decompilation is exceptionally valuable because it targets the same work and reconstructs the game's sound VM/driver. It is still a reverse-engineered reconstruction, not original authored source. Function and field names may include analyst interpretation; executable behavior and data-flow are stronger evidence than labels alone.

## Bounded structural surface

VGMTrans supplies byte-structure hypotheses that can be tested directly against reconstructed PSF1 memory without adopting its MIDI projection.

For later AKAO sequences the structural surface includes:

```text
"AKAO" signature
+0x04 sequence ID
+0x06 declared sequence length
+0x14 associated sample-set ID
+0x20 track-allocation bitmap
+0x28/+0x2C/+0x38/+0x3C zero-valued structural fields
+0x30 instrument-data relative pointer
+0x34 drumkit relative pointer
+0x40 track-pointer table
```

VGMTrans's tagless `guessVersion()` is weaker than exact version attribution: a zero word at `+0x2C` is sufficient for its `VERSION_3_2` guess. Because later structural validation already expects that field to be zero, VGM Compiler must not turn this heuristic into exact version proof.

The bounded probe therefore reports:

```text
v3-compatible; VGMTrans-style 3.2 heuristic only
```

rather than:

```text
proven AKAO 3.2
```

Game/title provenance remains separate supporting evidence.

## Executable structural probe

`components/psf/akao_probe.py` scans a reconstructed `Psf1EffectiveImage` for literal AKAO signatures and classifies them without CPU or SPU execution.

It distinguishes:

```text
literal AKAO signature
sequence candidate
zero-length/non-sequence signature
rejected structural candidate
```

A zero declared sequence length is not promoted to a sequence because AKAO signatures can also identify sample collections.

Accepted candidates preserve:

- reconstructed memory address;
- declared length;
- sequence ID;
- associated sample-set ID;
- track bitmap/count;
- bounded track targets;
- instrument/drumkit targets;
- warnings;
- raw `FE 13` byte-pair count.

The probe still does not claim:

- exact AKAO version;
- complete event decoding;
- runtime use of the object;
- driver allocation behavior;
- SPU correspondence;
- authored source recovery.

## Correction: FE 13 semantics are now resolved at the driver layer

VGMTrans historically preserved late AKAO sub-opcode `FE 13` as unknown and explicitly cited three Chrono Cross cues:

- `114 Shadow Forest`
- `119 Hydra Marshes`
- `302 Chronopolis`

That was the strongest available evidence at the time and was correctly retained as unknown rather than guessed.

The same-game Chrono Cross decompilation now reconstructs the handler as:

```text
SoundVM_FE13_PreventVoicesFromRekeyingOnResume
```

Its behavior sets the current logical channel bit in:

```text
PreventRekeyOnMusicResumeMask
```

When the game pushes/suspends the current music state, the copied suspended context then performs:

```text
SuspendedMusicContext.ActiveNoteMask
    &= ~SuspendedMusicContext.PreventRekeyOnMusicResumeMask
```

Later music-state restoration derives pending key-ons from the saved active-note state.

The earned semantic claim is therefore:

```text
AKAO FE 13
-> mark this logical channel as not eligible to be re-keyed from the saved active-note state when suspended music resumes
```

This is more precise than merely calling it a resume flag.

The structural probe's raw-byte rule does not change:

```text
raw bytes FE 13 inside a declared sequence span
!= decoded FE 13 event occurrence
```

Known event semantics do not prove that a raw byte pair lies on an executed event boundary or runtime path.

## Correction: C6 is an earned AKAO -> SPU PMON route

VGMTrans labels AKAO `0xC6` as `FM (Pitch LFO) On`, while earlier research correctly refused to equate that label with the PlayStation SPU PMON register without runtime evidence.

The Chrono Cross decompilation now supplies the missing driver route.

Its opcode vocabulary names:

```text
C6 ENABLE_FM_VOICES
C7 DISABLE_FM_VOICES
```

The C6 handler marks the current logical music channel in `FmChannelFlags`. During the global voice-mode update, the driver:

```text
logical FmChannelFlags
-> ChannelMaskToVoiceMask / assigned physical voice mapping
-> g_Sound_VoiceModeFlags.Fm
-> SetVoiceFmMode(...)
-> SPU voice FM/PMON register
```

Therefore, for this reconstructed Chrono Cross driver:

```text
AKAO C6/C7
-> logical FM-mode state
-> dynamic logical-to-physical voice translation
-> physical SPU PMON mask update
```

is now an earned correspondence.

This does not mean every historical AKAO driver version used identical implementation details. The claim is currently strongest for the Chrono Cross driver lineage represented by the decompilation.

## D4/D6 are different side-chain mechanisms

The same-game opcode table separately names:

```text
D4/D5  enable/disable playback-rate sidechain
D6/D7  enable/disable pitch-volume sidechain
```

These are not aliases for C6 PMON.

The driver-side update path for D4 reads a 16-bit value at `current FSoundChannel - 0x0C`.

The reconstructed channel layout makes that address interpretable:

```text
sizeof(FSoundChannel) = 0x124
VoiceParams offset    = 0x108
SampleRate offset     = +0x10 within VoiceParams
previous channel SampleRate absolute offset = 0x118
current channel - 0x0C = previous channel + 0x118
```

Thus D4's side-chain source is the previous **logical channel's computed `VoiceParams.SampleRate`**, which is fed into the current logical channel's software playback-rate calculation.

D6 uses the same previous-channel coordinate in a volume-sidechain calculation.

This yields three distinct coupling mechanisms that must not be flattened:

```text
C6 hardware PMON
previous PHYSICAL voice post-source/post-ADSR mono signal
-> next physical voice phase step

D4 software playback-rate sidechain
previous LOGICAL channel computed sample-rate coordinate
-> current logical channel sample-rate calculation

D6 software pitch-to-volume sidechain
previous LOGICAL channel computed sample-rate coordinate
-> current logical channel volume calculation
```

The resemblance in musical effect does not erase the representation layer or causal source.

## Chrono Cross has 32 logical sound channels above 24 SPU voices

The same-game decompilation defines:

```text
SOUND_CHANNEL_COUNT = 0x20 = 32
VOICE_COUNT         = 24
```

and classifies software channels as music, SFX, or menu channels.

Independent ValleyBell AKAO tooling had already reported that *Chrono Cross: The Brink of Death* uses 31 AKAO channels. The decompilation now independently explains how that can coexist with 24 physical voices: software channels begin unassigned and are dynamically mapped onto the SPU pool.

Therefore:

```text
logical AKAO/sound channel
!= physical SPU voice
```

is a same-work executable fact, not merely an architectural preference.

## Physical allocation is dynamic

On a pending logical key-on, the reconstructed driver follows a bounded policy:

```text
if a physical voice is already/pre-allocated for this logical channel:
    use it
else:
    Sound_FindFreeVoice(...)
    if no free voice:
        Sound_StealQuietestVoice(...)
    if still unavailable:
        mark allocation exhausted
    else:
        assign physical voice
        write full voice parameters
        record voice-owner context
```

The driver retains explicit status for both:

```text
voice stolen
allocation exhausted
```

The steal path chooses the lowest current SPU envelope level in the eligible range and unassigns that physical voice from its previous logical owner.

This is stronger than a generic `voice stealing` label because the selection coordinate is observable:

```text
current physical SPU envelope level
```

## Allocation floor is programmable policy, not a fixed partition

The driver has a music-context field controlling the starting SPU voice index used by free/steal scans.

Chrono AKAO extended opcodes currently reconstructed as `FE 10` and `FE 11` modify that field:

```text
FE 10 -> load allocator starting voice index from sequence byte
FE 11 -> reset allocator starting voice index to 0
```

The normal free/quietest scan begins at that configured floor. A re-key/full-scan condition can instead force the scan to begin at voice 0.

This means a statement such as:

```text
music owns voices 0..11, SFX owns 12..23
```

would be too rigid.

At initialization the game does preassign 12 SFX software channels to physical voices 12..23, but music allocation policy can dynamically change its scan floor and can perform full-range scans under specific conditions.

Preserve:

```text
initial/preferred resource assignment
!= immutable hardware partition
```

## Suspended music remains another allocation domain

Chrono can preserve an active and a suspended music context. Each context retains logical channel state and physical-assignment evidence.

Voice-mode reconstruction accounts for:

- active music logical mode flags;
- suspended music logical mode flags;
- persistent SFX physical mode flags;
- current logical->physical assignments.

This is why a simple one-song `track -> voice` table is insufficient even before sound effects are considered.

## VGMTrans remains a valuable partial inverse model

The same-game decompilation does not make VGMTrans obsolete.

VGMTrans still provides an independent parser lineage, format-version comparisons, instrument/sample extraction, and documented failure cases. The history is scientifically useful because it records where static interpretation failed before deeper driver semantics were available.

Examples retained as adversarial controls include:

- *Dream's Creation*: valid sequence without required custom instrument/drum pointers;
- *Dragon God*: tuning/export, sample, loop, and ADSR edge cases;
- late AKAO CPU/runtime-dependent branches where static traversal does not cover every runtime path.

The stronger model is:

```text
VGMTrans structural/static inverse
+ same-game driver reconstruction
+ SPU implementation/hardware evidence
-> cross-checkable vertical interpretation
```

not replacement of one observatory with another.

## Tests earned so far

Synthetic regression coverage protects these structural boundaries:

1. accept a bounded v3-compatible sequence structure without claiming exact version;
2. do not promote a zero-length AKAO signature to a sequence;
3. reject nonzero later-v3 reserved fields;
4. reject track targets outside the declared sequence span;
5. count `FE 13` only as a raw clue;
6. retain suspicious track targets into the pointer-table region as warnings rather than silently repairing them;
7. keep structural/runtime/SPU claim states separate;
8. permit 31 logical AKAO tracks rather than inheriting the SPU's 24-voice limit.

The real 68-file PSF corpus audit is still not claimed as executed because the current GitHub connector cannot expose private binary PSF payloads to the local runtime and hosted CI is spending-limit blocked.

## Highest-information next experiments

### 1. Build a bounded Chrono AKAO event walker

Now that the same-game driver exposes exact opcode widths/handlers, decode one accepted AKAO object while retaining control flow and unknown operations.

Priority checks:

- `FE 13` event occurrence in Shadow Forest / Hydra Marshes / Chronopolis;
- `FE 10` / `FE 11` allocator-floor changes;
- C6/C7 FM/PMON changes;
- D4/D6 software side-chain events;
- FE0B, which remains less certain.

### 2. Trace one logical note through allocation

For a high-pressure cue such as *The Brink of Death*:

```text
logical track note
-> pending key-on
-> free/steal decision
-> physical AssignedVoiceNumber
-> voice-owner context
-> SPU parameter writes
-> physical voice trajectory
```

Measure actual stealing and allocator-floor behavior rather than assuming it occurs merely because 31 logical channels exist.

### 3. Compare C6 and D4 causally

Find cue intervals using each effect and verify:

```text
C6 -> PMON mask transition
D4 -> software SampleRate trajectory without requiring PMON
```

This is an ideal representation-boundary test because the two effects can sound conceptually related while living at different layers.

## Stop conditions

Stop rather than guess if:

- a VGMTrans version heuristic is reported as exact provenance;
- a literal `AKAO` signature is automatically called a sequence;
- raw `FE 13` bytes are automatically called an executed FE13 event;
- the same-game decompilation is called original authored source;
- C6 PMON semantics are generalized to every AKAO driver version without evidence;
- D4/D6 are collapsed into SPU PMON because they are also side-chain effects;
- 32 logical sound channels are confused with 24 simultaneous physical voices;
- initial SFX voice preassignment is called an immutable 12/12 partition;
- successful static parsing is treated as complete runtime path coverage.

Correction outranks coherence.
