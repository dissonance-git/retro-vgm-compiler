# Documentation

`docs/` owns durable project architecture, musical reasoning contracts, current roadmap material, and preserved history.

For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).

## Current surface

Keep the top level of `docs/` biased toward **current contracts and active orientation**.

High-value entry points include:

```text
retro-vgm-compiler-roadmap.md
holistic-soundtrack-understanding.md
holistic-musical-understanding.md
composer-level-understanding.md
musical-execution-model.md
musical-inference-evidence.md
musical-understanding-dependencies.md
music-representation-systems.md
persistent-musical-identity.md
human-musical-discourse.md
source-native-enhanced-rendering.md
omniphony-realtime-spatial-path.md
upstreams.md
```

`omniphony-realtime-spatial-path.md` is the canonical owner for the cross-project source-to-spatial boundary: source-native objects, AUTHORED/DERIVED/EMPTY authority, dry versus shared-wet semantics, causal timing/identity, and the rule that Omniphony's 8.1.4.4 scene is a presentation vocabulary rather than a forced game-music source width.

These documents describe different layers or obligations. Do not create another overview that merely restates them.

## Subdirectories

```text
docs/history/      superseded bootstrap instructions and technical lineage
docs/generated/    rebuildable navigation/inventory projections
```

Historical material stays reachable without competing with current contracts.
Generated material is a projection and must not become writable source truth.

## Placement rule

Before adding a new top-level document, ask:

1. Does an existing current contract already own this distinction?
2. Is this actually a bounded research result that belongs under `research/`?
3. Is this historical state that belongs under `docs/history/`?
4. Is this mechanically derivable inventory that belongs under `docs/generated/`?

The active documentation shelf should become easier to scan as the project grows, not proportionally larger.
