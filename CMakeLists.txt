cmake_minimum_required(VERSION 3.20)

project(foobar2000_game_audio_core LANGUAGES CXX)

option(GAMEAUDIO_BUILD_CORE_TESTS "Build dependency-free realtime core tests" ON)

add_library(gameaudio_vgm_core STATIC
    components/vgm/enhancement/genesis_state.cpp
    components/vgm/enhancement/sn76489_enhanced.cpp
    components/vgm/enhancement/sn76489_advance.cpp
    components/vgm/enhancement/ym2612_dac_enhanced.cpp
)

target_include_directories(gameaudio_vgm_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/components/vgm/enhancement
)

target_compile_features(gameaudio_vgm_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(gameaudio_vgm_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(gameaudio_vgm_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(GAMEAUDIO_BUILD_CORE_TESTS)
    enable_testing()

    set(GAMEAUDIO_TEST_TARGETS
        genesis_state_test
        genesis_event_tap_test
        sn76489_enhanced_test
        sn76489_variants_test
        sn76489_advance_test
        psg_block_capture_test
        ym2612_dac_enhanced_test
        ym2612_dac_block_capture_test
    )

    add_executable(genesis_state_test tests/vgm/genesis_state_test.cpp)
    add_executable(genesis_event_tap_test tests/vgm/genesis_event_tap_test.cpp)
    add_executable(sn76489_enhanced_test tests/vgm/sn76489_enhanced_test.cpp)
    add_executable(sn76489_variants_test tests/vgm/sn76489_variants_test.cpp)
    add_executable(sn76489_advance_test tests/vgm/sn76489_advance_test.cpp)
    add_executable(psg_block_capture_test tests/vgm/psg_block_capture_test.cpp)
    add_executable(ym2612_dac_enhanced_test tests/vgm/ym2612_dac_enhanced_test.cpp)
    add_executable(ym2612_dac_block_capture_test tests/vgm/ym2612_dac_block_capture_test.cpp)

    foreach(test_target IN LISTS GAMEAUDIO_TEST_TARGETS)
        target_link_libraries(${test_target} PRIVATE gameaudio_vgm_core)
        target_compile_features(${test_target} PRIVATE cxx_std_17)
        if(MSVC)
            target_compile_options(${test_target} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
    endforeach()

    add_test(NAME genesis_state COMMAND genesis_state_test)
    add_test(NAME genesis_event_tap COMMAND genesis_event_tap_test)
    add_test(NAME sn76489_enhanced COMMAND sn76489_enhanced_test)
    add_test(NAME sn76489_variants COMMAND sn76489_variants_test)
    add_test(NAME sn76489_advance COMMAND sn76489_advance_test)
    add_test(NAME psg_block_capture COMMAND psg_block_capture_test)
    add_test(NAME ym2612_dac_enhanced COMMAND ym2612_dac_enhanced_test)
    add_test(NAME ym2612_dac_block_capture COMMAND ym2612_dac_block_capture_test)
endif()
