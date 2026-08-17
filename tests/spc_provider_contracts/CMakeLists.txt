cmake_minimum_required(VERSION 3.20)
project(spc_provider_contracts LANGUAGES CXX)

enable_testing()
get_filename_component(RETRO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# These existing dependency-free tests cover the live child-side causal source
# chain that historically sat partly outside the root core registry:
# BRR/DIR topology -> native/enhanced source hooks -> packets/providers ->
# SRCE-v2 transport -> source-object projection.
set(SPC_PROVIDER_CONTRACT_TARGETS
    snes_spc_enhanced_source_hook_bridge_test
    snes_spc_native_source_hook_bridge_test
    snesapu_brr_playback_topology_test
    snesapu_prebrr_provider_test
    snesapu_prebrr_packet_test
    snesapu_source_object_projection_test
    snesapu_source_transport_v2_test
    snesapu_studio_source_provider_test
    snesapu_studio_source_packet_test
)

foreach(target IN LISTS SPC_PROVIDER_CONTRACT_TARGETS)
    add_executable(
        ${target}
        ${RETRO_ROOT}/tests/spc/${target}.cpp
    )
    target_include_directories(${target} PRIVATE ${RETRO_ROOT})
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endforeach()
