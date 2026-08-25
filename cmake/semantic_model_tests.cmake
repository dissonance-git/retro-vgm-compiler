# Dependency-free semantic-model regressions that sit above the realtime core.
#
# The core registry owns established semantic-model tests. This entry point
# composes it with the cadence-focused semantic regressions below.
include(${CMAKE_CURRENT_LIST_DIR}/semantic_model_tests_core.cmake)

# Cadence evidence and Ionian cadence-form regressions.
list(APPEND GAMEAUDIO_TEST_TARGETS
    cadential_arrival_hypothesis_test
    cadential_formal_closure_evidence_test
    ionian_functional_tendency_hypothesis_test
    ionian_cadence_class_hypothesis_test
    ionian_cadence_formal_binding_test
    ionian_deferred_authentic_resolution_test
    ionian_deceptive_cadence_candidate_test
    ionian_cadence_phrase_arbitration_test
)

add_executable(
    cadential_arrival_hypothesis_test
    tests/model/cadential_arrival_hypothesis_test.cpp
)
add_executable(
    cadential_formal_closure_evidence_test
    tests/model/cadential_formal_closure_evidence_test.cpp
)
add_executable(
    ionian_functional_tendency_hypothesis_test
    tests/model/ionian_functional_tendency_hypothesis_test.cpp
)
add_executable(
    ionian_cadence_class_hypothesis_test
    tests/model/ionian_cadence_class_hypothesis_test.cpp
)
add_executable(
    ionian_cadence_formal_binding_test
    tests/model/ionian_cadence_formal_binding_test.cpp
)
add_executable(
    ionian_deferred_authentic_resolution_test
    tests/model/ionian_deferred_authentic_resolution_test.cpp
)
add_executable(
    ionian_deceptive_cadence_candidate_test
    tests/model/ionian_deceptive_cadence_candidate_test.cpp
)
add_executable(
    ionian_cadence_phrase_arbitration_test
    tests/model/ionian_cadence_phrase_arbitration_test.cpp
)

target_include_directories(
    cadential_arrival_hypothesis_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    cadential_formal_closure_evidence_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_functional_tendency_hypothesis_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_cadence_class_hypothesis_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_cadence_formal_binding_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_deferred_authentic_resolution_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_deceptive_cadence_candidate_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ionian_cadence_phrase_arbitration_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME cadential_arrival_hypothesis
    COMMAND cadential_arrival_hypothesis_test
)
add_test(
    NAME cadential_formal_closure_evidence
    COMMAND cadential_formal_closure_evidence_test
)
add_test(
    NAME ionian_functional_tendency_hypothesis
    COMMAND ionian_functional_tendency_hypothesis_test
)
add_test(
    NAME ionian_cadence_class_hypothesis
    COMMAND ionian_cadence_class_hypothesis_test
)
add_test(
    NAME ionian_cadence_formal_binding
    COMMAND ionian_cadence_formal_binding_test
)
add_test(
    NAME ionian_deferred_authentic_resolution
    COMMAND ionian_deferred_authentic_resolution_test
)
add_test(
    NAME ionian_deceptive_cadence_candidate
    COMMAND ionian_deceptive_cadence_candidate_test
)
add_test(
    NAME ionian_cadence_phrase_arbitration
    COMMAND ionian_cadence_phrase_arbitration_test
)
