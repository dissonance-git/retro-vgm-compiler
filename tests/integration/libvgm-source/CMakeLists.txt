cmake_minimum_required(VERSION 3.20)
project(gameaudio_libvgm_source_tests LANGUAGES C CXX)

enable_testing()

set(LIBVGM_ROOT "" CACHE PATH "Path to the libvgm source tree used by source-capture integration")
if(NOT LIBVGM_ROOT)
    message(FATAL_ERROR "Set LIBVGM_ROOT to a libvgm checkout before configuring these integration tests")
endif()

set(LIBVGM_RESAMPLER_C "${LIBVGM_ROOT}/emu/Resampler.c")
set(LIBVGM_RESAMPLER_H "${LIBVGM_ROOT}/emu/Resampler.h")
if(NOT EXISTS "${LIBVGM_RESAMPLER_C}" OR NOT EXISTS "${LIBVGM_RESAMPLER_H}")
    message(FATAL_ERROR "LIBVGM_ROOT does not contain emu/Resampler.c and emu/Resampler.h: ${LIBVGM_ROOT}")
endif()

add_executable(
    linear_source_resampler_test
    linear_source_resampler_test.cpp
    "${LIBVGM_RESAMPLER_C}"
)
target_include_directories(linear_source_resampler_test PRIVATE "${LIBVGM_ROOT}")
target_compile_features(linear_source_resampler_test PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(linear_source_resampler_test PRIVATE /W4 /permissive-)
else()
    target_compile_options(linear_source_resampler_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME linear_source_resampler COMMAND linear_source_resampler_test)
