cmake_minimum_required(VERSION 3.20)

project(retro_vgm_hes_forensic_runtime LANGUAGES C CXX)

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
    LIBGME_HES_FORENSIC_COMMIT
    "fe8da4b6d3876d7542c2fb69d94487e19836d678"
    CACHE STRING
    "Pinned libgme commit used by the HES forensic runtime"
)
set(
    LIBGME_HES_FORENSIC_HOOK_CONTRACT
    "retro-vgm-compiler:libgme-hes-psg-native-adpcm-hook-v1-ordering-v1"
)

if(NOT LIBGME_HES_FORENSIC_COMMIT STREQUAL
       "fe8da4b6d3876d7542c2fb69d94487e19836d678")
    message(FATAL_ERROR
        "The HES forensic hooks are evidence-bound to libgme commit "
        "fe8da4b6d3876d7542c2fb69d94487e19836d678. "
        "Review the hook contract before changing the pin."
    )
endif()

# This is a research-only backend. Keep every unrelated emulator and optional
# dependency out so libgme remains a narrow HES execution dependency rather than
# entering the normal dependency-free VGM Compiler build.
set(USE_GME_AY OFF CACHE BOOL "" FORCE)
set(USE_GME_GBS OFF CACHE BOOL "" FORCE)
set(USE_GME_GYM OFF CACHE BOOL "" FORCE)
set(USE_GME_HES ON CACHE BOOL "" FORCE)
set(USE_GME_KSS OFF CACHE BOOL "" FORCE)
set(USE_GME_NSF OFF CACHE BOOL "" FORCE)
set(USE_GME_NSFE OFF CACHE BOOL "" FORCE)
set(USE_GME_SAP OFF CACHE BOOL "" FORCE)
set(USE_GME_SPC OFF CACHE BOOL "" FORCE)
set(USE_GME_VGM OFF CACHE BOOL "" FORCE)
set(GME_ZLIB OFF CACHE BOOL "" FORCE)
set(GME_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(GME_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(GME_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(GME_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(GME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libgme_hes_forensic
    GIT_REPOSITORY https://github.com/libgme/game-music-emu.git
    GIT_TAG ${LIBGME_HES_FORENSIC_COMMIT}
    GIT_SHALLOW FALSE
    GIT_PROGRESS FALSE
    UPDATE_DISCONNECTED TRUE
    PATCH_COMMAND
        ${Python3_EXECUTABLE}
        ${RETRO_VGM_COMPILER_ROOT}/tools/hes/patch_libgme_hes_forensic.py
        <SOURCE_DIR>
)
FetchContent_MakeAvailable(libgme_hes_forensic)

if(NOT TARGET gme_static)
    message(FATAL_ERROR "Pinned libgme HES build did not expose gme_static")
endif()

set(HES_FORENSIC_HOOK_HEADER
    "${CMAKE_CURRENT_LIST_DIR}/hes_forensic_hook_inject.h")

# libgme already owns GME_APU_HOOK at the HES CPU -> PSG boundary. Force-include
# our definition before Classic_Emu.h supplies its no-op fallback. The strict
# FetchContent transform adds only the otherwise-missing ADPCM observation call
# immediately before the pinned Hes_Apu_Adpcm write.
if(MSVC)
    target_compile_options(gme_static PRIVATE "/FI${HES_FORENSIC_HOOK_HEADER}")
else()
    target_compile_options(gme_static PRIVATE -include "${HES_FORENSIC_HOOK_HEADER}")
endif()

add_library(retro_vgm_hes_forensic_capture STATIC
    ${CMAKE_CURRENT_LIST_DIR}/hes_forensic_capture.cpp
)
target_include_directories(retro_vgm_hes_forensic_capture
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_features(retro_vgm_hes_forensic_capture PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(retro_vgm_hes_forensic_capture PRIVATE /W4 /permissive-)
else()
    target_compile_options(
        retro_vgm_hes_forensic_capture
        PRIVATE -Wall -Wextra -Wpedantic -Werror
    )
endif()

function(configure_hes_first_party target)
    target_link_libraries(${target} PRIVATE gme_static retro_vgm_hes_forensic_capture)
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} PRIVATE ${RETRO_VGM_COMPILER_ROOT})
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_executable(
    hes_forensic_hook_test
    ${RETRO_VGM_COMPILER_ROOT}/tests/hes/hes_forensic_hook_test.cpp
)
configure_hes_first_party(hes_forensic_hook_test)
add_test(NAME hes_forensic_hook COMMAND hes_forensic_hook_test)

add_executable(
    hes_forensic_features
    ${CMAKE_CURRENT_LIST_DIR}/hes_forensic_features.cpp
)
configure_hes_first_party(hes_forensic_features)
target_compile_definitions(
    hes_forensic_features
    PRIVATE
        RETRO_VGM_COMPILER_FORENSIC_COMMIT="${RETRO_VGM_COMPILER_FORENSIC_COMMIT}"
        LIBGME_HES_FORENSIC_COMMIT="${LIBGME_HES_FORENSIC_COMMIT}"
        LIBGME_HES_FORENSIC_HOOK_CONTRACT="${LIBGME_HES_FORENSIC_HOOK_CONTRACT}"
)
