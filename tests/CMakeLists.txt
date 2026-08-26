# Test identity is derived from the test tree whenever that relation is exact.
# Build scope is narrower: root C++ tests must also be dependency-free. Tests
# that compile against patched upstream source are classified explicitly below
# and belong to external-source verification, not this root projection.

function(vgm_compiler_add_cpp_test source)
    get_filename_component(test_target "${source}" NAME_WE)
    if(TARGET ${test_target})
        message(FATAL_ERROR "duplicate C++ test target: ${test_target}")
    endif()

    add_executable(${test_target} "${source}")
    target_include_directories(${test_target} PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(${test_target} PRIVATE vgm_compiler_core)
    target_compile_features(${test_target} PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(${test_target} PRIVATE /W4 /permissive- /UNDEBUG)
    else()
        target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)
    endif()

    string(REGEX REPLACE "_test$" "" ctest_name "${test_target}")
    add_test(NAME ${ctest_name} COMMAND ${test_target})
endfunction()

function(vgm_compiler_add_python_unittest test_name directory pattern)
    add_test(
        NAME ${test_name}
        COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
            -s tests/${directory}
            -p ${pattern}
    )
    set_tests_properties(
        ${test_name}
        PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endfunction()

function(vgm_compiler_add_python_test test_name directory script)
    add_test(
        NAME ${test_name}
        COMMAND ${Python3_EXECUTABLE} tests/${directory}/${script}
    )
    set_tests_properties(
        ${test_name}
        PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endfunction()

file(GLOB VGM_COMPILER_CPP_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/model/*_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vgm/*_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spc/*_test.cpp"
)

# These contracts compile the patched upstream snes_spc API itself. Their test
# identity is still derivable from *_test.cpp, but their dependency class is not.
# Keep that distinction explicit instead of pretending filename shape implies a
# dependency-free build contract.
set(VGM_COMPILER_EXTERNAL_SOURCE_CPP_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/spc/snes_spc_forensic_bridge_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spc/snes_spc_native_spatial_observer_test.cpp"
)
list(REMOVE_ITEM
    VGM_COMPILER_CPP_TEST_SOURCES
    ${VGM_COMPILER_EXTERNAL_SOURCE_CPP_TEST_SOURCES}
)
list(SORT VGM_COMPILER_CPP_TEST_SOURCES)

foreach(test_source IN LISTS VGM_COMPILER_CPP_TEST_SOURCES)
    vgm_compiler_add_cpp_test("${test_source}")
endforeach()

# Curated Python contracts whose runtime assumptions are known to CTest.
vgm_compiler_add_python_unittest(sonic3_harmonic_probe_py vgm test_sonic3_harmonic_probe.py)
vgm_compiler_add_python_unittest(genesis_psg_semantics_py vgm test_genesis_psg_semantics.py)
vgm_compiler_add_python_unittest(sonic3_mixed_harmonic_probe_py vgm test_sonic3_mixed_harmonic_probe.py)
vgm_compiler_add_python_unittest(attribution_control_registry_py model test_attribution_control_registry.py)
vgm_compiler_add_python_unittest(blind_attribution_match_manifest_py model test_blind_attribution_match_manifest.py)
vgm_compiler_add_python_unittest(cube_calibration_admissions_py model test_cube_calibration_admissions.py)
vgm_compiler_add_python_unittest(cube_evidence_worlds_py model test_cube_evidence_worlds.py)
vgm_compiler_add_python_unittest(sonic3_cube_target_panel_py model test_sonic3_cube_target_panel.py)
vgm_compiler_add_python_unittest(spc_freeze_forensic_sidecars_py spc test_freeze_forensic_sidecars.py)
vgm_compiler_add_python_unittest(cube_blind_panel_py spc test_cube_blind_panel.py)
vgm_compiler_add_python_unittest(cube_panel_repository_contract_py spc test_cube_panel_repository_contract.py)
vgm_compiler_add_python_unittest(spc_snapshot_state_correlation_py spc test_correlate_spc_snapshot_state.py)
vgm_compiler_add_python_unittest(studio_hq_fm_observer_patch_py vgm test_studio_hq_fm_observer_patch.py)
vgm_compiler_add_python_unittest(studio_hq_fm_runtime_patch_py vgm test_studio_hq_fm_runtime_patch.py)
vgm_compiler_add_python_unittest(playera_deferred_postrender_patch_py vgm test_playera_deferred_postrender_patch.py)
vgm_compiler_add_python_unittest(spc_original_sample_candidates_python spc test_original_sample_candidates.py)
vgm_compiler_add_python_unittest(spc_prebrr_sidecar_python spc test_prebrr_sidecar.py)
vgm_compiler_add_python_unittest(spc_studio_source_sidecar_python spc test_studio_source_sidecar.py)
vgm_compiler_add_python_test(spc_studio_source_provider_patch_python spc test_studio_source_provider_patch.py)
vgm_compiler_add_python_test(spc_studio_source_transport_patch_python spc test_studio_source_transport_patch.py)
vgm_compiler_add_python_test(spc_enhanced_runtime_rate_python spc test_enhanced_runtime_rate.py)
