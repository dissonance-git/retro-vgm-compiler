# Dependency-free semantic-model regressions that sit above the realtime core.
#
# Keep this file append-only in spirit: semantic research layers can register
# their own strict C++17 tests here without repeatedly rewriting the large root
# CMake target inventory. Every target appended here is linked and compiled by
# the root GAMEAUDIO_TEST_TARGETS foreach loop.

list(APPEND GAMEAUDIO_TEST_TARGETS
    tonal_region_evidence_adapter_test
    harmonic_pitch_class_collection_adapter_test
    diatonic_chord_degree_hypothesis_test
    structural_composer_grammar_bridge_test
    role_scoped_orchestration_grammar_test
    realization_role_deployment_test
    cross_realization_part_correspondence_test
    genesis_psg_analysis_features_test
    execution_semantic_provenance_test
    execution_semantic_dialect_test
    execution_semantic_scope_test
)

add_executable(
    tonal_region_evidence_adapter_test
    tests/model/tonal_region_evidence_adapter_test.cpp
)
add_executable(
    harmonic_pitch_class_collection_adapter_test
    tests/model/harmonic_pitch_class_collection_adapter_test.cpp
)
add_executable(
    diatonic_chord_degree_hypothesis_test
    tests/model/diatonic_chord_degree_hypothesis_test.cpp
)
add_executable(
    structural_composer_grammar_bridge_test
    tests/model/structural_composer_grammar_bridge_test.cpp
)
add_executable(
    role_scoped_orchestration_grammar_test
    tests/model/role_scoped_orchestration_grammar_test.cpp
)
add_executable(
    realization_role_deployment_test
    tests/model/realization_role_deployment_test.cpp
)
add_executable(
    cross_realization_part_correspondence_test
    tests/model/cross_realization_part_correspondence_test.cpp
)
add_executable(
    genesis_psg_analysis_features_test
    tests/vgm/genesis_psg_analysis_features_test.cpp
)
add_executable(
    execution_semantic_provenance_test
    tests/model/execution_semantic_provenance_test.cpp
)
add_executable(
    execution_semantic_dialect_test
    tests/model/execution_semantic_dialect_test.cpp
)
add_executable(
    execution_semantic_scope_test
    tests/model/execution_semantic_scope_test.cpp
)

target_include_directories(
    tonal_region_evidence_adapter_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    harmonic_pitch_class_collection_adapter_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    diatonic_chord_degree_hypothesis_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    structural_composer_grammar_bridge_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    role_scoped_orchestration_grammar_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    realization_role_deployment_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    cross_realization_part_correspondence_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    genesis_psg_analysis_features_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    execution_semantic_provenance_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    execution_semantic_dialect_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    execution_semantic_scope_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME tonal_region_evidence_adapter
    COMMAND tonal_region_evidence_adapter_test
)
add_test(
    NAME harmonic_pitch_class_collection_adapter
    COMMAND harmonic_pitch_class_collection_adapter_test
)
add_test(
    NAME diatonic_chord_degree_hypothesis
    COMMAND diatonic_chord_degree_hypothesis_test
)
add_test(
    NAME structural_composer_grammar_bridge
    COMMAND structural_composer_grammar_bridge_test
)
add_test(
    NAME role_scoped_orchestration_grammar
    COMMAND role_scoped_orchestration_grammar_test
)
add_test(
    NAME realization_role_deployment
    COMMAND realization_role_deployment_test
)
add_test(
    NAME cross_realization_part_correspondence
    COMMAND cross_realization_part_correspondence_test
)
add_test(
    NAME genesis_psg_analysis_features
    COMMAND genesis_psg_analysis_features_test
)
add_test(
    NAME execution_semantic_provenance
    COMMAND execution_semantic_provenance_test
)
add_test(
    NAME execution_semantic_dialect
    COMMAND execution_semantic_dialect_test
)
add_test(
    NAME execution_semantic_scope
    COMMAND execution_semantic_scope_test
)

add_test(
    NAME sonic3_harmonic_probe_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_sonic3_harmonic_probe.py
)
set_tests_properties(
    sonic3_harmonic_probe_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME genesis_psg_semantics_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_genesis_psg_semantics.py
)
set_tests_properties(
    genesis_psg_semantics_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME sonic3_mixed_harmonic_probe_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_sonic3_mixed_harmonic_probe.py
)
set_tests_properties(
    sonic3_mixed_harmonic_probe_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

list(APPEND GAMEAUDIO_TEST_TARGETS
    blind_attribution_experiment_test
)

add_executable(
    blind_attribution_experiment_test
    tests/model/blind_attribution_experiment_test.cpp
)

target_include_directories(
    blind_attribution_experiment_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME blind_attribution_experiment
    COMMAND blind_attribution_experiment_test
)

add_test(
    NAME attribution_control_registry_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/model
        -p test_attribution_control_registry.py
)
set_tests_properties(
    attribution_control_registry_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME blind_attribution_match_manifest_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/model
        -p test_blind_attribution_match_manifest.py
)
set_tests_properties(
    blind_attribution_match_manifest_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME cube_calibration_admissions_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/model
        -p test_cube_calibration_admissions.py
)
set_tests_properties(
    cube_calibration_admissions_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME cube_evidence_worlds_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/model
        -p test_cube_evidence_worlds.py
)
set_tests_properties(
    cube_evidence_worlds_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME sonic3_cube_target_panel_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/model
        -p test_sonic3_cube_target_panel.py
)
set_tests_properties(
    sonic3_cube_target_panel_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

list(APPEND GAMEAUDIO_TEST_TARGETS
    spc_label_blind_corpus_features_test
    part_motif_attribution_bridge_test
    spc_runtime_trace_replay_test
    spc_runtime_trace_recorder_test
)

add_executable(
    spc_label_blind_corpus_features_test
    tests/spc/spc_label_blind_corpus_features_test.cpp
)
add_executable(
    part_motif_attribution_bridge_test
    tests/model/part_motif_attribution_bridge_test.cpp
)
add_executable(
    spc_runtime_trace_replay_test
    tests/spc/spc_runtime_trace_replay_test.cpp
)
add_executable(
    spc_runtime_trace_recorder_test
    tests/spc/spc_runtime_trace_recorder_test.cpp
)

target_include_directories(
    spc_label_blind_corpus_features_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    part_motif_attribution_bridge_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_runtime_trace_replay_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_runtime_trace_recorder_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME spc_label_blind_corpus_features
    COMMAND spc_label_blind_corpus_features_test
)
add_test(
    NAME part_motif_attribution_bridge
    COMMAND part_motif_attribution_bridge_test
)
add_test(
    NAME spc_runtime_trace_replay
    COMMAND spc_runtime_trace_replay_test
)
add_test(
    NAME spc_runtime_trace_recorder
    COMMAND spc_runtime_trace_recorder_test
)

add_test(
    NAME spc_freeze_forensic_sidecars_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/spc
        -p test_freeze_forensic_sidecars.py
)
set_tests_properties(
    spc_freeze_forensic_sidecars_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME cube_blind_panel_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/spc
        -p test_cube_blind_panel.py
)
set_tests_properties(
    cube_blind_panel_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME cube_panel_repository_contract_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/spc
        -p test_cube_panel_repository_contract.py
)
set_tests_properties(
    cube_panel_repository_contract_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME spc_snapshot_state_correlation_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/spc
        -p test_correlate_spc_snapshot_state.py
)
set_tests_properties(
    spc_snapshot_state_correlation_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

list(APPEND GAMEAUDIO_TEST_TARGETS
    spatial_source_host_assembler_test
)

add_executable(
    spatial_source_host_assembler_test
    tests/model/spatial_source_host_assembler_test.cpp
)

target_include_directories(
    spatial_source_host_assembler_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME spatial_source_host_assembler
    COMMAND spatial_source_host_assembler_test
)

include(cmake/host_transport_tests.cmake)

# Source-native Enhanced synthesis regressions. Keep these here rather than in
# the root target inventory so enhancement research can evolve without churning
# the large core CMake surface.
list(APPEND GAMEAUDIO_TEST_TARGETS
    ym2612_hq_fm_backend_test
    spc_sample_restoration_policy_test
    ym2612_hq_algorithm_test
    studio_alignment_queue_test
    studio_source_stream_test
    studio_source_timeline_test
    studio_source_resampler_test
    studio_hq_fm_observer_rebase_test
    studio_frame_transport_test
)

add_executable(
    ym2612_hq_fm_backend_test
    tests/vgm/ym2612_hq_fm_backend_test.cpp
)
add_executable(
    spc_sample_restoration_policy_test
    tests/spc/spc_sample_restoration_policy_test.cpp
)
add_executable(
    ym2612_hq_algorithm_test
    tests/vgm/ym2612_hq_algorithm_test.cpp
)
add_executable(
    studio_alignment_queue_test
    tests/vgm/studio_alignment_queue_test.cpp
)
add_executable(
    studio_source_stream_test
    tests/vgm/studio_source_stream_test.cpp
)
add_executable(
    studio_source_timeline_test
    tests/vgm/studio_source_timeline_test.cpp
)
add_executable(
    studio_source_resampler_test
    tests/vgm/studio_source_resampler_test.cpp
)
add_executable(
    studio_hq_fm_observer_rebase_test
    tests/vgm/studio_hq_fm_observer_rebase_test.cpp
)
add_executable(
    studio_frame_transport_test
    tests/vgm/studio_frame_transport_test.cpp
)

target_include_directories(
    ym2612_hq_fm_backend_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_sample_restoration_policy_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ym2612_hq_algorithm_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_alignment_queue_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_source_stream_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_source_timeline_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_source_resampler_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_hq_fm_observer_rebase_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    studio_frame_transport_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME ym2612_hq_fm_backend
    COMMAND ym2612_hq_fm_backend_test
)
add_test(
    NAME spc_sample_restoration_policy
    COMMAND spc_sample_restoration_policy_test
)
add_test(
    NAME ym2612_hq_algorithm
    COMMAND ym2612_hq_algorithm_test
)
add_test(
    NAME studio_alignment_queue
    COMMAND studio_alignment_queue_test
)
add_test(
    NAME studio_source_stream
    COMMAND studio_source_stream_test
)
add_test(
    NAME studio_source_timeline
    COMMAND studio_source_timeline_test
)
add_test(
    NAME studio_source_resampler
    COMMAND studio_source_resampler_test
)
add_test(
    NAME studio_hq_fm_observer_rebase
    COMMAND studio_hq_fm_observer_rebase_test
)
add_test(
    NAME studio_frame_transport
    COMMAND studio_frame_transport_test
)

add_test(
    NAME studio_hq_fm_observer_patch_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_studio_hq_fm_observer_patch.py
)
set_tests_properties(
    studio_hq_fm_observer_patch_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME studio_hq_fm_runtime_patch_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_studio_hq_fm_runtime_patch.py
)
set_tests_properties(
    studio_hq_fm_runtime_patch_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME playera_deferred_postrender_patch_py
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s tests/vgm
        -p test_playera_deferred_postrender_patch.py
)
set_tests_properties(
    playera_deferred_postrender_patch_py
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
