cmake_minimum_required(VERSION 3.20)

project(foobar2000_game_audio_core LANGUAGES CXX)

option(GAMEAUDIO_BUILD_CORE_TESTS "Build dependency-free realtime core tests" ON)

add_library(gameaudio_vgm_core STATIC
    components/vgm/enhancement/genesis_state.cpp
    components/vgm/enhancement/sn76489_enhanced.cpp
    components/vgm/enhancement/sn76489_advance.cpp
    components/vgm/enhancement/ym2612_dac_enhanced.cpp
    components/vgm/enhancement/ym2612_pcm_stream.cpp
    components/vgm/enhancement/ym2612_fm_timeline.cpp
    components/vgm/enhancement/source_stem_mixer.cpp
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
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(GAMEAUDIO_TEST_TARGETS
        genesis_state_test
        genesis_event_tap_test
        authored_stereo_route_test
        sn76489_enhanced_test
        sn76489_variants_test
        sn76489_advance_test
        sn76489_alias_test
        psg_block_capture_test
        ym2612_block_capture_test
        ym2612_dac_enhanced_test
        ym2612_dac_block_capture_test
        ym2612_pcm_stream_test
        ym2612_fm_timeline_test
        genesis_nominal_pitch_test
        smps_programmed_pitch_test
        yamaha_nominal_pitch_test
        tone_generator_nominal_pitch_test
        source_stem_mixer_test
        spatial_source_test
        spatial_source_timed_evidence_test
        realtime_spatial_scene_dsp_test
        realtime_musical_spatial_observer_test
        realtime_musical_role_hypothesis_test
        realtime_musical_role_tracker_test
        realtime_musical_spatial_frontend_test
        realtime_auditory_state_evidence_test
        omniphony_source_transport_test
        omniphony_realtime_client_test
        realtime_musical_omniphony_pipeline_test
        musical_execution_graph_test
        transformation_identity_test
        voice_separation_hypothesis_test
        musical_inference_evidence_test
        musical_understanding_dependency_test
        genesis_nominal_pitch_feature_test
        analysis_feature_test
        source_analysis_feature_adapter_test
        tracker_analysis_feature_test
        listener_response_feature_test
        musicological_identity_test
        cross_cultural_pitch_rhythm_test
        timbre_instrument_identity_test
        creative_role_attribution_test
        attribution_discovery_control_test
        genesis_driver_source_boundary_test
        authored_source_projection_test
        tonal_center_hypothesis_test
        tonal_center_evidence_adapter_test
        diatonic_key_hypothesis_test
        pitch_class_collection_adapter_test
        vgm_execution_graph_adapter_test
        vgm_command_trace_capture_test
        vgm_yamaha_register_write_test
        vgm_chip_clock_test
        vgm_dac_stream_command_test
        yamaha_opn_family_test
        yamaha_opn_register_test
        yamaha_opm_register_test
        yamaha_opl_family_test
        yamaha_opl_register_test
        yamaha_opll_register_test
        genesis_execution_graph_adapter_test
        genesis_performance_adapter_test
        genesis_pitch_control_adapter_test
        ym2612_performed_pitch_motion_test
        genesis_performed_part_motif_test
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
    add_executable(authored_stereo_route_test tests/vgm/authored_stereo_route_test.cpp)
    add_executable(sn76489_enhanced_test tests/vgm/sn76489_enhanced_test.cpp)
    add_executable(sn76489_variants_test tests/vgm/sn76489_variants_test.cpp)
    add_executable(sn76489_advance_test tests/vgm/sn76489_advance_test.cpp)
    add_executable(sn76489_alias_test tests/vgm/sn76489_alias_test.cpp)
    add_executable(psg_block_capture_test tests/vgm/psg_block_capture_test.cpp)
    add_executable(ym2612_block_capture_test tests/vgm/ym2612_block_capture_test.cpp)
    add_executable(ym2612_dac_enhanced_test tests/vgm/ym2612_dac_enhanced_test.cpp)
    add_executable(ym2612_dac_block_capture_test tests/vgm/ym2612_dac_block_capture_test.cpp)
    add_executable(ym2612_pcm_stream_test tests/vgm/ym2612_pcm_stream_test.cpp)
    add_executable(ym2612_fm_timeline_test tests/vgm/ym2612_fm_timeline_test.cpp)
    add_executable(genesis_nominal_pitch_test tests/vgm/genesis_nominal_pitch_test.cpp)
    add_executable(smps_programmed_pitch_test tests/vgm/smps_programmed_pitch_test.cpp)
    add_executable(yamaha_nominal_pitch_test tests/vgm/yamaha_nominal_pitch_test.cpp)
    add_executable(tone_generator_nominal_pitch_test tests/vgm/tone_generator_nominal_pitch_test.cpp)
    add_executable(source_stem_mixer_test tests/vgm/source_stem_mixer_test.cpp)
    add_executable(spatial_source_test tests/model/spatial_source_test.cpp)
    add_executable(spatial_source_timed_evidence_test tests/model/spatial_source_timed_evidence_test.cpp)
    add_executable(realtime_spatial_scene_dsp_test tests/vgm/realtime_spatial_scene_dsp_test.cpp)
    add_executable(realtime_musical_spatial_observer_test tests/vgm/realtime_musical_spatial_observer_test.cpp)
    add_executable(realtime_musical_role_hypothesis_test tests/vgm/realtime_musical_role_hypothesis_test.cpp)
    add_executable(realtime_musical_role_tracker_test tests/vgm/realtime_musical_role_tracker_test.cpp)
    add_executable(realtime_musical_spatial_frontend_test tests/vgm/realtime_musical_spatial_frontend_test.cpp)
    add_executable(realtime_auditory_state_evidence_test tests/vgm/realtime_auditory_state_evidence_test.cpp)
    add_executable(omniphony_source_transport_test tests/vgm/omniphony_source_transport_test.cpp)
    add_executable(omniphony_realtime_client_test tests/vgm/omniphony_realtime_client_test.cpp)
    add_executable(realtime_musical_omniphony_pipeline_test tests/vgm/realtime_musical_omniphony_pipeline_test.cpp)
    add_executable(musical_execution_graph_test tests/model/musical_execution_graph_test.cpp)
    add_executable(transformation_identity_test tests/model/transformation_identity_test.cpp)
    add_executable(voice_separation_hypothesis_test tests/model/voice_separation_hypothesis_test.cpp)
    add_executable(musical_inference_evidence_test tests/model/musical_inference_evidence_test.cpp)
    add_executable(musical_understanding_dependency_test tests/model/musical_understanding_dependency_test.cpp)
    add_executable(genesis_nominal_pitch_feature_test tests/model/genesis_nominal_pitch_feature_test.cpp)
    add_executable(analysis_feature_test tests/model/analysis_feature_test.cpp)
    add_executable(source_analysis_feature_adapter_test tests/model/source_analysis_feature_adapter_test.cpp)
    add_executable(tracker_analysis_feature_test tests/model/tracker_analysis_feature_test.cpp)
    add_executable(listener_response_feature_test tests/model/listener_response_feature_test.cpp)
    add_executable(musicological_identity_test tests/model/musicological_identity_test.cpp)
    add_executable(cross_cultural_pitch_rhythm_test tests/model/cross_cultural_pitch_rhythm_test.cpp)
    add_executable(timbre_instrument_identity_test tests/model/timbre_instrument_identity_test.cpp)
    add_executable(creative_role_attribution_test tests/model/creative_role_attribution_test.cpp)
    add_executable(attribution_discovery_control_test tests/model/attribution_discovery_control_test.cpp)
    add_executable(genesis_driver_source_boundary_test tests/model/genesis_driver_source_boundary_test.cpp)
    add_executable(authored_source_projection_test tests/model/authored_source_projection_test.cpp)
    add_executable(tonal_center_hypothesis_test tests/model/tonal_center_hypothesis_test.cpp)
    add_executable(tonal_center_evidence_adapter_test tests/model/tonal_center_evidence_adapter_test.cpp)
    add_executable(diatonic_key_hypothesis_test tests/model/diatonic_key_hypothesis_test.cpp)
    add_executable(pitch_class_collection_adapter_test tests/model/pitch_class_collection_adapter_test.cpp)
    add_executable(vgm_execution_graph_adapter_test tests/vgm/vgm_execution_graph_adapter_test.cpp)
    add_executable(vgm_command_trace_capture_test tests/vgm/vgm_command_trace_capture_test.cpp)
    add_executable(vgm_yamaha_register_write_test tests/vgm/vgm_yamaha_register_write_test.cpp)
    add_executable(vgm_chip_clock_test tests/vgm/vgm_chip_clock_test.cpp)
    add_executable(vgm_dac_stream_command_test tests/vgm/vgm_dac_stream_command_test.cpp)
    add_executable(yamaha_opn_family_test tests/vgm/yamaha_opn_family_test.cpp)
    add_executable(yamaha_opn_register_test tests/vgm/yamaha_opn_register_test.cpp)
    add_executable(yamaha_opm_register_test tests/vgm/yamaha_opm_register_test.cpp)
    add_executable(yamaha_opl_family_test tests/vgm/yamaha_opl_family_test.cpp)
    add_executable(yamaha_opl_register_test tests/vgm/yamaha_opl_register_test.cpp)
    add_executable(yamaha_opll_register_test tests/vgm/yamaha_opll_register_test.cpp)
    add_executable(genesis_execution_graph_adapter_test tests/vgm/genesis_execution_graph_adapter_test.cpp)
    add_executable(genesis_performance_adapter_test tests/vgm/genesis_performance_adapter_test.cpp)
    add_executable(genesis_pitch_control_adapter_test tests/vgm/genesis_pitch_control_adapter_test.cpp)
    add_executable(ym2612_performed_pitch_motion_test tests/vgm/ym2612_performed_pitch_motion_test.cpp)
    add_executable(genesis_performed_part_motif_test tests/vgm/genesis_performed_part_motif_test.cpp)
    add_executable(spc_snapshot_test tests/spc/spc_snapshot_test.cpp)
    add_executable(spc_brr_sample_test tests/spc/spc_brr_sample_test.cpp)
    add_executable(spc_runtime_voice_adapter_test tests/spc/spc_runtime_voice_adapter_test.cpp)
    add_executable(spc_runtime_source_transition_test tests/spc/spc_runtime_source_transition_test.cpp)
    add_executable(spc_ram_generation_test tests/spc/spc_ram_generation_test.cpp)
    add_executable(spc_ram_shadow_test tests/spc/spc_ram_shadow_test.cpp)
    add_executable(spc_runtime_sample_adapter_test tests/spc/spc_runtime_sample_adapter_test.cpp)
    add_executable(spc_runtime_capture_test tests/spc/spc_runtime_capture_test.cpp)
    add_executable(spc_runtime_capture_adapter_test tests/spc/spc_runtime_capture_adapter_test.cpp)
    target_include_directories(genesis_nominal_pitch_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(smps_programmed_pitch_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(spatial_source_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(spatial_source_timed_evidence_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(musical_execution_graph_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(transformation_identity_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(voice_separation_hypothesis_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(musical_inference_evidence_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(musical_understanding_dependency_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(genesis_nominal_pitch_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(analysis_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(source_analysis_feature_adapter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(tracker_analysis_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(listener_response_feature_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(musicological_identity_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(cross_cultural_pitch_rhythm_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(timbre_instrument_identity_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(creative_role_attribution_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(attribution_discovery_control_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(genesis_driver_source_boundary_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(authored_source_projection_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(tonal_center_hypothesis_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(tonal_center_evidence_adapter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(diatonic_key_hypothesis_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(pitch_class_collection_adapter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
    add_test(NAME authored_stereo_route COMMAND authored_stereo_route_test)
    add_test(NAME sn76489_enhanced COMMAND sn76489_enhanced_test)
    add_test(NAME sn76489_variants COMMAND sn76489_variants_test)
    add_test(NAME sn76489_advance COMMAND sn76489_advance_test)
    add_test(NAME sn76489_alias COMMAND sn76489_alias_test)
    add_test(NAME psg_block_capture COMMAND psg_block_capture_test)
    add_test(NAME ym2612_block_capture COMMAND ym2612_block_capture_test)
    add_test(NAME ym2612_dac_enhanced COMMAND ym2612_dac_enhanced_test)
    add_test(NAME ym2612_dac_block_capture COMMAND ym2612_dac_block_capture_test)
    add_test(NAME ym2612_pcm_stream COMMAND ym2612_pcm_stream_test)
    add_test(NAME ym2612_fm_timeline COMMAND ym2612_fm_timeline_test)
    add_test(NAME genesis_nominal_pitch COMMAND genesis_nominal_pitch_test)
    add_test(NAME smps_programmed_pitch COMMAND smps_programmed_pitch_test)
    add_test(NAME yamaha_nominal_pitch COMMAND yamaha_nominal_pitch_test)
    add_test(NAME tone_generator_nominal_pitch COMMAND tone_generator_nominal_pitch_test)
    add_test(NAME source_stem_mixer COMMAND source_stem_mixer_test)
    add_test(NAME spatial_source COMMAND spatial_source_test)
    add_test(NAME spatial_source_timed_evidence COMMAND spatial_source_timed_evidence_test)
    add_test(NAME realtime_spatial_scene_dsp COMMAND realtime_spatial_scene_dsp_test)
    add_test(NAME realtime_musical_spatial_observer COMMAND realtime_musical_spatial_observer_test)
    add_test(NAME realtime_musical_role_hypothesis COMMAND realtime_musical_role_hypothesis_test)
    add_test(NAME realtime_musical_role_tracker COMMAND realtime_musical_role_tracker_test)
    add_test(NAME realtime_musical_spatial_frontend COMMAND realtime_musical_spatial_frontend_test)
    add_test(NAME realtime_auditory_state_evidence COMMAND realtime_auditory_state_evidence_test)
    add_test(NAME omniphony_source_transport COMMAND omniphony_source_transport_test)
    add_test(NAME omniphony_realtime_client COMMAND omniphony_realtime_client_test)
    add_test(NAME realtime_musical_omniphony_pipeline COMMAND realtime_musical_omniphony_pipeline_test)
    add_test(NAME musical_execution_graph COMMAND musical_execution_graph_test)
    add_test(NAME transformation_identity COMMAND transformation_identity_test)
    add_test(NAME voice_separation_hypothesis COMMAND voice_separation_hypothesis_test)
    add_test(NAME musical_inference_evidence COMMAND musical_inference_evidence_test)
    add_test(NAME musical_understanding_dependency COMMAND musical_understanding_dependency_test)
    add_test(NAME genesis_nominal_pitch_feature COMMAND genesis_nominal_pitch_feature_test)
    add_test(NAME analysis_feature COMMAND analysis_feature_test)
    add_test(NAME source_analysis_feature_adapter COMMAND source_analysis_feature_adapter_test)
    add_test(NAME tracker_analysis_feature COMMAND tracker_analysis_feature_test)
    add_test(NAME listener_response_feature COMMAND listener_response_feature_test)
    add_test(NAME musicological_identity COMMAND musicological_identity_test)
    add_test(NAME cross_cultural_pitch_rhythm COMMAND cross_cultural_pitch_rhythm_test)
    add_test(NAME timbre_instrument_identity COMMAND timbre_instrument_identity_test)
    add_test(NAME creative_role_attribution COMMAND creative_role_attribution_test)
    add_test(NAME attribution_discovery_control COMMAND attribution_discovery_control_test)
    add_test(NAME genesis_driver_source_boundary COMMAND genesis_driver_source_boundary_test)
    add_test(NAME authored_source_projection COMMAND authored_source_projection_test)
    add_test(NAME tonal_center_hypothesis COMMAND tonal_center_hypothesis_test)
    add_test(NAME tonal_center_evidence_adapter COMMAND tonal_center_evidence_adapter_test)
    add_test(NAME diatonic_key_hypothesis COMMAND diatonic_key_hypothesis_test)
    add_test(NAME pitch_class_collection_adapter COMMAND pitch_class_collection_adapter_test)
    add_test(NAME vgm_execution_graph_adapter COMMAND vgm_execution_graph_adapter_test)
    add_test(NAME vgm_command_trace_capture COMMAND vgm_command_trace_capture_test)
    add_test(NAME vgm_yamaha_register_write COMMAND vgm_yamaha_register_write_test)
    add_test(NAME vgm_chip_clock COMMAND vgm_chip_clock_test)
    add_test(NAME vgm_dac_stream_command COMMAND vgm_dac_stream_command_test)
    add_test(NAME yamaha_opn_family COMMAND yamaha_opn_family_test)
    add_test(NAME yamaha_opn_register COMMAND yamaha_opn_register_test)
    add_test(NAME yamaha_opm_register COMMAND yamaha_opm_register_test)
    add_test(NAME yamaha_opl_family COMMAND yamaha_opl_family_test)
    add_test(NAME yamaha_opl_register COMMAND yamaha_opl_register_test)
    add_test(NAME yamaha_opll_register COMMAND yamaha_opll_register_test)
    add_test(NAME genesis_execution_graph_adapter COMMAND genesis_execution_graph_adapter_test)
    add_test(NAME genesis_performance_adapter COMMAND genesis_performance_adapter_test)
    add_test(NAME genesis_pitch_control_adapter COMMAND genesis_pitch_control_adapter_test)
    add_test(NAME ym2612_performed_pitch_motion COMMAND ym2612_performed_pitch_motion_test)
    add_test(NAME genesis_performed_part_motif COMMAND genesis_performed_part_motif_test)
    add_test(NAME spc_snapshot COMMAND spc_snapshot_test)
    add_test(NAME spc_brr_sample COMMAND spc_brr_sample_test)
    add_test(NAME spc_runtime_voice_adapter COMMAND spc_runtime_voice_adapter_test)
    add_test(NAME spc_runtime_source_transition COMMAND spc_runtime_source_transition_test)
    add_test(NAME spc_ram_generation COMMAND spc_ram_generation_test)
    add_test(NAME spc_ram_shadow COMMAND spc_ram_shadow_test)
    add_test(NAME spc_runtime_sample_adapter COMMAND spc_runtime_sample_adapter_test)
    add_test(NAME spc_runtime_capture COMMAND spc_runtime_capture_test)
    add_test(NAME spc_runtime_capture_adapter COMMAND spc_runtime_capture_adapter_test)
    add_test(
        NAME xsf_unit
        COMMAND ${Python3_EXECUTABLE} -B -m unittest discover -s tests/xsf -p test_*.py
    )
    set_tests_properties(xsf_unit PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(
        NAME xsf_real_corpus
        COMMAND ${Python3_EXECUTABLE} -B tools/xsf_corpus_audit.py --quiet
            tests/corpus/chrono-cross-psf1-playstation
            tests/corpus/mario-kart-ds-2sf-nintendo-64
            tests/corpus/ocarina-of-time-usf-nintendo-64
    )
    set_tests_properties(xsf_real_corpus PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()