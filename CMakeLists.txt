cmake_minimum_required(VERSION 3.20)

project(vgm_compiler LANGUAGES CXX)

option(VGM_COMPILER_BUILD_TESTS "Build VGM Compiler tests" ON)

add_library(vgm_compiler_core STATIC
    components/vgm/enhancement/genesis_state.cpp
    components/vgm/enhancement/sn76489_enhanced.cpp
    components/vgm/enhancement/sn76489_advance.cpp
    components/vgm/enhancement/ym2612_dac_enhanced.cpp
    components/vgm/enhancement/ym2612_pcm_stream.cpp
    components/vgm/enhancement/ym2612_fm_timeline.cpp
    components/vgm/enhancement/ym2612_hq_fm_backend.cpp
    components/vgm/enhancement/source_stem_mixer.cpp
)

target_include_directories(vgm_compiler_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/components/vgm/enhancement
)

target_compile_features(vgm_compiler_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(vgm_compiler_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(vgm_compiler_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(VGM_COMPILER_BUILD_TESTS)
    enable_testing()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_subdirectory(tests)
endif()
