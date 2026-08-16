# AGENTS.md

## Entrance sequence

For substantive work, use this bounded entry path:

```text
current main HEAD
→ README.md
→ AGENTS.md at the same HEAD
→ docs/retro-vgm-compiler-roadmap.md
→ recent commits
→ task-relevant source family / research frontier
→ exact target code, test, or document
```

1. Resolve the Retro VGM Compiler repository and record current `main`.
2. Read `README.md`, this file, and `docs/retro-vgm-compiler-roadmap.md` from the same commit.
3. Inspect recent commits before choosing the smallest relevant work area.
4. Before a replacement write, re-fetch current `main` and the exact target file. Use its current blob SHA.
5. Preserve unrelated concurrent work. Never replace a file from a cached older copy.
6. Work on `main`; do not create branches or PRs unless explicitly requested.
7. Never force-push.
8. After publication, verify the resulting commit and report code/tests, CI, reference parity, research evidence, and listening validation as separate evidence states.

Direct user correction outranks project prose.

## Project scope

Retro VGM Compiler exists primarily to build holistic musical understanding of digital game soundtracks. Executable understanding, source recovery, device/chip modeling, perceptual organization, provenance, and source-native rendering are supporting capabilities that make that musical understanding deeper, more accurate, more specific, and more defensible.

The ideal system should be able to discuss a game's soundtrack with the integrated understanding of a strong critic, composer/musician, producer, and musicologist: composition, arrangement, harmony, rhythm, melody, form, timbre, sound design, dramatic/game function, thematic relations, soundtrack-scale identity, stylistic lineage, and meaningful exceptions. It should seem to have internalized the musical logic of the soundtrack from the inside without inventing undocumented creator intent.

See `docs/holistic-soundtrack-understanding.md` and `docs/retro-vgm-compiler-roadmap.md`.

It may own:

- bit/byte, format, container, memory-map, and object parsers;
- tracker, MML, and other authored symbolic inputs;
- driver and sequence models;
- device/chip models;
- reference and enhanced synthesis;
- source, performance, device, sample, and routing state;
- provenance-aware musical analysis;
- listener-level musical organization grounded in lower evidence;
- soundtrack-scale composition, arrangement, thematic, dramatic, stylistic, and critical models;
- deterministic fixtures and corpus tooling;
- playback/front-end bridges.

VGM/VGZ and SPC are important source families, not the project ontology.

## North-star priority

The top layer is the objective. The lower layers are microscopes and support structures.

```text
PRIMARY
holistic understanding of the soundtrack as a musical world

SECONDARY BUT OFTEN NECESSARY
track/part/event analysis
perceptual organization
source / driver / device reconstruction
exact technical provenance
rendering and tooling
```

Do not measure progress mainly by how much implementation state has been decoded. Measure whether the work materially improves the system's ability to understand a soundtrack, a track's musical logic, a relation among tracks, a compositional/arranging habit, a dramatic function, or a meaningful ambiguity.

A low-level investigation is primary research when it discriminates among important musical hypotheses. Examples include proving that an apparent delay is programmed counterpoint, recovering persistent parts that reveal voice leading, distinguishing authored articulation from an emulator artifact, or separating composition from implementation in an attribution problem.

A technically exact result that does not improve the musical model may still be valuable infrastructure, but it is supporting work rather than the project destination.

`explain why these bytes became this musical moment` is therefore an optional depth capability. It is powerful evidence descent, not the headline product.

## Project boundaries

```text
Helix
  shared research execution, evidence discipline, project continuity

Retro VGM Compiler
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
track and soundtrack-scale analysis / discourse / response / forensics
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

This rule governs implementation work. It does not mean bottom-up technical work outranks soundtrack-level understanding when choosing research priorities.

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

A research pass should end in one of: an improved musical model, a discriminating soundtrack/track analysis, an executable mechanism, a regression, a bounded negative result, a stronger evidence rule, or a clearly stated next discriminating test.

## Corpus law

`tests/corpus/` is permanent experimental apparatus.

- Source files are immutable once admitted unless provenance is proven wrong.
- Hashes and manifest metadata must remain reproducible.
- New controls should add orthogonal information, not raw volume.
- Related-version or same-work controls must state exactly what equivalence is and is not claimed.
- A valid file is not automatically an authentic or complete execution witness.

The corpus should contain both confirmations and counterexamples for proposed abstractions.

The corpus should also support soundtrack-level tests: multiple cues from the same game, contrasting cue functions, thematic relations, transformed reprises, stylistic exceptions, and where possible documentary/contextual controls.

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
- listening tests for perceptual quality and listener-level musical interpretation where appropriate;
- whole-soundtrack reviews tested for specificity, cross-track coherence, explanatory depth, competing interpretations, and evidence descent;
- counterfactual musical questions that test whether the model learned the soundtrack's own grammar rather than generic genre clichés.

Do not call CI green unless the runner actually executed successfully. A blocked runner is not a pass or a failure of the code.

## Current analytical frontier

The project already has substantial lower infrastructure: device-specific nominal-pitch mechanics, source/driver boundaries, provenance-aware analysis features, persistent-part hypotheses, musical-dependency regressions, harmonic/formal evidence rules, discourse projections, source-native rendering experiments, and a broad cross-architecture corpus.

The current implementation roadmap is maintained in `docs/retro-vgm-compiler-roadmap.md`. The next high-information frontier is to make those capabilities converge upward into holistic soundtrack models rather than continuing to expand the lower stack indefinitely.

Priority order:

1. choose representative complete or near-complete soundtrack cases with multiple cue functions and enough audio/source/context to support serious analysis;
2. produce a whole-score model first: musical identity, track families, contrasts, recurring grammar, thematic/timbral networks, dramatic/game function, stylistic lineage and meaningful exceptions;
3. pressure-test that model on representative individual cues using melody, rhythm, bass, harmony, form, orchestration, sound design, production and loop/game-function analysis together;
4. test cross-track reasoning: transformed motifs, shared schemas, arrangement habits, soundtrack-scale pacing, reprises and changed contextual meaning;
5. test counterfactual understanding: plausible continuations, variations, substitutions and transformations under the soundtrack's own musical grammar;
6. descend into source/driver/device/perceptual machinery only where it resolves a meaningful musical ambiguity, strengthens a higher claim, or provides an unusually strong explanatory bridge;
7. keep lower infrastructure moving where necessary for missing formats and source-native rendering, but do not let technical completeness substitute for musical understanding;
8. evaluate the final result as a critic/musicologist/composer-level account of the soundtrack, with technical evidence available underneath rather than dominating the surface.

The default question is no longer `what lower layer can we decode next?` It is `what prevents us from understanding this soundtrack more completely?`

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
