# Host-facing source transport regressions. Keep these separate from the growing
# semantic research inventory so foobar/VGM/SPC integration can evolve without
# repeatedly editing the central test registry.

list(APPEND GAMEAUDIO_TEST_TARGETS
    spatial_source_host_session_test
    spc_runtime_spatial_adapter_test
    spc_runtime_spatial_end_boundary_test
    spc_runtime_host_pipeline_test
    spc_runtime_snesapu_host_pipeline_test
    spc_native_source_capture_test
    spc_native_exact_source_storage_test
    snes_spc_native_source_hook_bridge_test
    spc_enhanced_reconstruction_test
    snes_spc_enhanced_source_hook_bridge_test
    snesapu_source_transport_v2_test
    snesapu_source_object_projection_test
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
add_executable(
    spc_runtime_snesapu_host_pipeline_test
    tests/spc/spc_runtime_snesapu_host_pipeline_test.cpp
)
add_executable(
    spc_native_source_capture_test
    tests/spc/spc_native_source_capture_test.cpp
)
add_executable(
    spc_native_exact_source_storage_test
    tests/spc/spc_native_exact_source_storage_test.cpp
)
add_executable(
    snes_spc_native_source_hook_bridge_test
    tests/spc/snes_spc_native_source_hook_bridge_test.cpp
)
add_executable(
    spc_enhanced_reconstruction_test
    tests/spc/spc_enhanced_reconstruction_test.cpp
)
add_executable(
    snes_spc_enhanced_source_hook_bridge_test
    tests/spc/snes_spc_enhanced_source_hook_bridge_test.cpp
)
add_executable(
    snesapu_source_transport_v2_test
    tests/spc/snesapu_source_transport_v2_test.cpp
)
add_executable(
    snesapu_source_object_projection_test
    tests/spc/snesapu_source_object_projection_test.cpp
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
target_include_directories(
    spc_runtime_snesapu_host_pipeline_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_native_source_capture_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_native_exact_source_storage_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    snes_spc_native_source_hook_bridge_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_enhanced_reconstruction_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    snes_spc_enhanced_source_hook_bridge_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    snesapu_source_transport_v2_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    snesapu_source_object_projection_test
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
add_test(
    NAME spc_runtime_snesapu_host_pipeline
    COMMAND spc_runtime_snesapu_host_pipeline_test
)
add_test(
    NAME spc_native_source_capture
    COMMAND spc_native_source_capture_test
)
add_test(
    NAME spc_native_exact_source_storage
    COMMAND spc_native_exact_source_storage_test
)
add_test(
    NAME snes_spc_native_source_hook_bridge
    COMMAND snes_spc_native_source_hook_bridge_test
)
add_test(
    NAME spc_enhanced_reconstruction
    COMMAND spc_enhanced_reconstruction_test
)
add_test(
    NAME snes_spc_enhanced_source_hook_bridge
    COMMAND snes_spc_enhanced_source_hook_bridge_test
)
add_test(
    NAME snesapu_source_transport_v2
    COMMAND snesapu_source_transport_v2_test
)
add_test(
    NAME snesapu_source_object_projection
    COMMAND snesapu_source_object_projection_test
)
