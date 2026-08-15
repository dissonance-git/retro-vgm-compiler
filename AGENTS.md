# AGENTS.md

## Entrance sequence

For substantive work, use this bounded entry path:

```text
current main HEAD
→ README.md
→ AGENTS.md at the same HEAD
→ recent commits
→ task-relevant source family / research frontier
→ exact target code, test, or document
```

1. Resolve `dissonance-git/game-music-interpreter` and record current `main`.
2. Read `README.md` and this file from the same commit.
3. Inspect recent commits before choosing the smallest relevant work area.
4. Before a replacement write, re-fetch current `main` and the exact target file. Use its current blob SHA.
5. Preserve unrelated concurrent work. Never replace a file from a cached older copy.
6. Work on `main`; do not create branches or PRs unless explicitly requested.
7. Never force-push.
8. After publication, verify the resulting commit and report code/tests, CI, reference parity, research evidence, and listening validation as separate evidence states.

Direct user correction outranks project prose.

## Project scope

Game Music Interpreter owns executable understanding, musical analysis, and source-native rendering of digital game music across the full vertical route from encoded data to listener-level musical understanding.

It may own:

- bit/byte, format, container, memory-map, and object parsers;
- tracker, MML, and other authored symbolic inputs;
- driver and sequence models;
- device/chip models;
- reference and enhanced synthesis;
- source, performance, device, sample, and routing state;
- provenance-aware musical analysis;
- listener-level musical organization grounded in lower evidence;
- deterministic fixtures and corpus tooling;
- playback/front-end bridges.

VGM/VGZ and SPC are important source families, not the project ontology.

## Project boundaries

```text
Helix
  shared research execution, evidence discipline, project continuity

Game Music Interpreter
  game-music source, driver, device, performance, analysis, rendering

libaural
  general artificial hearing

Omniphony
  general headphone spatial rendering
```

Chip-specific machinery stays here unless it becomes genuinely general. Peer-project instructions apply only when work actually crosses that boundary.

Sonic 3 attribution is a bounded adversarial case because it depends on SMPS, YM2612, PSG, DAC, prototype/final, and arrangement evidence. Technical similarity must never become automatic authorship proof.

## Core execution law

Different sources require different ingestion and execution machinery. Normalize meaning only after the source-specific semantics have been respected.

The project must remain vertically traceable across all materially available layers:

```text
physical encoding / exact bits and bytes
        ↓
format / object / memory semantics
        ↓
source-specific representation
        ↓
source-specific parser / compiler / executor
        ↓
program and driver execution
        ↓
exact device / synthesis / routing state
        ↓
performed musical gestures and persistent parts
        ↓
acoustic realization
        ↓
auditory organization
        ↓
listener musical model of the song
        ↓
analysis / discourse / response / forensics
```

No layer is allowed to impersonate another. Raw bytes are not automatically notes. Registers are not automatically musical parts. PCM is not automatically auditory organization. An auditory stream is not automatically a melody, accompaniment role, section, cadence, or song-form interpretation. A listener response is not the same thing as a listener's musical understanding.

Do not make MIDI, PCM, stems, notation, chord sequences, or prose summaries the canonical representation.

Keep source evidence attached so higher reasoning can descend to exact bits, bytes, addresses, commands, registers, samples, driver events, MML/tracker commands, device state, acoustic contributions, auditory evidence, or documentary sources. A higher claim may compress its support, but it must not sever the route back down.

Likewise, lower layers should expose sufficient causal structure to support higher reasoning where possible. Do not stop at technically exact machine state when the task is to understand the music as heard, and do not fabricate a higher layer when the required bridge has not been established.

## Identity and evidence law

Do not equate implementation coordinates with musical identity.

```text
physical slot
!= bounded voice episode
!= persistent musical part
!= auditory stream
!= listener-assigned musical role
```

```text
register frequency
!= nominal frequency
!= programmed pitch
!= transposed pitch
!= frequency displacement
!= performed pitch
!= heard pitch
!= note spelling
```

```text
source / implementation fingerprint
!= composition fingerprint
!= authorship proof
```

```text
listener musical understanding
!= listener response
```

Mappings may be one-to-one, one-to-many, many-to-one, or time-varying.

Every higher inference needs provenance and an evidence status appropriate to the layer that actually supports it. Preserve meaningful alternatives when the evidence does not separate them.

## Source-family rules

### Authored symbolic / programming source

MML, trackers, score-like data, and source code can provide explicit notes, rests, effects, loops, macros, instrument references, articulation, modulation, and logical-part identity. Preserve the exact dialect/toolchain. A source note or effect token can be exact while the realized performance still requires stateful execution.

### Driver / sequence data

Known drivers such as SMPS, GEMS, N-SPC, MDX, PMD, and other identified engines can expose logical tracks, performance events, instruments, modulation, loops, and allocation policy.

### Logged execution

VGM/VGZ can strongly establish command order/timing, device configuration, register state, and embedded data. It does not automatically reveal the original score, tracker, driver track, or authoring grammar.

Format semantics come from the format specification; chip semantics come from device documentation and validated implementations.

### Executable snapshots / rips

SPC, NSF/NSFe, HES, KSS, PSF-family and related objects may preserve CPU/program state, RAM, driver code/data, samples, and live device state. Their semantic altitude must be discovered rather than inferred from the extension.

### Symbolic performance formats

MIDI may preserve notes, controllers, bends, programs, banks, and SysEx. The target module/synth remains part of the realization.

## Realtime law

Realtime players must not require whole-song reverse compilation before playback. Small causal state, streaming analysis, and bounded lookahead are allowed when justified.

Offline forensic/research tools are allowed but must not become hidden playback prerequisites.

## Accuracy and enhancement

The reference renderer is the scientific control, not the quality ceiling.

Enhanced rendering may relax a specific historical implementation ceiling only when the same musical identity survives. Preserve:

- notes and timing relationships;
- groove and phrasing;
- logical part relationships;
- patch/sample/instrument identity;
- programmed articulation, modulation, and automation;
- deliberate effects and structural density;
- device behavior that became part of an instrument's identity.

Candidate improvements include better interpolation, bandwidth, numerical precision, sample reconstruction, DAC realization, summation/headroom, or source-aware effects when independently validated.

Every audible change needs reference-vs-enhanced comparison. Change one meaningful variable at a time where possible.

## Source-domain first

Prefer improving the known source object before repairing the final stereo bus.

Examples:

- improve a PCM voice before summation rather than EQ the whole mix;
- reconstruct a DAC event at its trigger rather than use a generic transient shaper;
- render an FM patch from live operator/register state rather than apply a stereo exciter;
- preserve dry/effect distinctions and persistent driver identity when available.

Generic bus processing is a fallback, not the design center.

## Shared-core rule

A mechanism becomes shared only when materially different source families genuinely require the same abstraction and sharing does not erase useful source information.

Good candidates include exact event timing, provenance structures, persistent source identifiers, diagnostics, high-quality resampling, source-aware headroom/mixing primitives, and common performance-event/trajectory objects.

BRR reconstruction stays SNES-specific. FM operator rendering stays FM-specific. QSound behavior stays QSound-specific. Driver allocation and MML/tracker grammar remain source-specific until evidence earns a generalization.

> **Shared abstractions should be discovered by agreement and disagreement.**

## Upstream and research policy

Mature repositories, manuals, source releases, reverse-engineered implementations, and literature are observatories before they are dependencies.

Use evidence roles explicitly:

```text
format spec → file semantics
official manual → documented hardware/platform behavior
validated emulator/device core → implementation and undocumented behavior
identified driver/source → actual software semantics
real corpus → observed preserved execution
literature → pressure-test general analytical claims
```

Pin repository commits when behavior matters. Record source provenance. Respect licenses. Do not churn imported upstream bytes or line endings.

A research pass should end in one of: an executable mechanism, a regression, a bounded negative result, a stronger evidence rule, or a clearly stated next discriminating test.

## Corpus law

`tests/corpus/` is permanent experimental apparatus.

- Source files are immutable once admitted unless provenance is proven wrong.
- Hashes and manifest metadata must remain reproducible.
- New controls should add orthogonal information, not raw volume.
- Related-version or same-work controls must state exactly what equivalence is and is not claimed.
- A valid file is not automatically an authentic or complete execution witness.

The corpus should contain both confirmations and counterexamples for proposed abstractions.

## Testing

Core tests are registered through CMake. Use strict warnings where practical.

Important mechanisms need more than synthetic unit tests. Use whichever controls fit the claim:

- deterministic invariants;
- real corpus fixtures;
- negative controls;
- independent implementations;
- official documentation;
- source→driver→device forward controls;
- paired preserved representations;
- reference-vs-enhanced captures;
- auditory/perceptual controls where the claim crosses into heard organization;
- listening tests for perceptual quality and listener-level musical interpretation where appropriate.

Do not call CI green unless the runner actually executed successfully. A blocked runner is not a pass or a failure of the code.

## Current analytical frontier

The project already has device-specific nominal-pitch mechanics, source/driver boundaries, provenance-aware analysis features, persistent-part hypotheses, musical-dependency regressions, harmonic/formal evidence rules, discourse projections, and a broad cross-architecture corpus.

The next high-information work is time-bearing performance reconstruction across non-Genesis controls, followed by explicit bridges into heard musical organization:

1. recover device-native pitch/voice/sample trajectories;
2. separate physical episodes from persistent parts;
3. compare executable-rip and downstream-trace representations where a same-work control exists;
4. build bounded source/driver adapters that can be followed forward into device state;
5. push higher into acoustic contribution and auditory organization without discarding source identity;
6. recover listener-level roles, phrases, sections, form, tension/release, and other song-level relations only where the lower evidence supports them;
7. test whether natural human musical descriptions can be generated from that vertical evidence path without reducing them to one-feature phrase rules;
8. only then strengthen style and attribution claims on those controls.

## Write discipline

Before each GitHub replacement write:

```text
current main
→ exact target at that main
→ current blob SHA
→ smallest coherent edit
→ publish to main
→ verify resulting commit
```

Do not rewrite unrelated files for style. Do not modify immutable corpus source bytes during documentation or architecture work. Corrections outrank narrative consistency.
