# Ludic-function evidence for game-music interpretation

## Why this belongs in the interpreter

The project should not stop at reconstructing notes, instruments, channels, or even musical form. Game music also participates in play. It can announce state, shape perceived affordances, stabilize a place, identify a character, bridge failure and restart, regulate tension, react to player action, or manage a transition.

Those are useful analytical claims, but they are also easy to overstate. A loop that sounds like an area theme is not proof that it functions as an area theme. A rising-density passage is not proof that the game is increasing danger. The interpreter therefore needs a machine-readable layer that can express ludic-function hypotheses while retaining the evidence route and uncertainty behind them.

The intended chain is:

```text
native format / source artifact
    -> driver and engine execution
    -> synthesis and performance evidence
    -> normalized musical structure
    -> soundtrack-relational evidence
    -> ludic-function hypothesis
    -> human musical explanation
```

This keeps the original project direction intact. Native-format decoding, chip/engine semantics, composition and arrangement inference, soundtrack-level interpretation, and human-readable explanation are one pipeline rather than separate projects.

## Literature-derived analytical lenses

### Music can structure gameplay situations and affordances

Michiel Kamp, "Musical Ecologies in Video Games" (2014), uses an ecological/affordance framework to argue that game music can help structure how players perceive goal-oriented situations. His examples include both situations inside the game world, such as enemy appearance, and situations beyond a single world-state, such as death and level restart.

Interpreter consequence: `affordance_cue`, `game_state_signal`, `failure_reset`, and `transition_management` are legitimate hypotheses, but they require contextual evidence before they can become strong claims.

### Loops and repetition are not merely formal statistics

Julianne Grasso, "Ludomusical Narrativity" (2024), treats musical form as one means by which ludic and narrative experience can be reconciled. Repetition and looping can contribute to the narrativization of play rather than simply producing timeless stasis.

Mathew Arnold, "Inside the Loop: The Audio Functionality of Inside" (2018), examines looping audio across gameplay, death, and respawn, including the idea of musical continuity or "suture" across those state boundaries.

Interpreter consequence: loop topology should feed higher-level questions such as continuity across failure/restart, episodic boundaries, and narrative framing. `loop_count` or `loop_start` alone is not the endpoint.

### Players can hear music as game information

Marlous Kamp, *Four Ways of Hearing Video Game Music* (2023), distinguishes background, aesthetic, ludic, and semiotic modes of hearing. In the semiotic mode, music may be heard as a signal conveying information about game state; ludic hearing emphasizes playing along or responding to the music.

Karen Collins, *Playing with Sound* (2013), develops an interaction-centered account of game sound from the player's perspective, emphasizing embodied and multimodal interaction rather than passive listening.

Interpreter consequence: distinguish music that merely correlates with a state from music that plausibly functions as information or action coupling. Runtime timing evidence is especially valuable here.

### Place, time, and exploration matter

Ailbhe Warde-Brown, "Waltzing on Rooftops and Cobblestones" (2021), combines musical-immersion work with player-involvement theory to emphasize spatiotemporal involvement in open-world play.

Interpreter consequence: `place_identity` should not be reduced to a timbral label such as "ambient". Evidence may include recurrence by location, transition behavior at boundaries, soundtrack-family relations, persistence during exploration, and documented game context.

### Adaptive music requires game context and musical behavior together

Patrick Hutchings and Jon McCormack, "Adaptive Music Composition for Games" (2020), argue that adaptive systems need to model player action, game-world context, and emotion alongside the music-generation system. Recent literature surveys of adaptive game music continue to distinguish horizontal resequencing, vertical layering, dynamic mixing, and algorithmic adaptation as recurring implementation strategies.

Interpreter consequence: transition and action-coupling analysis should represent both sides of the relation. A musical change without a game-state witness is a musical event; a game-state change without a musical witness is context; their measured relation is the evidence for adaptation.

## Executable ontology

The first implementation lives in `model/ludic_function_hypothesis.h` and uses the existing `semantic_layer::musicological_context` layer.

Initial function vocabulary:

- `game_state_signal`
- `affordance_cue`
- `place_identity`
- `character_identity`
- `narrative_frame`
- `tension_regulation`
- `continuity_bridge`
- `action_coupling`
- `transition_management`
- `reward_feedback`
- `failure_reset`
- `menu_meta`

This vocabulary should remain extensible. It is not intended as a final taxonomy of all game-music functions.

### Evidence origins

The model explicitly separates:

1. `musical_intrinsic`
   - harmony, rhythm, texture, form, loop topology, density, cadence, orchestration, register, motif, and similar musical evidence.

2. `sequence_or_engine`
   - authored sequence behavior, driver state, branch/loop instructions, layer enablement, engine parameters, channel allocation, synthesis state, and timing obtained from native formats or emulation.

3. `soundtrack_relational`
   - motif recurrence across cues, cue families, transformation relationships, shared instrumentation, harmonic fingerprints, alternate arrangements, and other corpus-level relationships.

4. `runtime_game_context`
   - measured game-state transitions, player actions, locations, battle state, failure/restart, menus, cutscenes, or other instrumented runtime context aligned with the music.

5. `external_annotation`
   - official cue sheets, source code labels, developer documentation, interviews, reliable metadata, or other explicit documentary evidence.

Evidence status and evidence origin are orthogonal. An exact SPU register write is exact evidence about synthesis execution; it is not exact evidence that the cue represents a character.

### Confidence guardrail

A ludic claim supported only by musical, engine, or soundtrack-relational evidence is allowed to exist, but its confidence is capped below the project's strong-claim range. A runtime-game-context or explicit documentary witness is required to remove that ceiling.

This is a policy boundary, not an empirical probability calibration. The current intrinsic-only ceiling is `0.64` and should be revised only with an explicit reason and tests.

Counterevidence remains first-class. The graph records support and counterevidence as separate `derived_from` edges with explicit polarity. It should not disappear into one opaque score.

## How native formats feed this layer

### USF / Nintendo 64

USF-style emulation and projects in the lazyusf/lazyusf2 lineage are valuable primarily as execution witnesses. They can expose the state needed to reconstruct what the N64 audio program actually did at runtime. For this project, that evidence should flow upward into voice identity, sequence behavior, timing, loop topology, instrumentation, and arrangement before supporting contextual hypotheses.

Ocarina of Time remains the preferred USF research case. Useful questions include:

- which musical identities persist across location/state variants,
- whether layers or transitions are authored as distinct sequence behavior,
- how engine-level state changes correspond to audible form,
- where recurring material creates place, danger, puzzle, or character relations,
- which claims can be established from sequence/engine evidence and which still require gameplay context.

### PSF / PlayStation

The repository already contains PSF/AKAO probes and Chrono Cross-specific allocator tests. Chrono Cross should remain the preferred PSF case because its soundtrack gives us a rich test bed for orchestration, cue-family relations, motif transformation, arrangement identity, and context-sensitive interpretation.

The important next step is not merely broader AKAO byte coverage. It is linking decoded/observed PSF behavior to normalized musical evidence and then asking what higher-level soundtrack relations become answerable.

### 2SF / Nintendo DS

2SF should follow the same evidence contract. The useful output is not "2SF supported" as a badge; it is a set of trustworthy authored/runtime observations that can be compared with PSF, USF, SPC, VGM, SMPS, and GEMS-derived evidence.

Priorities include sequence identity, instrument/sample mapping, voice allocation, loop/form topology, dynamic layer behavior, and timing relationships that survive normalization.

### SMPS and GEMS

SMPS and GEMS are particularly valuable because driver/source understanding can expose authoring decisions that a rendered capture hides. Programmed pitch, channel priorities, rest/hold behavior, envelopes, modulation, DAC use, and driver-level branching can all improve composition/arrangement inference.

Their role in this layer is to provide stronger lower-level evidence, not to make Genesis-specific ludic heuristics.

## GitHub research posture

External codebases are evidence suppliers and implementation references, not replacement architectures.

- `vgmtrans/vgmtrans` is useful as a broad reference for recovering sequence and instrument structure from game formats.
- the `lazyusf` / `lazyusf2` family is useful for N64/USF execution behavior.
- format-specific players, emulators, and decoders should be mined for exact state semantics and edge cases where they expose information the current adapters do not.
- source code for SMPS and GEMS remains unusually valuable because it can establish driver semantics directly rather than forcing them to be guessed from captures.

The project should import the smallest trustworthy mechanism or fact needed, preserve provenance, and normalize the result into the existing evidence graph.

## Near-term build order

1. Keep strengthening PSF/USF/2SF/SMPS/GEMS evidence extraction where it unlocks a musical question.
2. Add soundtrack-relational features: motif recurrence, cue families, arrangement transformations, shared harmonic/rhythmic/instrument fingerprints.
3. Add runtime-context adapters only where a game can be instrumented or reliably annotated.
4. Feed those observations into ludic-function hypotheses with explicit support/counterevidence.
5. Extend human-readable interpretation so it can say both what the music is doing and why the system believes that, for example:

   > The cue strongly functions as a transition bridge because the texture change and cadence align with the observed game-state boundary. The same musical evidence without the runtime boundary would support only a moderate hypothesis.

6. Evaluate on soundtrack-scale cases, not isolated synthetic cues. Ocarina of Time and Chrono Cross should be the first two deep cases for USF and PSF respectively.

## Research references used in this pass

- Arnold, Mathew. "Inside the Loop: The Audio Functionality of Inside." 2018. DOI: 10.1007/S40869-018-0071-X.
- Collins, Karen. *Playing with Sound: A Theory of Interacting with Sound and Music in Video Games.* 2013.
- Grasso, Julianne. "Ludomusical Narrativity." 2024. DOI: 10.1093/oxfordhb/9780197556160.013.12.
- Hutchings, Patrick, and Jon McCormack. "Adaptive Music Composition for Games." 2020. DOI: 10.1109/TG.2019.2921979.
- Kamp, Marlous. *Four Ways of Hearing Video Game Music.* 2023. DOI: 10.1093/oso/9780197651216.001.0001.
- Kamp, Michiel. "Musical Ecologies in Video Games." 2014. DOI: 10.1007/S13347-013-0113-Z.
- Warde-Brown, Ailbhe. "Waltzing on Rooftops and Cobblestones: Sonic Immersion through Spatiotemporal Involvement in the Assassin's Creed Series." 2021. DOI: 10.1525/JSMG.2021.2.3.34.

The literature was located through the project's SciSpace research pass. Claims above are deliberately translated into implementation consequences rather than treated as authority for game-specific conclusions.
