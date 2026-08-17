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

# Guard the high-risk generated-host seams before the expensive Windows build.
# These tests operate only on temporary files and assert exact guarded patch
# composition/order, including family-local fallback and rate-specific
# Omniphony processor lifetime.
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
    NAME spc_spatial_omniphony_rate_lifecycle_patch
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${RETRO_ROOT}/tests/spc
        -p test_spatial_omniphony_private_rate_lifecycle_patch.py
)
set_tests_properties(
    spc_spatial_omniphony_rate_lifecycle_patch
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
