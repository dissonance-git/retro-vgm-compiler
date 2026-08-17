cmake_minimum_required(VERSION 3.20)
project(spc_provider_contracts LANGUAGES CXX)

enable_testing()
get_filename_component(RETRO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

set(SPC_PROVIDER_CONTRACT_TARGETS
    snesapu_prebrr_provider_test
    snesapu_prebrr_packet_test
    snesapu_studio_source_provider_test
    snesapu_studio_source_packet_test
)

add_executable(
    snesapu_prebrr_provider_test
    ${RETRO_ROOT}/tests/spc/snesapu_prebrr_provider_test.cpp
)
add_executable(
    snesapu_prebrr_packet_test
    ${RETRO_ROOT}/tests/spc/snesapu_prebrr_packet_test.cpp
)
add_executable(
    snesapu_studio_source_provider_test
    ${RETRO_ROOT}/tests/spc/snesapu_studio_source_provider_test.cpp
)
add_executable(
    snesapu_studio_source_packet_test
    ${RETRO_ROOT}/tests/spc/snesapu_studio_source_packet_test.cpp
)

foreach(target IN LISTS SPC_PROVIDER_CONTRACT_TARGETS)
    target_include_directories(${target} PRIVATE ${RETRO_ROOT})
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ${target} COMMAND ${target})
endforeach()
