cmake_minimum_required(VERSION 3.20)

project(foobar2000_game_audio_core LANGUAGES CXX)

option(GAMEAUDIO_BUILD_CORE_TESTS "Build dependency-free realtime core tests" ON)

add_library(gameaudio_vgm_core STATIC
    components/vgm/enhancement/genesis_state.cpp
    components/vgm/enhancement/sn76489_enhanced.cpp
    components/vgm/enhancement/sn76489_advance.cpp
    components/vgm/enhancement/ym2612_dac_enhanced.cpp
    components/vgm/enhancement/ym2612_pcm_stream.cpp
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
        ym2612_pcm_stream_test
        musical_execution_graph_test
        voice_separation_hypothesis_test
        musical_inference_evidence_test
        analysis_feature_test
        source_analysis_feature_adapter_test
        tracker_analysis_feature_test
        vgm_execution_graph_adapter_test
        vgm_command_trace_capture_test
        genesis_execution_graph_adapter_test
        genesis_performance_adapter_test
        genesis_pitch_control_adapter_test
        spc_snapshot_test
        spc_brr_sample_test
        spc_runtime_voice_adapter_test
        spc_runtime_source_transition_test
        spc_ram_generation_test
        spc_ram_shadow_test
        spc_runtime_sample_adapter_test
        spc_runtime_capture_test
        spc_runtime_capture_adapter_test
    )

    add_executable(genesis_state_test tests/vgm/genesis_state_test.cpp)
    add_executable(genesis_event_tap_test tests/vgm/genesis_event_tap_test.cpp)
    add_executable(sn76489_enhanced_test tests/vgm/sn76489_enhanced_test.cpp)
    add_executable(sn76489_variants_test tests/vgm/sn76489_variants_test.cpp)
    add_executable(sn76489_advance_test tests/vgm/sn76489_advance_test.cpp)
    add_executable(psg_block_capture_test tests/vgm/psg_block_capture_test.cpp)
    add_executable(ym2612_dac_enhanced_test tests/vgm/ym2612_dac_enhanced_test.cpp)
    add_executable(ym2612_dac_block_capture_test tests/vgm/ym2612_dac_block_capture_test.cpp)
    add_executable(ym2612_pcm_stream_test tests/vgm/ym2612_pcm_stream_test.cpp)
    add_executable(musical_execution_graph_test tests/model/musical_execution_graph_test.cpp)
    add_executable(voice_separation_hypothesis_test tests/model/voice_separation_hypothesis_test.cpp)
    add_executable(musical_inference_evidence_test tests/model/musical_inference_evidence_test.cpp)
    add_executable(analysis_feature_test tests/model/analysis_feature_test.cpp)
    add_executable(source_analysis_feature_adapter_test tests/model/source_analysis_feature_adapter_test.cpp)
    add_executable(tracker_analysis_feature_test tests/model/tracker_analysis_feature_test.cpp)
    add_executable(vgm_execution_graph_adapter_test tests/vgm/vgm_execution_graph_adapter_test.cpp)
    add_executable(vgm_command_trace_capture_test tests/vgm/vgm_command_trace_capture_test.cpp)
    add_executable(genesis_execution_graph_adapter_test tests/vgm/genesis_execution_graph_adapter_test.cpp)
    add_executable(genesis_performance_adapter_test tests/vgm/genesis_performance_adapter_test.cpp)
    add_executable(genesis_pitch_control_adapter_test tests/vgm/genesis_pitch_control_adapter_test.cpp)
    add_executable(spc_snapshot_test tests/spc/spc_snapshot_test.cpp)
    add_executable(spc_brr_sample_test tests/spc/spc_brr_sample_test.cpp)
    add_executable(spc_runtime_voice_adapter_test tests/spc/spc_runtime_voice_adapter_test.cpp)
    add_executable(spc_runtime_source_transition_test tests/spc/spc_runtime_source_transition_test.cpp)
    add_executable(spc_ram_generation_test tests/spc/spc_ram_generation_test.cpp)
    add_executable(spc_ram_shadow_test tests/spc/spc_ram_shadow_test.cpp)
    add_executable(spc_runtime_sample_adapter_test tests/spc/spc_runtime_sample_adapter_test.cpp)
    add_executable(spc_runtime_capture_test tests/spc/spc_runtime_capture_test.cpp)
    add_executable(spc_runtime_capture_adapter_test tests/spc/spc_runtime_capture_adapter_test.cpp)
    target_include_directories(musical_execution_graph_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(voice_separation_hypothesis_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(musical_inference_evidence_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(analysis_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(source_analysis_feature_adapter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(tracker_analysis_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
    add_test(NAME ym2612_pcm_stream COMMAND ym2612_pcm_stream_test)
    add_test(NAME musical_execution_graph COMMAND musical_execution_graph_test)
    add_test(NAME voice_separation_hypothesis COMMAND voice_separation_hypothesis_test)
    add_test(NAME musical_inference_evidence COMMAND musical_inference_evidence_test)
    add_test(NAME analysis_feature COMMAND analysis_feature_test)
    add_test(NAME source_analysis_feature_adapter COMMAND source_analysis_feature_adapter_test)
    add_test(NAME tracker_analysis_feature COMMAND tracker_analysis_feature_test)
    add_test(NAME vgm_execution_graph_adapter COMMAND vgm_execution_graph_adapter_test)
    add_test(NAME vgm_command_trace_capture COMMAND vgm_command_trace_capture_test)
    add_test(NAME genesis_execution_graph_adapter COMMAND genesis_execution_graph_adapter_test)
    add_test(NAME genesis_performance_adapter COMMAND genesis_performance_adapter_test)
    add_test(NAME genesis_pitch_control_adapter COMMAND genesis_pitch_control_adapter_test)
    add_test(NAME spc_snapshot COMMAND spc_snapshot_test)
    add_test(NAME spc_brr_sample COMMAND spc_brr_sample_test)
    add_test(NAME spc_runtime_voice_adapter COMMAND spc_runtime_voice_adapter_test)
    add_test(NAME spc_runtime_source_transition COMMAND spc_runtime_source_transition_test)
    add_test(NAME spc_ram_generation COMMAND spc_ram_generation_test)
    add_test(NAME spc_ram_shadow COMMAND spc_ram_shadow_test)
    add_test(NAME spc_runtime_sample_adapter COMMAND spc_runtime_sample_adapter_test)
    add_test(NAME spc_runtime_capture COMMAND spc_runtime_capture_test)
    add_test(NAME spc_runtime_capture_adapter COMMAND spc_runtime_capture_adapter_test)
endif()
