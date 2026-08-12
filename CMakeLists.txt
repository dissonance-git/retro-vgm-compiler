cmake_minimum_required(VERSION 3.20)

project(foobar2000_game_audio_core LANGUAGES CXX)

option(GAMEAUDIO_BUILD_CORE_TESTS "Build dependency-free realtime core tests" ON)

add_library(gameaudio_vgm_core STATIC
    components/vgm/enhancement/genesis_state.cpp
    components/vgm/enhancement/sn76489_enhanced.cpp
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

    add_executable(genesis_state_test
        tests/vgm/genesis_state_test.cpp
    )
    target_link_libraries(genesis_state_test PRIVATE gameaudio_vgm_core)
    target_compile_features(genesis_state_test PRIVATE cxx_std_17)

    add_executable(sn76489_enhanced_test
        tests/vgm/sn76489_enhanced_test.cpp
    )
    target_link_libraries(sn76489_enhanced_test PRIVATE gameaudio_vgm_core)
    target_compile_features(sn76489_enhanced_test PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(genesis_state_test PRIVATE /W4 /permissive-)
        target_compile_options(sn76489_enhanced_test PRIVATE /W4 /permissive-)
    else()
        target_compile_options(genesis_state_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(sn76489_enhanced_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME genesis_state COMMAND genesis_state_test)
    add_test(NAME sn76489_enhanced COMMAND sn76489_enhanced_test)
endif()
