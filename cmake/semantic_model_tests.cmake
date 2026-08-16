# Dependency-free semantic-model regressions that sit above the realtime core.
#
# Keep this file append-only in spirit: semantic research layers can register
# their own strict C++17 tests here without repeatedly rewriting the large root
# CMake target inventory. Every target appended here is linked and compiled by
# the root GAMEAUDIO_TEST_TARGETS foreach loop.

list(APPEND GAMEAUDIO_TEST_TARGETS
    tonal_region_evidence_adapter_test
)

add_executable(
    tonal_region_evidence_adapter_test
    tests/model/tonal_region_evidence_adapter_test.cpp
)

target_include_directories(
    tonal_region_evidence_adapter_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME tonal_region_evidence_adapter
    COMMAND tonal_region_evidence_adapter_test
)
