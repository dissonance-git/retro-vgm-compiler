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
    spc_enhanced_native_interval_test
    snes_spc_enhanced_source_hook_bridge_test
    snesapu_source_transport_v2_test
    snesapu_source_object_projection_test
    snesapu_prebrr_provider_test
    spc_sample_restoration_test
    spc_sample_lineage_verification_test
    spc_original_sample_bank_test
    genesis_enhanced_recomposition_test
    sn76489_enhanced_source_block_test
    ym2612_hq_fm_profile_test
    ym2612_hq_fm_backend_test
    ym2612_hq_source_calibration_test
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
    spc_enhanced_native_interval_test
    tests/spc/spc_enhanced_native_interval_test.cpp
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
add_executable(
    snesapu_prebrr_provider_test
    tests/spc/snesapu_prebrr_provider_test.cpp
)
add_executable(
    spc_sample_restoration_test
    tests/spc/spc_sample_restoration_test.cpp
)
add_executable(
    spc_sample_lineage_verification_test
    tests/spc/spc_sample_lineage_verification_test.cpp
)
add_executable(
    spc_original_sample_bank_test
    tests/spc/spc_original_sample_bank_test.cpp
)
add_executable(
    genesis_enhanced_recomposition_test
    tests/vgm/genesis_enhanced_recomposition_test.cpp
)
add_executable(
    sn76489_enhanced_source_block_test
    tests/vgm/sn76489_enhanced_source_block_test.cpp
)
add_executable(
    ym2612_hq_fm_profile_test
    tests/vgm/ym2612_hq_fm_profile_test.cpp
)
add_executable(
    ym2612_hq_fm_backend_test
    tests/vgm/ym2612_hq_fm_backend_test.cpp
)
add_executable(
    ym2612_hq_source_calibration_test
    tests/vgm/ym2612_hq_source_calibration_test.cpp
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
    spc_enhanced_native_interval_test
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
target_include_directories(
    snesapu_prebrr_provider_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_sample_restoration_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_sample_lineage_verification_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    spc_original_sample_bank_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    genesis_enhanced_recomposition_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    sn76489_enhanced_source_block_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ym2612_hq_fm_profile_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ym2612_hq_fm_backend_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    ym2612_hq_source_calibration_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    sn76489_enhanced_source_block_test
    PRIVATE gameaudio_vgm_core
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
    NAME spc_enhanced_native_interval
    COMMAND spc_enhanced_native_interval_test
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
add_test(
    NAME snesapu_prebrr_provider
    COMMAND snesapu_prebrr_provider_test
)
add_test(
    NAME spc_sample_restoration
    COMMAND spc_sample_restoration_test
)
add_test(
    NAME spc_sample_lineage_verification
    COMMAND spc_sample_lineage_verification_test
)
add_test(
    NAME spc_original_sample_bank
    COMMAND spc_original_sample_bank_test
)
add_test(
    NAME genesis_enhanced_recomposition
    COMMAND genesis_enhanced_recomposition_test
)
add_test(
    NAME sn76489_enhanced_source_block
    COMMAND sn76489_enhanced_source_block_test
)
add_test(
    NAME ym2612_hq_fm_profile
    COMMAND ym2612_hq_fm_profile_test
)
add_test(
    NAME ym2612_hq_fm_backend
    COMMAND ym2612_hq_fm_backend_test
)
add_test(
    NAME ym2612_hq_source_calibration
    COMMAND ym2612_hq_source_calibration_test
)
