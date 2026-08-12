cmake_minimum_required(VERSION 3.20)

project(foobar2000_game_audio_core LANGUAGES CXX)

option(GAMEAUDIO_BUILD_CORE_TESTS "Build dependency-free realtime state tests" ON)

add_library(gameaudio_vgm_state STATIC
    components/vgm/enhancement/genesis_state.cpp
)

target_include_directories(gameaudio_vgm_state
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/components/vgm/enhancement
)

target_compile_features(gameaudio_vgm_state PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(gameaudio_vgm_state PRIVATE /W4 /permissive-)
else()
    target_compile_options(gameaudio_vgm_state PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(GAMEAUDIO_BUILD_CORE_TESTS)
    enable_testing()

    add_executable(genesis_state_test
        tests/vgm/genesis_state_test.cpp
    )
    target_link_libraries(genesis_state_test PRIVATE gameaudio_vgm_state)
    target_compile_features(genesis_state_test PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(genesis_state_test PRIVATE /W4 /permissive-)
    else()
        target_compile_options(genesis_state_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME genesis_state COMMAND genesis_state_test)
endif()
