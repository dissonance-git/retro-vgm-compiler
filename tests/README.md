# Tests

`tests/` owns executable contracts and immutable real-music evidence. Tests validate current behavior, boundaries, and evidence obligations. They do not preserve retired implementation shapes merely because those shapes once existed.

## Owners

| Path | Owns |
| --- | --- |
| `model/` | shared musical/evidence semantic regressions |
| `vgm/` | VGM/Genesis execution, source, synthesis, bridge, and semantic regressions |
| `spc/` | SPC/SNESAPU state, runtime, source, and reconstruction regressions |
| `hes/` | HES-specific contracts |
| `integration/` | behavior spanning multiple durable owners |
| `private_components/` | package/build/runtime fixtures for private installable components |
| `spc_provider_contracts/` | provider-facing SPC contract fixtures |
| `corpus/` | immutable real-music controls, provenance, and machine inventory |
| root `test_*.py` | repository/build/package contracts that do not belong to one source family |

The real-music corpus contract is [`corpus/README.md`](corpus/README.md). Machine inventory and provenance live beside the corpus in tracked manifests and hashes.

## Test law

A useful test states a current invariant:

```text
current owner
+ current behavior / boundary
+ current evidence obligation
→ executable falsifier
```

Do not require deleted helpers, obsolete filenames, historical prose, or generated artifacts unless their continued existence is itself a current contract.

When a refactor removes duplicate ownership, update tests to validate the surviving invariant rather than recreating the retired route.

## Evidence classes

Keep these outcomes distinct:

```text
unit / semantic regression
build / link verification
corpus control
package verification
runtime smoke
listening / perceptual validation
```

Passing one class does not imply another class passed.

## Running tests

Default repository route:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`tests/run_full_core_suite.py` provides a repository-owned broader suite. Focused source-family or model tests should run first when they can falsify a change more cheaply.

Generated captures and outputs belong in ignored runtime paths such as `tests/output/`, not in the tracked test tree unless a fixture is explicitly canonical evidence.
