# GSF effective-object semantics

## Question and boundary

What can version `0x22` GSF establish before a GBA runtime or music driver is
executed?

```text
exact xSF bytes
→ dependency-ordered GBA uploads
→ deterministic effective image + selected entry
→ runtime unavailable
```

This does not establish that GSF is MP2K, that a reconstructed upload image is
an original ROM, or that a driver, sequence, voice set, or playback path ran.

## Pinned observatories

| Observatory | Commit | Evidential role |
| --- | --- | --- |
| `kode54/psflib` | `95509e0c6f13d769593bbf51a1b0e0efdc355ba1` | Shared xSF envelope and dependency order |
| `kode54/viogsf` | `6c43a9926a6a85fbb736ea8f5f7f6c4f59ed3d64` | Current GSF player/runtime boundary |
| `loveemu/gsfopt` | `41538ea8bb9e3087f0c485e937467ed6b354f7b6` | 12-byte header, address mapping, recursive overlay, historical `gsf2rom` route |
| `loveemu/saptapper` | `ff7ec3e4da1f1ffc3bcc05793268036a319b4466` | Little-endian entry/load/size emission and per-song patches |
| `loveemu/agbinator` | `ec56b1ecfebddd29131f4934f65a94dfabcaf65d` | Candidate driver-signature scanner; inspected, not executed as an authority |
| `loveemu/vgmdocs` | `6c3c455071d81457e7a8cfec378a2edb8125ce7b` | Documentary GBA driver vocabulary |
| `ipatix/agbplay` | `0960aadec72dddbefc144216886d86bef220a0bb` | Independent header and GBA music-player control |
| `mgba-emu/mgba` | `afd6f14eaf8bd35214ed3fb9dc69a92bfc3877a9` | Whole-machine GBA runtime observatory, not integrated here |
| `pret/pokeemerald` | `9a83a2bbe8e097e62c00f1dbd56849766775d7b6` | Same-game reconstructed source; contextual driver evidence, not byte identity for this GSF image |

No current public `kode54/gsf2rom` repository was located. The observed
`gsf2rom` lineage is in `loveemu/gsfopt`; this source-location uncertainty is
preserved rather than reassigned by name.

## Observed contract and disagreement

The platform program is:

```text
u32 little-endian entry address
u32 little-endian load address
u32 little-endian payload size
payload bytes
```

`saptapper`, `gsfopt`, `in_xsf`, and `agbplay` independently expose this
layout. `gsfopt` and `in_xsf` differ in entry validation/precedence details:
`gsfopt` requires consistent entrypoints while the player-side `in_xsf`
surface selects the root object's entry after library-first mapping. The
project therefore preserves every upload entry and selects the resolved root
entry without pretending the upstreams say more uniformly than they do.

The implemented loader accepts bounded EWRAM or ROM uploads, rejects mixed
flat address spaces and malformed sizes, applies common xSF dependency order,
records per-byte overlay provenance, and exposes populated ranges separately
from zero-filled allocation gaps. Driver evidence is an explicit state:
`exact/source-established`, `signature-supported`, `behavioral-candidate`, or
`unknown`; the loader defaults to `unknown`.

## Real receiving evidence

The authorized Zophar Pokémon Emerald archive has SHA-256
`c9e7b778f3cc0c219a0dde8fd60a0d4ea47b64125051177e7df879add0282d88`.
It supplied 16 `.minigsf` roots and `AGB-BPEE-USA.gsflib`. Every root resolves
through that library. The library uploads 16,777,216 bytes at `0x08000000`;
each root overlays two bytes and selects entry `0x08000000`. All outer CRCs,
dependencies, bounds, provenance, and repeated effective builds pass.

The same-game source and historical tags make MP2K a research candidate, but
no runtime or independent byte-level driver identification was executed in
this change. The corpus manifest therefore records driver evidence as
`unknown`.
