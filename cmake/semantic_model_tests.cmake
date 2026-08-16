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
