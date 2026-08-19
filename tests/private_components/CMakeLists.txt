cmake_minimum_required(VERSION 3.20)
project(private_component_frontier LANGUAGES CXX)

enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
get_filename_component(RETRO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

set(PRIVATE_COMPONENT_TESTS
    spatial_playback_options_test
    genesis_timed_spatial_transport_test
    genesis_selected_source_block_test
    genesis_spatial_route_transport_test
    snesapu_private_spatial_transport_test
)

add_executable(spatial_playback_options_test
    ${RETRO_ROOT}/tests/model/spatial_playback_options_test.cpp)
add_executable(genesis_timed_spatial_transport_test
    ${RETRO_ROOT}/tests/vgm/genesis_timed_spatial_transport_test.cpp)
add_executable(genesis_selected_source_block_test
    ${RETRO_ROOT}/tests/vgm/genesis_selected_source_block_test.cpp)
add_executable(genesis_spatial_route_transport_test
    ${RETRO_ROOT}/tests/vgm/genesis_spatial_route_transport_test.cpp)
add_executable(snesapu_private_spatial_transport_test
    ${RETRO_ROOT}/tests/spc/snesapu_private_spatial_transport_test.cpp)

foreach(target IN LISTS PRIVATE_COMPONENT_TESTS)
    target_include_directories(${target} PRIVATE ${RETRO_ROOT})
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endforeach()

# The final bundle names one exact Retro VGM Compiler commit as provenance.
# Refuse a non-Git or modified tracked source state before any external
# dependency checkout so that manifest commit always describes compiled input.
add_test(
    NAME build_source_provenance
    COMMAND ${Python3_EXECUTABLE} -B
        ${RETRO_ROOT}/tools/verify_build_source_provenance.py
        ${RETRO_ROOT}
)
set_tests_properties(
    build_source_provenance
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Exercise the provenance validator itself in disposable Git repositories. This
# catches staged/unstaged drift and a clean HEAD switch during a long build.
add_test(
    NAME build_source_provenance_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_build_source_provenance_contract.py
)
set_tests_properties(
    build_source_provenance_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# The complete repository core suite is intentionally not a prerequisite for
# producing the two private foobar components. It has its own normal CI surface
# and can fail on unrelated research/frontier work. Keep this test registered
# for manual diagnostics, but disable it in this binary-delivery frontier so
# unrelated core work cannot prevent packaging an otherwise verified runtime.
set(FULL_CORE_SUITE_COMMAND
    ${Python3_EXECUTABLE} -B
    ${RETRO_ROOT}/tests/run_full_core_suite.py
    --generator "${CMAKE_GENERATOR}"
    --config Release
)
if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND FULL_CORE_SUITE_COMMAND --platform "${CMAKE_GENERATOR_PLATFORM}")
endif()
add_test(NAME full_root_core_suite COMMAND ${FULL_CORE_SUITE_COMMAND})
set_tests_properties(
    full_root_core_suite
    PROPERTIES
        WORKING_DIRECTORY ${RETRO_ROOT}
        DISABLED TRUE
)

# Guard the high-risk generated-host seams before the expensive Windows build.
# These tests operate only on temporary files and assert exact guarded patch
# composition/order, including family-local fallback, the one 48 kHz private
# host clock, companion launch geometry, and rate-specific Omniphony lifetime.
add_test(
    NAME vgm_deferred_family_independence_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/vgm
        -p test_studio_deferred_family_independence_patch.py
)
set_tests_properties(
    vgm_deferred_family_independence_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME vgm_private_48khz_output_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/vgm
        -p test_private_48khz_output_patch.py
)
set_tests_properties(
    vgm_private_48khz_output_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME vgm_spatial_omniphony_rate_lifecycle_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/vgm
        -p test_spatial_omniphony_rate_lifecycle_patch.py
)
set_tests_properties(
    vgm_spatial_omniphony_rate_lifecycle_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME spc_private_48khz_output_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/spc
        -p test_enhanced_runtime_rate.py
)
set_tests_properties(
    spc_private_48khz_output_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME spc_spatial_omniphony_rate_lifecycle_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/spc
        -p test_spatial_omniphony_private_rate_lifecycle_patch.py
)
set_tests_properties(
    spc_spatial_omniphony_rate_lifecycle_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME spc_private_child_launch_path_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/spc
        -p test_private_child_launch_path.py
)
set_tests_properties(
    spc_private_child_launch_path_patch
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Keep the retirement invariant registered, but do not make this historical
# repository-wide hygiene scan a binary-delivery prerequisite. It currently
# sees preserved repair-workflow text under .github that names imports/vgmspc;
# the active private builder and materializers independently prove their exact
# inputs and do not consult that retired repository.
add_test(
    NAME no_live_vgmspc_dependency
    COMMAND ${Python3_EXECUTABLE} -B
        ${RETRO_ROOT}/tests/test_no_live_vgmspc_dependency.py
)
set_tests_properties(
    no_live_vgmspc_dependency
    PROPERTIES
        WORKING_DIRECTORY ${RETRO_ROOT}
        DISABLED TRUE
)

add_test(
    NAME enhanced_wording_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/vgm
        -p test_enhanced_wording.py
)
set_tests_properties(
    enhanced_wording_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME spc_active_patch_contract
    COMMAND ${Python3_EXECUTABLE} -B
        ${RETRO_ROOT}/tests/spc/test_active_spc_patch_contract.py
)
set_tests_properties(
    spc_active_patch_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# These regressions are script-style rather than unittest.TestCase-based;
# execute them directly so a discovery run cannot silently report "Ran 0 tests".
add_test(
    NAME vgm_materializer_contract
    COMMAND ${Python3_EXECUTABLE} -B
        ${RETRO_ROOT}/tests/vgm/test_materialize_foo_input_vgm.py
)
set_tests_properties(
    vgm_materializer_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME spc_materializer_contract
    COMMAND ${Python3_EXECUTABLE} -B
        ${RETRO_ROOT}/tests/spc/test_materialize_foo_snesapu.py
)
set_tests_properties(
    spc_materializer_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Verify the archive contract independently of compilation. Synthetic ZIPs prove
# that the final fb2k-component verifier accepts only the exact sibling runtime
# payload and rejects nested, traversal, duplicate, extra, or empty entries.
add_test(
    NAME private_component_package_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_private_component_package_contract.py
)
set_tests_properties(
    private_component_package_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# The outer release ZIP is also evidence: its manifest, checksums and component
# bytes must agree exactly, and its README must preserve the descriptive naming
# contract. Portable synthetic bundles exercise those metadata failures here.
add_test(
    NAME private_component_bundle_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_private_component_bundle_contract.py
)
set_tests_properties(
    private_component_bundle_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Keep the executable Omniphony version gate synchronized with the compiler's
# transport constants. The real Windows package verifier then loads the archived
# DLL copies and executes these same zero-argument ABI functions.
add_test(
    NAME omniphony_runtime_abi_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_omniphony_runtime_abi_contract.py
)
set_tests_properties(
    omniphony_runtime_abi_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Packaging only works if each plugin resolves omniphony_source.dll relative to
# the embedding component, not merely foobar2000.exe or PATH. Pin sibling-first
# lookup and the post-load ABI guard as a portable source contract.
add_test(
    NAME omniphony_sibling_loader_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_omniphony_sibling_loader_contract.py
)
set_tests_properties(
    omniphony_sibling_loader_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# The private builder itself is part of the product contract. Keep outputs
# deterministic and prove every packaged executable's bitness before archive
# creation so a stale or wrong-architecture artifact cannot be selected.
add_test(
    NAME private_component_builder_contract
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests
        -p test_private_component_builder_contract.py
)
set_tests_properties(
    private_component_builder_contract
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

# Materialize both complete private components from their repository-owned
# immutable seeds. This executes the canonical guarded Python patch chains in
# the same order used by the packaging script, catching stale anchors before
# external dependency checkout/compilation begins. The Windows private builder
# already requires 7-Zip on PATH for the VGM bootstrap archive.
set(PRIVATE_VGM_MATERIALIZED_ROOT
    ${CMAKE_CURRENT_BINARY_DIR}/materialized-vgm/components/vgm)
set(PRIVATE_SPC_MATERIALIZED_ROOT
    ${CMAKE_CURRENT_BINARY_DIR}/materialized-spc)

add_test(
    NAME private_vgm_materialization
    COMMAND ${Python3_EXECUTABLE}
        ${RETRO_ROOT}/tools/materialize_foo_input_vgm.py
        --sdk-root ${PRIVATE_VGM_MATERIALIZED_ROOT}
)
set_tests_properties(
    private_vgm_materialization
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)

add_test(
    NAME private_spc_materialization
    COMMAND ${Python3_EXECUTABLE}
        ${RETRO_ROOT}/tools/materialize_foo_snesapu.py
        ${PRIVATE_SPC_MATERIALIZED_ROOT}
        --force
)
set_tests_properties(
    private_spc_materialization
    PROPERTIES WORKING_DIRECTORY ${RETRO_ROOT}
)
