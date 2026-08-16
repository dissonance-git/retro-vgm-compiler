cmake_minimum_required(VERSION 3.20)

project(retro_vgm_spc_forensic_runtime LANGUAGES CXX)

enable_testing()

include(FetchContent)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

get_filename_component(
    RETRO_VGM_COMPILER_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../.."
    ABSOLUTE
)

set(
    SNES_SPC_FORENSIC_COMMIT
    "ec8ee2bbe30451614c1d02a83f7af1c97d497d45"
    CACHE STRING
    "Pinned snes_spc commit used by the SPC forensic runtime"
)

if(NOT SNES_SPC_FORENSIC_COMMIT STREQUAL
       "ec8ee2bbe30451614c1d02a83f7af1c97d497d45")
    message(FATAL_ERROR
        "The runtime patch is evidence-bound to snes_spc commit "
        "ec8ee2bbe30451614c1d02a83f7af1c97d497d45. "
        "Review and update the patch contract before changing the pin."
    )
endif()

FetchContent_Declare(
    snes_spc_forensic
    GIT_REPOSITORY https://github.com/blarggs-audio-libraries/snes_spc.git
    GIT_TAG ${SNES_SPC_FORENSIC_COMMIT}
    GIT_SHALLOW FALSE
    GIT_PROGRESS FALSE
    UPDATE_DISCONNECTED TRUE
    PATCH_COMMAND
        ${Python3_EXECUTABLE}
        ${RETRO_VGM_COMPILER_ROOT}/tools/spc/patch_snes_spc_runtime_strict.py
        <SOURCE_DIR>
)

FetchContent_GetProperties(snes_spc_forensic)
if(NOT snes_spc_forensic_POPULATED)
    FetchContent_Populate(snes_spc_forensic)
endif()

set(SNES_SPC_SOURCE_DIR "${snes_spc_forensic_SOURCE_DIR}/snes_spc")

add_library(gameaudio_snes_spc_forensic STATIC
    ${SNES_SPC_SOURCE_DIR}/SNES_SPC.cpp
    ${SNES_SPC_SOURCE_DIR}/SNES_SPC_misc.cpp
    ${SNES_SPC_SOURCE_DIR}/SNES_SPC_state.cpp
    ${SNES_SPC_SOURCE_DIR}/SPC_DSP.cpp
)

target_include_directories(gameaudio_snes_spc_forensic
    PUBLIC
        ${SNES_SPC_SOURCE_DIR}
    PRIVATE
        ${RETRO_VGM_COMPILER_ROOT}
)

target_compile_definitions(gameaudio_snes_spc_forensic
    PRIVATE
        HAVE_STDINT_H=1
        SPC_LESS_ACCURATE=0
        # Force the accurate DSP to catch up at SPC700 memory-access
        # boundaries. Playback normally permits lazy DSP catch-up, but a
        # forensic APURAM trace needs host callback order to preserve hardware
        # causal order between CPU writes and DSP/echo phases.
        SPC_MORE_ACCURACY=1
)

target_compile_features(gameaudio_snes_spc_forensic PUBLIC cxx_std_17)

# Keep upstream compiler diagnostics separate from first-party strict warnings.
# The source is pinned third-party evidence infrastructure, not style-normalized
# project code.
if(MSVC)
    target_compile_options(gameaudio_snes_spc_forensic PRIVATE /W3)
else()
    target_compile_options(gameaudio_snes_spc_forensic PRIVATE -Wall -Wextra)
endif()

add_executable(
    snes_spc_forensic_bridge_test
    ${RETRO_VGM_COMPILER_ROOT}/tests/spc/snes_spc_forensic_bridge_test.cpp
)
target_include_directories(
    snes_spc_forensic_bridge_test
    PRIVATE
        ${RETRO_VGM_COMPILER_ROOT}
)
target_link_libraries(
    snes_spc_forensic_bridge_test
    PRIVATE
        gameaudio_snes_spc_forensic
)
target_compile_features(snes_spc_forensic_bridge_test PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(snes_spc_forensic_bridge_test PRIVATE /W4 /permissive-)
else()
    target_compile_options(
        snes_spc_forensic_bridge_test
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
    )
endif()

add_test(NAME snes_spc_forensic_bridge COMMAND snes_spc_forensic_bridge_test)
