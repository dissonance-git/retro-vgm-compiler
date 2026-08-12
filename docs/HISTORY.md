# Project history and repository lineage

VGM Tooling has implementation history that predates this repository.

## Canonical implementation line

Current repository:

- `dissonance-git/vgm-tooling`
- created initially as `foobar2000-game-audio`
- renamed when the scope expanded from two playback components into executable game-music understanding

Historical predecessor:

- `dissonance-git/vgmspc`
- private
- final `main` observed at `773ae663be4966ee4019cfc8d5990c3ba7683694`
- final commit message: `fix: removed asymmetrical anti-phase and normalized M/S width to fix left bias and crackling`

Helix later recovered useful source-state concepts from this repository, including:

- VGM register shadowing
- YM2612 state
- SN76489 state
- SPC eight-voice telemetry
- OPM/OPN/OPL family routes
- persistent source IDs
- realtime adapter boundaries
- confidence/provenance ideas

It also identified old semantic-role heuristics and spatial-rendering code as historical experiments rather than architecture to revive unchanged.

## Required history migration

Do **not** squash `vgmspc` into one import commit.

Do **not** copy its final files into the root of VGM Tooling merely to claim the history was preserved.

The intended migration is a true Git merge with unrelated histories while keeping the current VGM Tooling working tree:

```bash
git clone git@github.com:dissonance-git/vgm-tooling.git
cd vgm-tooling

git remote add vgmspc git@github.com:dissonance-git/vgmspc.git
git fetch vgmspc main

# Preserve the exact old tip as a named historical anchor.
git tag -a history/vgmspc-final vgmspc/main \
  -m "Final vgmspc main before consolidation into VGM Tooling"

# Join the commit graphs without importing the obsolete working tree.
git merge --allow-unrelated-histories --no-ff -s ours vgmspc/main \
  -m "merge: preserve vgmspc history as VGM Tooling lineage"

# Verify that the old exact commit is now an ancestor of main.
git merge-base --is-ancestor 773ae663be4966ee4019cfc8d5990c3ba7683694 HEAD

git push origin main
git push origin history/vgmspc-final
```

Why `-s ours` is intentional:

```text
current VGM Tooling tree
= active implementation

vgmspc tree
= historical implementation state

vgmspc commit graph
= project ancestry worth preserving
```

The merge should preserve the **history**, not overwrite the current source tree with superseded files.

After the merge and push are verified, the old repository may be archived or deleted without losing its reachable commit history from the canonical repository.

## Verification before deleting the old repository

Run all of these against `vgm-tooling`:

```bash
git log --oneline --all --decorate --graph

git cat-file -t 773ae663be4966ee4019cfc8d5990c3ba7683694
# expected: commit

git merge-base --is-ancestor \
  773ae663be4966ee4019cfc8d5990c3ba7683694 \
  origin/main
# expected exit status: 0

git rev-parse history/vgmspc-final^{commit}
# expected: 773ae663be4966ee4019cfc8d5990c3ba7683694
```

Only then is deletion of `dissonance-git/vgmspc` safe from a Git-history perspective.

## Why this is one continuous project

The repository lineage should be read together with older Helix/VGMRips work:

```text
2012-2015
VGMRips / vgm2mid / SMPS / driver / ripping questions
        ↓
2026 early
vgmspc
register shadows + chip telemetry + playback/semantic experiments
        ↓
2026 Helix recovery
chip-state analyzer and tooling lineage preserved
        ↓
2026 current
VGM Tooling
executable source + driver + device + render understanding
```

The current project can reject old mechanisms without erasing the work that led to them.
