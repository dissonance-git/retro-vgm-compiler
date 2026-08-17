cmake_minimum_required(VERSION 3.20)
project(private_component_frontier LANGUAGES CXX)

enable_testing()
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
