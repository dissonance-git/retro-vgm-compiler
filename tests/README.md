# Tests

`tests/` owns executable contracts and immutable real-music evidence. Tests validate current behavior, boundaries, and evidence obligations. They do not preserve retired implementation shapes merely because those shapes once existed.

Choose the narrowest current owner: shared-model regressions live with model tests, source-family regressions with their family tests, cross-owner behavior in integration tests, repository/build/package contracts at the repository-contract layer, and immutable real-music controls under the corpus owner. Use the tree or `repository_catalog.py` for exact directory inventory rather than copying it here.

## Test law

A useful test states a current invariant:

```text
current owner
+ current behavior / boundary
+ current evidence obligation
→ executable falsifier
```

Do not require deleted helpers, obsolete filenames, historical prose, or generated artifacts unless their continued existence is itself a current contract. When a refactor removes duplicate ownership, update tests to validate the surviving invariant rather than recreating the retired route.

Keep outcome classes distinct:

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

`tests/run_full_core_suite.py` is the repository-owned broader core/provider/package route. `tools/run_core_tests.py` independently compiles dependency-free C++ tests against the complete local core so unregistered translation units cannot hide behind CMake registration. Those routes are complementary, not aliases.

Generated captures and outputs belong in ignored runtime paths such as `tests/output/`, not in the tracked test tree unless a fixture is explicitly canonical evidence.
