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

execute_process(
    COMMAND git -C ${RETRO_VGM_COMPILER_ROOT} rev-parse HEAD
    RESULT_VARIABLE RETRO_VGM_COMPILER_GIT_RESULT
    OUTPUT_VARIABLE RETRO_VGM_COMPILER_FORENSIC_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT RETRO_VGM_COMPILER_GIT_RESULT EQUAL 0 OR
   RETRO_VGM_COMPILER_FORENSIC_COMMIT STREQUAL "")
    set(RETRO_VGM_COMPILER_FORENSIC_COMMIT "unknown")
endif()

set(
    SNES_SPC_FORENSIC_COMMIT
    "ec8ee2bbe30451614c1d02a83f7af1c97d497d45"
    CACHE STRING
    "Pinned snes_spc commit used by the SPC forensic runtime"
)
set(
    SNES_SPC_FORENSIC_PATCH_CONTRACT
    "retro-vgm-compiler:snes-spc-runtime-hooks-v1-ordering-v2-native-spatial-v1-route-events-v1"
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
        ${RETRO_VGM_COMPILER_ROOT}/tools/spc/patch_snes_spc_forensic.py
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
        # State save/load intentionally stays outside the forensic build.
        # copy_state() toggles IPL visibility as serialization machinery, which
        # must never masquerade as a hardware-time APURAM mutation.
        SPC_NO_COPY_STATE_FUNCS=1
        # Keep upstream's broad validation mode disabled. SPC_MORE_ACCURACY
        # also enables unrelated probabilistic timer-glitch behavior, which is
        # inappropriate for deterministic corpus evidence.
        SPC_MORE_ACCURACY=0
        # Forensic ordering v2 catches the accurate DSP only at actual shared-
        # APURAM hazards: before ordinary CPU RAM writes, before reads that can
        # observe DSP echo RAM, and before CONTROL replaces the visible IPL
        # window. Native $F3 DSP register reads/writes keep their own upstream
        # synchronization path. The broad stochastic validation mode stays off.
        RETRO_VGM_SPC_FORENSIC_ORDERING=1
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

function(gameaudio_configure_forensic_first_party target)
    target_include_directories(${target} PRIVATE ${RETRO_VGM_COMPILER_ROOT})
    target_link_libraries(${target} PRIVATE gameaudio_snes_spc_forensic)
    target_compile_features(${target} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Werror
        )
    endif()
endfunction()

function(gameaudio_add_forensic_provenance target)
    target_compile_definitions(
        ${target}
        PRIVATE
            RETRO_VGM_COMPILER_FORENSIC_COMMIT="${RETRO_VGM_COMPILER_FORENSIC_COMMIT}"
            SNES_SPC_FORENSIC_COMMIT="${SNES_SPC_FORENSIC_COMMIT}"
            SNES_SPC_FORENSIC_PATCH_CONTRACT="${SNES_SPC_FORENSIC_PATCH_CONTRACT}"
    )
endfunction()

add_executable(
    snes_spc_forensic_bridge_test
    ${RETRO_VGM_COMPILER_ROOT}/tests/spc/snes_spc_forensic_bridge_test.cpp
)
gameaudio_configure_forensic_first_party(snes_spc_forensic_bridge_test)
add_test(NAME snes_spc_forensic_bridge COMMAND snes_spc_forensic_bridge_test)

add_executable(
    snes_spc_native_spatial_observer_test
    ${RETRO_VGM_COMPILER_ROOT}/tests/spc/snes_spc_native_spatial_observer_test.cpp
)
gameaudio_configure_forensic_first_party(snes_spc_native_spatial_observer_test)
add_test(
    NAME snes_spc_native_spatial_observer
    COMMAND snes_spc_native_spatial_observer_test
)

add_executable(
    spc_native_routed_source_projection_test
    ${RETRO_VGM_COMPILER_ROOT}/tests/spc/spc_native_routed_source_projection_test.cpp
)
gameaudio_configure_forensic_first_party(spc_native_routed_source_projection_test)
add_test(
    NAME spc_native_routed_source_projection
    COMMAND spc_native_routed_source_projection_test
)

add_executable(
    spc_forensic_features
    ${CMAKE_CURRENT_LIST_DIR}/spc_forensic_features.cpp
)
gameaudio_configure_forensic_first_party(spc_forensic_features)
gameaudio_add_forensic_provenance(spc_forensic_features)

add_executable(
    spc_spatial_governor_trace
    ${CMAKE_CURRENT_LIST_DIR}/spc_spatial_governor_trace.cpp
)
gameaudio_configure_forensic_first_party(spc_spatial_governor_trace)
gameaudio_add_forensic_provenance(spc_spatial_governor_trace)

# One-second real-file smoke only. This does not count as the preregistered
# 24-second corpus result; it exists to make the exact pinned DSP patch, snapshot
# seed, route events, ten source planes, ABI transport and trace validator prove
# that they can execute together on an immutable SPC object.
add_test(
    NAME spc_spatial_governor_trace_smoke
    COMMAND spc_spatial_governor_trace
        "${RETRO_VGM_COMPILER_ROOT}/tests/corpus/super-black-bass-spc/01 - Title Screen.spc"
        "${CMAKE_CURRENT_BINARY_DIR}/spc-governor-smoke.json"
        "c04e06a65e964746beb594b87317e8eb5caaaf0020f328fc9eb8305ff6812d98"
        1
)
set_tests_properties(
    spc_spatial_governor_trace_smoke
    PROPERTIES TIMEOUT 60
)
