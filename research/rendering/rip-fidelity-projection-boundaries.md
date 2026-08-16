# Rip fidelity and projection boundaries

## Status

Research input for executable-rip validation, source-native export, and stem/isolation semantics.

This pass was prompted by cross-checking `loveemu/vgmdocs`, HCS64 forum history, reversible native-format tooling, and music-representation literature.

The central correction is:

```text
container validity
!= rip fidelity
!= projection fidelity
```

A file can be structurally valid and dependency-complete while still being a musically incorrect executable witness. A faithful executable witness can then be projected into MIDI/SF2/DLS/WAV in ways that are useful but non-invertible.

These statuses must remain separate.

## 1. Container validity is the lowest question

For xSF and related executable-rip formats, container validity can establish facts such as:

- magic/version;
- compressed program integrity;
- CRC behavior;
- tag framing;
- `_lib` dependency resolution;
- deterministic effective-object reconstruction;
- platform-specific load-map validity.

None of those facts proves that the rip reproduces the intended in-game performance.

The stronger path is:

```text
valid container
-> resolved dependency graph
-> reconstructed executable/runtime object
-> correct initialization state
-> correct runtime behavior
-> reference-equivalent musical performance
```

Every arrow can fail independently.

## 2. Rip fidelity is a separate evidence axis

### GSF: correct bytes can still enter the driver incorrectly

HCS64's `Issue with Racing Gears Advance rip` documents a GBA MusyX GSF set whose affected tracks played at the wrong speed even after converting the GSF back to ROM and running it in mGBA.

The later reverse-engineering result identified the cause as the game's MusyX initialization function being called with the wrong argument. Different argument values selected different initialization parameters, including a different timer-1 frequency.

Source:

- https://www.hcs64.com/mboard/forum.php?showthread=61024

This establishes a general counterexample:

```text
correct executable payload
+ runnable entry point
!= correct performance
```

when musically causal initialization parameters are missing or wrong.

A future executable-rip fidelity record may therefore need evidence for:

- entry point;
- stack/register state;
- save-state or RAM state;
- initialization arguments;
- timer/clock mode;
- selected song/subsong identifier;
- runtime flags or shared variables;
- required interrupt/DMA timing assumptions;
- game-state inputs that influence the music path.

Do not create one universal state schema yet. Preserve source-family-specific coordinates and provenance.

### USF: song number is not complete game state

The USF FAQ documents tracks with missing intros or wrong tempo when a rip is produced from one game point and only the song selector is varied. Other state can participate in determining the actual playback path.

Source:

- https://www.hcs64.com/usf/usffaq.html

Therefore:

```text
song identifier
!= sufficient playback state
```

### USF: optimization can encode emulator assumptions

The HCS64 `USF on the real hardware` discussion documents historical rips trimmed around assumptions that hardware actions, interrupts, and audio DMA would effectively complete immediately. More accurate timing can expose those assumptions and break rips that previously played in permissive players.

Source:

- https://www.hcs64.com/mboard/forum.php?showthread=50243

This adds another distinction:

```text
plays in reference player
!= timing-faithful executable witness
```

The player's tolerated machine model is part of the evidence route.

### PSF: old player bugs can hide invalid executable behavior

The HCS64 xSF discussion records an old Chocobo's Dungeon 2 PSF rip whose branch/delay-slot behavior was tolerated by older PSF CPU code and exposed only when later player code became more accurate.

Source:

- https://www.hcs64.com/mboard/forum.php?showpage=200&showthread=39

The exact game-specific story should not be generalized beyond the evidence, but it proves the class of failure:

```text
historically accepted rip
+ historically accepted player
!= proof of valid machine semantics
```

## 3. Reference player and reference hardware are different controls

HCS64's `N64 Zelda rips wrong tempo` discussion attributes a tempo discrepancy to the emulator core used by historical USF playback rather than to the sequence itself.

Source:

- https://www.hcs64.com/mboard/forum.php?showthread=33954

GSF history similarly records different VBA/VBA-M-derived player cores with timing differences and game-specific fixes.

Source example:

- https://www.hcs64.com/mboard/forum.php?showpage=173&showthread=39

Therefore validation should report separately:

```text
container parity
runtime-object parity
reference-player parity
independent-emulator parity
hardware parity
```

when those controls are actually available.

Do not collapse them into one `accurate = true` flag.

## 4. Native sequence representation may be executable, not merely symbolic

### N64 Music Macro Language / AudioSeq

SEQ64 describes first-party Nintendo 64 Music Macro Language as containing both musical and program-like instructions:

- notes;
- pitch bend;
- instrument selection;
- branches;
- loops;
- calls;
- memory I/O;
- variables and related control behavior.

Its Ocarina of Time sound-effects sequence is a large program that reacts to messages from the game engine.

Source:

- https://github.com/sauraen/seq64

SEQ64 also reports byte-exact binary -> assembly -> binary round trips for several first-party N64 sequence programs, including Ocarina of Time.

This gives a useful validation hierarchy:

```text
native binary
<-> native structural/assembly representation
```

can sometimes be reversible, while:

```text
native binary
-> MIDI
```

is generally a task projection.

### Runtime modifications can live outside the nominal sequence/bank

The HCS64 `My personal VGMSequence concept` discussion records practical reverse-engineering cases in which games keep musically important state outside nominal sequence and bank objects, including runtime instrument properties, custom controllers, per-song key-region settings, and per-song reverb settings.

Source:

- https://www.hcs64.com/mboard/forum.php?showthread=65385

This reinforces:

```text
sequence + bank
!= universally complete performance program
```

## 5. MIDI/SF2/DLS are projections, not canonical truth

`loveemu/vgmdocs` is useful precisely because it catalogs many historical conversion paths and their known limitations.

Source:

- https://github.com/loveemu/vgmdocs/blob/master/Conversion_Tools_for_Video_Game_Music.md

Examples include tools whose limitations concern:

- frame-limited timing resolution;
- pitch detection;
- pitch bends;
- vibrato/tremolo;
- instrument mapping;
- driver-specific controllers;
- sequence control flow;
- incomplete bank/sample reconstruction.

The lesson is not that MIDI export is bad. It is that export must declare the transformation.

A future projection result should be able to report coordinate-level status such as:

```text
note onset/duration        exact | derived | approximated | omitted
performed pitch trajectory exact | derived | approximated | omitted
control flow               preserved | executed-before-export | flattened | omitted
logical track identity     preserved | remapped | ambiguous
physical allocation        preserved-as-provenance | omitted
instrument identity        exact | structural | mapped | heuristic
sample identity            exact | transformed | omitted
synthesis trajectory       preserved | approximated | omitted
volume/envelope             exact | transformed | approximated | omitted
pan/routing                 exact | transformed | approximated | omitted
effects/reverb              exact | approximated | omitted
runtime/game-state inputs   preserved | fixed-to-one-path | omitted
```

No new graph primitive is required yet. This can begin as explicit export/projection provenance.

## 6. Literature support: there is no single universal music representation

Roger Dannenberg's 1993 review `Music Representation Issues, Techniques, and Systems` distinguishes notation, performance/control information, and resulting sound, and emphasizes that music contains relationships across pitch, time, timbre, articulation, phrasing, harmony, and other dimensions.

This independently supports the project's layered model.

A representation is useful relative to a task. It is not automatically a lossless ontology for another representation layer.

Recent expressive-performance work makes the same pressure visible from another direction: note-level symbolic representations can omit acoustic and expressive detail needed for perceptual evaluation or rendering.

These papers support the abstract representation boundary. They do not establish any retro-console-specific mechanism.

## 7. Physical channel isolation is not musical stem isolation

HCS64 discussions of PSF/xSF channel muting repeatedly warn that dynamic allocation can move logical notes across physical sound-chip channels.

Sources:

- https://hcs64.com/mboard/forum.php?showthread=59231
- https://www.hcs64.com/mboard/forum.php?showthread=51739
- https://hcs64.com/mboard/forum.php?showpage=1&showthread=44688

This produces a strong general rule:

```text
physical channel stem
!= persistent musical part stem
```

and sometimes:

```text
muting physical output
!= observationally neutral intervention
```

If the player/driver can observe envelope completion, sample state, reverb state, or related effects, a mute that changes execution can desynchronize the performance.

A safe low-level mute should preserve internal execution and suppress only the chosen output contribution where possible.

A musically meaningful stem should instead be selected at the highest proven stable layer available, for example:

```text
source track / sequence layer
-> logical note episodes
-> dynamic physical allocations
-> synthesis
```

while allowing the lower runtime to continue normally.

For drivers where no stable logical part layer is known, label hardware-channel dumps as hardware-channel dumps rather than musical stems.

## 8. Dynamic music makes one-path export especially dangerous

The HCS64 xSF thread documents Silent Hill music whose runtime progression adjusts track/channel volumes according to game state and initially mutes some dynamic layers.

Source:

- https://hcs64.com/mboard/forum.php?showpage=19&showthread=39

This proves another important class:

```text
one executed performance path
!= complete authored dynamic music program
```

A future interpreter should be able to distinguish:

- source program/graph;
- one observed runtime trajectory;
- reachable alternate trajectories;
- game-state condition or trigger where known.

Do not infer unreachable branches merely because the bytecode contains them. Reachability remains a runtime/control-flow claim.

## 9. NCSF is an excellent representation-boundary control

The NCSF development thread records several cases where playback correctness required implementing random/variable/comparison SSEQ commands and correcting track-allocation timing rather than merely parsing note events.

It also documents the importance of SDAT PLAYER objects, including channel-allocation masks.

Sources:

- https://www.hcs64.com/mboard/forum.php?showpage=10&showthread=34052
- https://www.hcs64.com/mboard/forumlong.php?showpage=1&showthread=34052

This directly supports the planned Mario Kart DS pair:

```text
2SF executable/runtime witness
versus
NCSF structured SDAT witness
```

The comparison must operate on named observables rather than declaring one representation universally superior.

## 10. Test protocol earned by this pass

For every executable-rip family admitted in the future, distinguish at least these questions:

### Container

- Is the file structurally valid?
- Is its dependency graph resolved exactly?
- Is the effective object deterministic?

### Rip

- Is the correct code/data present?
- Is initialization state known?
- Are runtime arguments/flags known?
- Are timing assumptions known?
- Is the song-selection state complete?
- Has optimization/minification changed required behavior?

### Runtime

- Does it match the historical/reference player?
- Does it match an independent emulator?
- Does it match hardware where a practical control exists?
- If controls disagree, is the disagreement preserved?

### Projection

- Which native semantics survive export?
- Which are executed and flattened?
- Which are approximated?
- Which are dropped?
- Is the projection reversible?

### Isolation

- Is the isolated object a physical resource, logical track, sample/instrument, or persistent musical part?
- Can the mute alter runtime behavior?
- Is dynamic allocation preserved?

## 11. Immediate consequences for Game Music Interpreter

### xSF

Do not treat successful effective-object reconstruction as reference parity.

Add a future rip-fidelity stage between effective-object reconstruction and higher musical analysis.

### MIDI/SF2/DLS export

When export arrives, make it explicitly projection-aware. A successful file write must not imply semantic completeness.

### Stems

Do not name physical-channel dumps `parts` or `stems` unless a higher identity mapping has been established.

### Enhancement

A higher-quality renderer should be compared against the best recovered performance state, not against an accidentally incorrect historical rip or an emulator-specific bug.

This matters especially for GBA, N64, and other software-mixed systems where incorrect timing or initialization can masquerade as platform character.

## Stop conditions

Stop rather than overclaim if:

- a valid xSF container is called a faithful rip without runtime evidence;
- a rip is considered exact because one permissive player accepts it;
- an emulator disagreement is silently normalized away;
- a MIDI export is called the source sequence when control flow or runtime semantics were flattened;
- a SoundFont/DLS mapping is called the original instrument definition without source proof;
- a hardware voice dump is called a musical stem under dynamic allocation;
- muting changes internal runtime state but the result is called an isolated original part;
- one observed dynamic-music path is called the complete composition;
- an enhanced renderer uses an inaccurate rip as the unquestioned reference baseline.

Correction outranks coherence.
