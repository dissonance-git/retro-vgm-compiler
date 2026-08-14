# Runtime synthesis-resource repurposing

## Status

Cross-device research input for physical-resource identity, synthesis topology, mode transitions, and device-state interpretation.

## Central correction

A numbered physical channel/slot can remain the same hardware resource while the **kind of synthesis role it performs changes at runtime**.

Therefore:

```text
physical resource identity
!= immutable synthesis role
```

and:

```text
channel number
!= stable voice type
```

The common representation should preserve stable hardware identity separately from time-bearing synthesis mode/topology.

## 1. YM2612 / OPN2 DAC mode

Pinned observatory:

- `nukeykt/Nuked-OPN2`
- commit `335747d78cb0abbc3b55b004e62dad9763140115`
- `ym3438.c`

Registers:

```text
0x2A -> DAC data
0x2B -> DAC enable
```

Nuked OPN2 records `0x2B` bit 7 as DAC enable.

At the channel-6 output stage, when DAC is enabled, the normal locked FM-channel output is replaced with the DAC sample value.

Conceptually:

```text
DAC disabled:
physical channel 6 output <- FM operator graph

DAC enabled:
physical channel 6 output <- DAC data path
```

The underlying FM operator/register state can still exist, but it is no longer the source of the channel's emitted output while the DAC path is selected.

Thus:

```text
YM2612 channel 6
!= permanently an FM voice
```

and a register analyzer must not infer audible FM activity from channel-6 operator state alone while DAC mode is active.

## 2. YM2413 / OPLL rhythm mode

Pinned observatory:

- `nukeykt/Nuked-OPLL`
- commit `1269cf5a783b65583b50fa2464d08be75830aaa0`
- `opll.c`

The OPLL rhythm register controls a rhythm-mode enable plus individual percussion keys.

Nuked OPLL explicitly tracks rhythm-resource selections for:

```text
bass drum operator 0
hi-hat
tom
bass drum operator 1
snare drum
top cymbal
```

and applies drum-specific key behavior and patch/level selection when those resources are active.

The important result is structural:

```text
upper melodic channel/operator resources
-> rhythm mode
-> percussion-specific resource grouping/semantics
```

This is more than `same oscillator, different preset`.

Operator grouping, key semantics, and signal-generation rules change with the device mode.

Some OPLL-family variants can make rhythm mode mandatory rather than switchable, which is further evidence that `YM2413-like hardware` does not imply one universal melodic-channel schema.

## 3. YMF262 / OPL3 rhythm mode

Pinned observatory:

- `nukeykt/Nuked-OPL3`
- commit `cfedb09efc03f1d7b5fc1f04dd449d77d8c49d50`
- `opl3.c`

Nuked OPL3 makes the topology change explicit in its own runtime type field.

Its channel types include:

```text
ch_2op
ch_4op
ch_4op2
ch_drum
```

When rhythm mode is enabled, channels 6 through 8 are assigned `ch_drum` and their algorithm/modulation paths are reconfigured for percussion.

When rhythm mode is disabled, channels 6 through 8 return to `ch_2op` and drum-envelope keys are released.

Therefore the exact same physical channel number can undergo:

```text
2-op melodic resource
-> drum resource
-> 2-op melodic resource
```

over one execution.

OPL3's independent 4-op pairing mode supplies another form of resource repurposing:

```text
two independent 2-op channel resources
-> one linked 4-op synthesis topology
-> two independent 2-op resources
```

See `research/fm-operator-trajectory-semantics.md` for the operator-graph detail.

## 4. Mode changes are time-bearing topology

The correct lower representation is not:

```text
slot 6: type=FM
```

but closer to:

```text
physical slot 6
  interval A -> participates in synthesis topology X
  interval B -> participates in synthesis topology Y
  interval C -> returns to topology X
```

The physical identity can remain stable while its active synthesis object and relations change.

Useful exact coordinates can include:

```text
physical resource ID
mode-control write/state
active synthesis role
active grouping/topology
role transition time
output-source selection
key/envelope semantics under that mode
```

Device-specific adapters remain responsible for the numeric and register law.

## 5. Existing common graph already survives this test

`model/musical_execution_graph.h` already separates:

- `physical_slot`;
- `synthesis_object`;
- time-bounded nodes/edges;
- `occupies`, `controls`, `routes_to`, `transforms`, and causal relationships.

No new common node kind is earned.

A source-specific adapter can model:

```text
physical slot
<- occupies / realizes ->
time-bounded synthesis object
```

and replace/reconfigure that synthesis object when mode state changes.

Do not encode one immutable `resource_type` into physical-slot identity.

## 6. Activity inference must be mode-aware

A raw register log can contain apparently active state that is not the active realization path.

Example:

```text
YM2612 channel-6 FM registers contain a valid patch + frequency
DAC enable = on
```

The FM state exists, but channel output is currently supplied by the DAC path.

Similarly, an OPL channel's ordinary melodic operator registers may still contain values while rhythm mode changes how the resources are keyed/routed.

Thus:

```text
configured state
!= selected realization path
```

Any `voice active` or `instrument sounding` claim must consider the current synthesis mode.

## 7. Persistent musical identity must not follow physical role blindly

A driver may use one physical channel for:

```text
melodic FM
-> sampled drum hit via DAC
-> melodic FM again
```

That does not imply one persistent musical part transformed continuously from melody into percussion.

Conversely, one persistent musical part may migrate between differently configured resources.

Keep separate:

```text
physical slot continuity
synthesis-role continuity
logical-note/driver continuity
persistent musical-part continuity
```

## 8. Instrument identity consequence

A static statement such as:

```text
channel 7 uses patch P
```

is incomplete if channel 7 is currently a drum resource whose patch/routing semantics are mode-defined.

Instrument interpretation should descend through:

```text
current device mode
-> active resource topology
-> programmed parameters used by that topology
-> performed synthesis trajectory
-> timbral evidence
-> instrument/role hypothesis
```

not simply:

```text
register bank -> patch name
```

## 9. Stem/isolation consequence

A stem selector based on fixed physical channel labels can change meaning mid-cue.

For example:

```text
"OPL channel 7 stem"
```

may contain a melodic 2-op channel in one interval and a percussion component in another.

That can still be a valid **physical-resource stem**, but it is not automatically one musical stem.

Label the coordinate honestly.

## 10. Real-corpus test surface

The permanent corpus already contains useful controls:

- `sonic-3-ym2612-psg` for YM2612 DAC/FM competition;
- `disc-station-ym2413` for OPLL rhythm mode;
- `bucket-relay-champ-ymf262` for OPL3 rhythm and 4-op topology;
- `truxton-ym3812` and other OPL-family fixtures as generalization controls.

The existing `tools/vgm_fm_mode_audit.py` already records YMF262 rhythm-mode and 4-op/new-mode presence.

A later audit extension should add time-bearing transitions for at least:

```text
YM2612 DAC enable/disable
YM2413 rhythm enable/disable
YMF262 rhythm enable/disable
```

rather than only `ever seen` booleans.

High-information assertions:

1. a mode transition changes the active synthesis-role interpretation while physical resource identity remains stable;
2. configured but bypassed FM state is not reported as current channel output during YM2612 DAC mode;
3. OPL/OPLL rhythm mode changes resource grouping/role rather than merely instrument name;
4. disabling the mode restores the ordinary topology without inventing a new physical slot identity.

## 11. Broader law

The result combines with earlier research:

```text
physical slot != voice episode != persistent part
```

and strengthens it to:

```text
physical slot
!= immutable synthesis object
```

The hardware slot is a resource coordinate. What synthesis process occupies or uses it is part of the time-bearing device state.

## Stop conditions

Stop rather than overclaim if:

- a physical channel is assigned one immutable synthesis type for the whole file;
- YM2612 channel-6 FM registers are treated as audible FM while DAC mode selects the DAC output path;
- OPL/OPLL rhythm mode is represented as a cosmetic patch change;
- a 2-op/4-op/drum transition creates a fake new physical slot instead of a new time-bounded topology;
- physical-slot continuity is promoted to persistent musical-part continuity;
- a physical-channel stem is called one musical instrument when its synthesis role changes during the cue;
- mode-specific semantics are flattened into a universal `channel_type` enum with identical numeric behavior across chips.

Correction outranks coherence.
