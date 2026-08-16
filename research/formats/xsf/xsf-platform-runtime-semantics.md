# xSF platform runtime semantics

## Question

After PSF1, USF, and 2SF containers have been admitted and their effective platform objects reconstructed, what additional execution layers are required before Game Music Interpreter can claim notes, instruments, physical voices, performed pitch, or heard musical gestures?

This pass uses the newly added xSF corpus as a pressure test rather than forcing PlayStation, Nintendo 64, and Nintendo DS audio into the VGM/chip-register model.

## Evidence method

Prefer a vertical route through the strongest available evidence:

```text
xSF dependency/envelope object
-> reconstructed platform memory/ROM object
-> platform/game runtime semantics
-> logical sequence/note/instrument state where exposed
-> physical synthesis resource state
-> acoustic realization
-> musical interpretation
```

Implementation observatories used here include:

- `zeldaret/oot` `05e9046417d11d8a9660074b1c014023b337eb27`
- `n64decomp/sm64` `9921382a68bb0c865e5e45eb594d9c64db59b1af`
- `stenzek/duckstation` `5fd366809053fe287291de7a39752c4d5d5b146b`
- `melonDS-emu/melonDS` `d3cd6164deb1f217d4b262d18af3ef9b97e536c8`
- `vgmtrans/vgmtrans` `083f7c71fe773078061eb785573621082c3e0d1c`

Scholarly anchors include:

- Donahue, Mao & McAuley, *The NES Music Database: A multi-instrumental dataset with expressive performance attributes* (ISMIR 2018 / arXiv:1806.04278)
- Cardoso, Moraes & Ferreira, *The NES Video-Music Database* (2024)
- Karen Collins, *In the Loop: Creativity and Constraint in 8-bit Video Game Audio* (2007)
- Kevin R. Burke, *Hard Limitations and Soft Possibilities: A “Systematic” History of Early Video Game Sound Technology* (2024)

The literature supports representation and historical interpretation boundaries. It does not substitute for platform-specific runtime evidence.

## USF / Nintendo 64: Ocarina of Time gives a same-work vertical observatory

The permanent USF corpus contains *The Legend of Zelda: Ocarina of Time*.

The matching `zeldaret/oot` reconstruction is unusually valuable because its audio system exposes several layers that a USF snapshot alone does not name.

### Sequence language exists above rendered note state

OoT's reconstructed `seqplayer.c` identifies the music representation as a customized assembly language based on MIDI, with three distinct instruction sets:

```text
sequence instructions
channel instructions
layer instructions
```

These share control-flow machinery but retain different command interpreters.

The channel/layer language includes, among other mechanisms:

- instrument/font selection
- volume and expression-like controls
- pan
- transposition
- frequency scaling
- pitch bend
- vibrato depth/rate/delay and gradients
- portamento
- ADSR/envelope selection
- reverb controls
- note allocation and priority
- calls, loops, branches, delays, and dynamic control flow

Therefore:

```text
USF effective ROM/state
!= flat note list
```

and:

```text
sequence layer
!= physical synthesis note
```

The interpreter itself must execute before the performed note stream is fully known.

### Logical note/layer state is distinct from allocated synthesis notes

OoT's playback layer keeps `SequenceLayer`, `SequenceChannel`, and allocated `Note` state separate.

A note can be reassigned, released, decayed, or disabled according to allocation/priority state while its parent layer remains a higher logical music object.

This independently supports the project's existing law:

```text
logical track/layer
!= physical voice allocation
!= persistent musical part
```

### Pitch continues transforming after the sequence note

OoT's playback code derives a sample-state frequency from the sequence-layer note frequency and then multiplies it by additional live transforms:

```text
layer note frequency scale
* vibrato frequency scale
* portamento frequency scale
* runtime resample-rate scaling
-> note sample-state resampling rate
```

The synthesis layer then converts that state into RSP audio commands and resampling work.

Thus an authored/sequence pitch event is neither the final physical resampling coordinate nor the final heard pitch trajectory.

### Spatial and effect state is also executed, not merely tagged

The runtime computes left/right gains from pan state and can apply:

- stereo pan curves
- “strong” left/right routing state
- headphone Haas delays
- reverb sends and ring-buffer processing
- filters and comb-filter parameters

These are source/runtime transformations with musical and perceptual consequences.

They must remain below later human descriptions such as “wide”, “distant”, “dry”, “echoing”, or “centered”.

### Same-work opportunity

The Ocarina USF corpus plus the matching decomp/reconstruction creates a high-value future experiment:

```text
USF effective ROM/state
-> locate game audio sequence/font/sample objects
-> execute or reconstruct OoT sequence/player state
-> follow one layer note
-> allocated Note
-> resampling / envelope / pan / reverb state
-> RSP synthesis command trajectory
```

This is stronger than using an unrelated open-source N64 engine as a mechanism observatory because the game identity matches.

However, matching game identity does not automatically prove that every decompiled source symbol or reconstructed type is historically authored source. Preserve the reconstruction provenance.

## PSF1 / PlayStation: physical voices can form causal graphs

The PSF1 corpus currently contains *Chrono Cross* and reconstructs exact PS-X EXE memory while correctly stopping before CPU/SPU execution.

DuckStation's SPU implementation shows why that stop is necessary.

### A PlayStation SPU voice is more than sample + pitch

The SPU exposes 24 voices, each with state including:

- left/right volume or autonomous volume sweep
- ADPCM sample-rate/pitch register
- ADPCM start and repeat addresses
- ADPCM block decode history
- ADSR parameters and live ADSR phase/level
- interpolation/sample counter state
- key-on/off state
- reverb participation
- optional noise mode

The realized voice therefore depends on state that evolves between CPU writes.

### Pitch modulation creates inter-voice causality

For SPU voice `n > 0`, pitch modulation can use the immediately preceding voice's live output amplitude to transform the current voice's sample step.

Conceptually:

```text
voice[n].programmed_sample_rate
* transform(voice[n-1].live_output)
-> voice[n].effective_sample_step(t)
```

Therefore:

```text
physical voice episode
```

is not always independent of neighboring physical voice episodes.

A platform execution graph may require causal edges between voices.

This is related to HuC6280's coupled LFO lesson but is not the same mechanism and must remain SPU-specific.

### Noise mode changes the source class

A voice can be configured to use the SPU noise generator rather than its ordinary ADPCM sample stream.

Thus:

```text
physical voice number
!= stable sample-source identity
```

A voice slot is a resource allocation coordinate. Its active source/synthesis mode is time-bearing state.

### Volume is also autonomous

Left/right voice volume registers can select sweep behavior rather than one fixed scalar, while ADSR independently evolves the voice's amplitude envelope.

So:

```text
volume register
!= instantaneous channel gain
```

and:

```text
ADSR parameters
!= ADSR trajectory
```

The same programmed-versus-evolved boundary established on Yamaha FM and periodic chips survives into sampled PlayStation audio.

### PSF1 consequence

The next PSF1 adapter should not jump from PS-X EXE memory to “24 stems”.

The stronger route is:

```text
CPU/driver state
-> SPU register/memory commands
-> evolving SPU voice/resource graph
-> decoded/interpolated voice trajectories
-> reverb/mix realization
-> higher note/part/instrument hypotheses
```

Chrono Cross can become the real-work control once the CPU/driver surface is available.

## 2SF / Nintendo DS: one physical channel can change synthesis class

The current 2SF corpus contains *Mario Kart DS* and reconstructs effective Nintendo DS ROM maps while correctly stopping before ARM/audio runtime execution.

melonDS and VGMTrans expose complementary parts of the DS audio stack.

### DS hardware channel modes are heterogeneous

melonDS models 16 SPU channels.

A channel's format/mode can select:

```text
PCM8
PCM16
IMA-ADPCM
PSG square wave
noise
```

with PSG/noise availability depending on the physical channel number.

Therefore:

```text
DS physical channel
!= stable synthesis type
```

and a common “sample voice” schema would already be wrong for the hardware.

### ADPCM playback has history

DS ADPCM playback maintains predictor and index state and restores saved predictor/index state at loop boundaries.

Consequently:

```text
sample address + timer
!= current decoded sample trajectory
```

without the decoder history and loop-state provenance.

### SSEQ exposes a much higher symbolic/control layer

VGMTrans's Nintendo DS parser treats SSEQ as a control-flow-bearing sequence language rather than a MIDI file.

Observed commands include:

- note + velocity + duration
- rest
- program change
- open track
- jump and call
- random values and variables
- conditional logic
- pan and volume
- transpose
- pitch bend and bend range
- priority
- note-wait/tie state
- portamento control/time
- modulation depth/speed/type/range/delay
- attack, decay, sustain, release controls
- expression
- tempo
- sweep pitch
- loops and returns

The parser even documents a variable-operation command observed in a Mario Kart DS sequence, demonstrating that this is not merely theoretical format surface.

### SDAT binds sequence, bank, and wave archives

VGMTrans's SDAT scanner reconstructs relations among:

```text
SSEQ sequence
-> SBNK instrument bank
-> up to several SWAR wave archives
```

and then attaches those objects into one collection.

That suggests a powerful bounded test for the existing Mario Kart DS 2SF corpus:

```text
2SF effective NDS ROM map
-> locate SDAT
-> recover SSEQ/SBNK/SWAR dependency graph
-> select one sequence event
-> resolve program/instrument/sample or PSG source
-> execute modulation/envelope/control flow
-> map to DS SPU channel allocation
-> physical sample/PSG/noise trajectory
```

This could produce a source-native vertical slice without first needing a complete general NDS emulator inside Game Music Interpreter.

VGMTrans remains a mechanism/parser observatory. Its MIDI-like projections must not become canonical truth, and unknown/partially handled opcodes must remain explicit evidence gaps.

## Cross-platform result

The xSF family does not reveal one new universal voice object.

It reveals three different runtime geometries.

### Nintendo 64

```text
sequence
-> channel
-> layer
-> allocated Note
-> sample/resample/envelope/pan/effects
-> RSP synthesis
```

### PlayStation

```text
CPU/driver commands
-> 24 physical SPU resources
-> ADPCM/noise source + autonomous ADSR/volume state
-> optional inter-voice pitch-modulation edges
-> reverb/mix
```

### Nintendo DS

```text
SSEQ control program
-> bank/wave dependencies
-> runtime logical note state
-> 16 physical channels with PCM/ADPCM/PSG/noise modes
-> hardware mix/capture
```

The common invariant is smaller and stronger:

```text
source/runtime musical object
!= physical synthesis resource
!= acoustic trajectory
!= persistent musical part
```

Preserve the transformations instead of flattening the nodes.

## Literature connection

NES-MDB independently separates composition from expressive performance and renders the latter through emulation rather than treating a score as the acoustic result.

That aligns with the interpreter's vertical architecture:

```text
symbolic/source state
-> performance state
-> synthesis state
```

But the newer xSF evidence shows that those layers may themselves contain several source-specific sublayers, allocation steps, and causal graphs.

NES-VMDB demonstrates the usefulness of symbolic game-music representations at scale, but its downstream MIDI representation is a task projection rather than a reason to make MIDI canonical here.

## Historical interpretation guardrail

Collins and Burke both emphasize that hardware/software constraints shaped compositional decisions and platform-specific musical styles.

Therefore source-native enhancement must not use the rule:

```text
hardware limitation -> remove it
```

The safer rule is:

```text
identify the transformation
-> determine whether evidence supports implementation ceiling, authored technique, or ambiguity
-> preserve identity-critical behavior
-> relax only constraints whose removal does not rewrite the work
```

This matters especially when driver/source evidence shows that a composer deliberately composed into a limitation or exploited a hardware behavior as an instrument.

Historical evidence that composers disliked some workflow or memory limitations does not license erasing every hardware affordance.

## Highest-information next experiments

### 1. Ocarina USF same-work vertical slice

This is now the strongest immediate xSF experiment.

For one bounded cue:

```text
USF effective object
-> sequence identity
-> channel/layer command trajectory
-> note allocation
-> live vibrato/portamento/ADSR
-> resampling state
-> synthesis command trajectory
```

Mark exactly where the current USF snapshot/runtime bridge becomes incomplete.

### 2. Mario Kart DS SDAT recovery

Search the effective 2SF ROM object for SDAT and recover the exact SSEQ/SBNK/SWAR graph.

Do not execute unknown sequence commands speculatively.

If the exact Mario Kart DS sequence represented by the mini2sf can be resolved, this becomes a second source-native vertical slice with a very different hardware model.

### 3. Chrono Cross SPU runtime trace

Once PSF1 CPU execution exists, capture:

```text
SPU writes
voice source mode
sample/repeat address
programmed pitch
live pitch-modulated step
ADSR phase/level
left/right sweep level
reverb participation
```

Explicitly test for intervals where physical voice state evolves without a direct CPU write at that moment.

### 4. Cross-platform note/voice allocation test

Compare:

```text
OoT logical layer -> allocated Note
NDS sequence track -> SPU channel
PS1 driver/event -> SPU voice
```

The goal is not one allocation algorithm. The goal is a common evidence contract for documenting temporary physical-resource ownership without confusing it with a persistent musical part.

## Stop conditions

Stop rather than guess if:

- a USF snapshot is flattened directly to notes without executing sequence/player logic;
- OoT sequence layer identity is promoted to physical Note identity;
- a PlayStation SPU voice is assumed independent while pitch modulation is enabled;
- a PlayStation voice number is treated as stable sample identity while noise mode can replace its source;
- a 2SF effective ROM is called an SDAT/SSEQ source before the archive has actually been located;
- a DS physical channel is assigned one permanent synthesis type;
- VGMTrans's MIDI projection overwrites unknown or control-flow-bearing SSEQ evidence;
- a decompilation/reconstruction is called original authored source without provenance;
- enhancement removes a hardware behavior merely because it is a limitation rather than demonstrating that it is non-identity-critical.

Correction outranks coherence.
