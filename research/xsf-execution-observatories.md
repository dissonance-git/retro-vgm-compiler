# xSF executable-object observatories

## Question

What is genuinely shared by PSF1, USF, and 2SF, and where must their
effective-object and runtime semantics remain platform-specific?

The receiving task was deliberately narrower than playback:

```text
exact xSF bytes
→ validated envelope and dependency order
→ platform-specific effective object
→ runtime unavailable
```

This pass does not establish CPU, SPU, RSP, or ARM execution; driver or
sequence identity; voice or part recovery; audio parity; or playback.

## Pinned adjacent observatories

| Observatory | Inspected commit | Evidence role | License observation |
| --- | --- | --- | --- |
| [`kode54/psflib`](https://github.com/kode54/psflib) | `95509e0c6f13d769593bbf51a1b0e0efdc355ba1` | Shared envelope parsing, outer compressed-program CRC, tag callbacks, and `_lib` load order | Repository `LICENSE` is MIT |
| [`kode54/lazyusf2`](https://gitlab.com/kode54/lazyusf2) | `421f00bcaa1988b8e1825e91780129f24fbd1aa0` | USF reserved-section upload into N64 ROM and Project64 save-state state | No repository-level license file was found; no code was copied |
| [`RGBA-CRT/vio2sf-fork`](https://github.com/RGBA-CRT/vio2sf-fork) | `cbad66408b72d3bdc9f6c5ba724fe3e17f996865` | 2SF program map overlays and reserved `SAVE` map records | No repository-level license file was found; no code was copied |
| [`vgmtrans/vgmtrans`](https://github.com/vgmtrans/vgmtrans) | `083f7c71fe773078061eb785573621082c3e0d1c` | Independent PS1 AKAO and Nintendo DS SDAT structural vocabulary | Repository `LICENSE` is zlib-style |

The PSF format v1.4 text attributed to Neill Corlett was also inspected via
the [preserved specification mirror](https://gist.github.com/SaxxonPike/11618bd321a45a70c01febae43ff564e).
It supplies the 16-byte envelope and PSF1 PS-X EXE field contract. The mature
players remain independent controls on how that contract is consumed.

## Observed facts and transfers

| Observed fact | Project transfer | Receiving check |
| --- | --- | --- |
| `psflib` processes the deepest primary `_lib` chain first, the current object next, then numbered `_lib2...` chains | `components/xsf/envelope.py` owns deterministic dependency resolution and cycle/missing/version rejection | Synthetic primary plus numbered-library ordering, cycles, missing files, and repeat-build equality |
| The outer CRC covers the compressed program bytes, not the reserved section or decompressed result | The common envelope validates that exact field before one strict zlib decode | Synthetic CRC corruption, truncation, version mismatch, and concatenated-zlib rejection; all 264 real objects admitted |
| PSF1 version `0x01` carries a PS-X EXE whose header names entry, text address/size, and stack state | `components/psf/psf1.py` overlays only PS-X EXE text and preserves root entry metadata | Synthetic overlapping executables plus 68 Chrono Cross effective-memory reconstructions |
| LazyUSF2 uploads each USF reserved section in psflib order; each ROM/save-state table marker is either zero or `SR64`; the observed program section is not part of that upload path | `components/usf/usf.py` parses ordered ROM and Project64 save-state patches and rejects non-empty program payloads | Synthetic ROM/save overlays plus 109 Ocarina roots resolving through `NUS-CZLE-USA.usflib` |
| vio2sf treats the decompressed 2SF program as little-endian offset/size/data and reserved `SAVE` payloads as compressed maps; its observed inner-CRC check is disabled with `if (0)` | `components/twosf/twosf.py` preserves the declared inner CRC and reports both compressed/decompressed comparisons without using either as an admission oracle | Synthetic ROM and save overlays plus 84 Mario Kart DS roots resolving through two libraries |
| VGMTrans has source-specific AKAO and SDAT machinery rather than one xSF semantic model | Keep PSF1, USF, and 2SF effective state in separate components | Version separation tests and no shared runtime/driver/voice abstraction |

## Negative results and boundaries

The search did **not** support a shared PSF-family execution backend. The
common mechanism earned here is limited to envelope identity, exact bytes,
CRC/zlib admission, tags, dependencies, overlay provenance, and deterministic
ordering. The meaning of both the program and reserved sections changes by
version.

VGMTrans's PS1 AKAO parser does not prove that a Chrono Cross PSF contains
AKAO, and its Nintendo DS SDAT parser does not prove that the effective Mario
Kart DS ROM has been located or interpreted as SDAT. Those are future
driver/sequence investigations requiring evidence from the effective object.

The Ocarina archive selection was not title-only. Two Zophar packs were
downloaded and compared. The retained source is the current 109-root pack
whose archive SHA-256 is
`78a5183ad0908b664ff2df356f70fa245f35060f43edb1fb7788f9281370c885`;
the alternate older 108-root pack was rejected to avoid two candidate corpus
truths.

## Real receiving evidence

| Corpus | Exact objects | Dependency surface | Validated effective object |
| --- | ---: | --- | --- |
| Chrono Cross | 68 PSF | 68 roots, no libraries | PS-X EXE memory at `0x80010000`, 1,140,032–1,509,120 bytes |
| Mario Kart DS | 84 mini2SF + 2 libraries | 53 roots use `NTR-AMCE-USA.2sflib`; 31 use `MKDSsound_data.2sflib` | NDS ROM maps, 4,276,320–4,276,348 bytes; no reserved `SAVE` records observed |
| Ocarina of Time | 109 miniUSF + 1 library | every root uses `NUS-CZLE-USA.usflib` | N64 ROM 5,087,668 bytes plus Project64 save state 4,204,380 bytes |

Every outer CRC and dependency closure passed. These are executable-object
controls, not playback controls. The next justified experiment is to attach
one platform runtime at a time and compare reference audio/state trajectories;
no common runtime should be proposed before those independent experiments.
