# Host-facing source transport regressions. Keep these separate from the growing
# semantic research inventory so foobar/VGM/SPC integration can evolve without
# repeatedly editing the central test registry.

list(APPEND GAMEAUDIO_TEST_TARGETS
    spatial_source_host_session_test
    spc_runtime_spatial_adapter_test
    spc_runtime_spatial_end_boundary_test
    spc_runtime_host_pipeline_test
)

add_executable(
    spatial_source_host_session_test
    tests/model/spatial_source_host_session_test.cpp
)
add_executable(
    spc_runtime_spatial_adapter_test
    tests/spc/spc_runtime_spatial_adapter_test.cpp
)
add_executable(
    spc_runtime_spatial_end_boundary_test
    tests/spc/spc_runtime_spatial_end_boundary_test.cpp
)
add_executable(
    spc_runtime_host_pipeline_test
    tests/spc/spc_runtime_host_pipeline_test.cpp
)

target_include_directories(
    spatial_source_host_session_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_runtime_spatial_adapter_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_runtime_spatial_end_boundary_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_runtime_host_pipeline_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME spatial_source_host_session
    COMMAND spatial_source_host_session_test
)
add_test(
    NAME spc_runtime_spatial_adapter
    COMMAND spc_runtime_spatial_adapter_test
)
add_test(
    NAME spc_runtime_spatial_end_boundary
    COMMAND spc_runtime_spatial_end_boundary_test
)
add_test(
    NAME spc_runtime_host_pipeline
    COMMAND spc_runtime_host_pipeline_test
)
