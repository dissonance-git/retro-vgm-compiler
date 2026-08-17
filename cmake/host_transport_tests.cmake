# Host-facing source transport regressions. Keep these separate from the growing
# semantic research inventory so foobar/VGM/SPC integration can evolve without
# repeatedly editing the central test registry.

list(APPEND GAMEAUDIO_TEST_TARGETS
    spatial_source_host_session_test
)

add_executable(
    spatial_source_host_session_test
    tests/model/spatial_source_host_session_test.cpp
)

target_include_directories(
    spatial_source_host_session_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME spatial_source_host_session
    COMMAND spatial_source_host_session_test
)
