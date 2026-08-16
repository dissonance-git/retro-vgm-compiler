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
