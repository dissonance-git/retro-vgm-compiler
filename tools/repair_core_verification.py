#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    if old in text:
        if text.count(old) != 1:
            raise SystemExit(f"{path}: expected one anchor, found {text.count(old)}")
        write(path, text.replace(old, new, 1))
    elif new not in text:
        raise SystemExit(f"{path}: anchor not found")


def replace_count(path: str, old: str, new: str, count: int) -> None:
    text = read(path)
    if old in text:
        if text.count(old) != count:
            raise SystemExit(f"{path}: expected {count} anchors, found {text.count(old)}")
        write(path, text.replace(old, new))
    elif text.count(new) < count:
        raise SystemExit(f"{path}: replacement not present")


# Release builds must not compile assertions out of tests.
replace_once(
    "CMakeLists.txt",
    "target_compile_options(${test_target} PRIVATE /W4 /permissive-)",
    "target_compile_options(${test_target} PRIVATE /W4 /permissive- /UNDEBUG)",
)
replace_once(
    "CMakeLists.txt",
    "target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)",
    "target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)",
)

# Current spatial API terminology.
replace_once(
    "tests/vgm/genesis_enhanced_recomposition_test.cpp",
    "spatial_playback_path::source_full_sphere",
    "spatial_playback_path::source_spatial",
)
replace_once(
    "tests/spc/spc_runtime_snesapu_host_pipeline_test.cpp",
    "spatial_playback_path::source_full_sphere",
    "spatial_playback_path::source_spatial",
)

# Rewrite the stale public-spatial-options regression around the live two-toggle contract.
path = "tests/model/spatial_source_test.cpp"
text = read(path)
start_marker = "    // Both foobar components expose enhancement and spatialization as separate\n"
end_marker = "    auto genesis = gameaudio::vgm::make_genesis_spatial_source("
start = text.find(start_marker)
end = text.find(end_marker, start if start >= 0 else 0)
if start >= 0 and end > start:
    replacement = "\n".join([
        "    // Both foobar components expose enhancement and spatialization as separate",
        "    // user choices. Renderer topology stays internal to Omniphony.",
        "    vgmtooling::model::spatial_playback_options playback{};",
        "    assert(!playback.surround);",
        "    assert(!playback.enhanced);",
        "    assert(vgmtooling::model::resolve_spatial_playback(playback)",
        "        == vgmtooling::model::spatial_playback_path::reference_stereo);",
        "    assert(!vgmtooling::model::uses_source_renderer(playback));",
        "    assert(!vgmtooling::model::uses_enhanced_renderer(playback));",
        "",
        "    playback.enhanced = true;",
        "    assert(vgmtooling::model::uses_enhanced_renderer(playback));",
        "    assert(vgmtooling::model::resolve_spatial_playback(playback)",
        "        == vgmtooling::model::spatial_playback_path::reference_stereo);",
        "    assert(!vgmtooling::model::uses_source_renderer(playback));",
        "",
        "    playback.surround = true;",
        "    assert(vgmtooling::model::resolve_spatial_playback(playback)",
        "        == vgmtooling::model::spatial_playback_path::source_spatial);",
        "    assert(vgmtooling::model::uses_source_renderer(playback));",
        "    assert(vgmtooling::model::uses_enhanced_renderer(playback));",
        "",
        "    playback.enhanced = false;",
        "    assert(!vgmtooling::model::uses_enhanced_renderer(playback));",
        "    assert(vgmtooling::model::uses_source_renderer(playback));",
        "",
        "    playback.surround = false;",
        "    assert(vgmtooling::model::resolve_spatial_playback(playback)",
        "        == vgmtooling::model::spatial_playback_path::reference_stereo);",
        "    assert(!vgmtooling::model::uses_source_renderer(playback));",
        "",
        "",
    ])
    write(path, text[:start] + replacement + text[end:])
elif "uses_externalization" in text or "spatial_depth_mode" in text:
    raise SystemExit(f"{path}: stale spatial block markers not found")

# True default construction without permitting numeric implicit conversion.
replace_once(
    "components/vgm/enhancement/ym2612_pcm_stream.h",
    "    explicit ym2612_pcm_stream(double output_sample_rate = 48000.0) noexcept;",
    "    ym2612_pcm_stream() noexcept;\n    explicit ym2612_pcm_stream(double output_sample_rate) noexcept;",
)
replace_once(
    "components/vgm/enhancement/ym2612_pcm_stream.cpp",
    "ym2612_pcm_stream::ym2612_pcm_stream(double output_sample_rate) noexcept {",
    "ym2612_pcm_stream::ym2612_pcm_stream() noexcept\n    : ym2612_pcm_stream(48000.0) {}\n\nym2612_pcm_stream::ym2612_pcm_stream(double output_sample_rate) noexcept {",
)

replace_once(
    "components/vgm/enhancement/genesis_pitch_control_adapter.h",
    "    const node_id device_transition_id = *result.performance.execution.device_transition_id;",
    "    const vgmtooling::model::node_id device_transition_id = *result.performance.execution.device_transition_id;",
)
replace_once(
    "model/realization_role_grammar_bridge.h",
    "inline structural_grammar_observation realization_role_deployment_as_grammar_observation(",
    "inline blind_structural_grammar_observation realization_role_deployment_as_grammar_observation(",
)
replace_once(
    "model/realization_role_grammar_bridge.h",
    "    structural_grammar_observation result;",
    "    blind_structural_grammar_observation result;",
)

# Localize the host assembler storage proof at the write boundary.
replace_once(
    "model/spatial_source_host_assembler.h",
    "        if (!valid_block_shape(block))\n            return fail(spatial_source_host_assembler_error::invalid_block);",
    "        if (!valid_block_shape(block))\n            return fail(spatial_source_host_assembler_error::invalid_block);\n        if (block.lane_count > MaxLanes)\n            return fail(spatial_source_host_assembler_error::invalid_block);",
)
replace_once(
    "model/spatial_source_host_assembler.h",
    "            for (std::size_t lane = 0; lane < lane_count_; ++lane) {\n                const auto& source_lane = block.lanes[lane];",
    "            for (std::size_t lane = 0; lane < block.lane_count; ++lane) {\n                const auto& source_lane = block.lanes[lane];",
)
replace_once(
    "model/spatial_source_host_assembler.h",
    "                pcm_[lane * CapacityFrames + destination] =\n                    available ? source_lane.mono_pcm[frame] : 0.0f;\n                availability_[lane * CapacityFrames + destination] = available ? 1u : 0u;",
    "                const std::size_t storage_index = lane * CapacityFrames + destination;\n                if (storage_index >= pcm_.size() || storage_index >= availability_.size())\n                    return fail(spatial_source_host_assembler_error::invalid_block);\n                pcm_[storage_index] = available ? source_lane.mono_pcm[frame] : 0.0f;\n                availability_[storage_index] = available ? 1u : 0u;",
)

replace_count(
    "tests/vgm/sn76489_enhanced_source_block_test.cpp",
    "sn76489_write_kind::data",
    "sn76489_write_kind::register_write",
    5,
)

# Policy tests follow the current schema-4 evidence partition. Gleylancer moved
# to prospective exact controls; Galaxy Force II is an implementation confound.
replace_once(
    "tests/model/test_cube_evidence_worlds.py",
    'self.assertEqual(self.policy["schema_version"], 2)',
    'self.assertEqual(self.policy["schema_version"], 4)',
)
replace_once(
    "tests/model/test_cube_evidence_worlds.py",
    '''            {\n                "Gleylancer",\n                "Galaxy Force II (Mega Drive)",\n                "Human Sports Festival",\n                "Power of the Hired",\n            }.issubset(by_candidate["Masanori Hikichi"])''',
    '''            {\n                "Human Sports Festival",\n                "Power of the Hired",\n            }.issubset(by_candidate["Masanori Hikichi"])''',
)
replace_once(
    "tests/spc/test_cube_panel_repository_contract.py",
    'self.assertEqual(self.policy["schema_version"], 3)',
    'self.assertEqual(self.policy["schema_version"], 4)',
)

# Direct-script execution puts tests/spc, not repository root, on sys.path.
# Use the same unittest-discovery form as the rest of the Python suite.
for test_name, script_name in (
    ("spc_original_sample_candidates_python", "test_original_sample_candidates.py"),
    ("spc_prebrr_sidecar_python", "test_prebrr_sidecar.py"),
    ("spc_studio_source_sidecar_python", "test_studio_source_sidecar.py"),
):
    replace_once(
        "cmake/host_transport_tests.cmake",
        f"    NAME {test_name}\n    COMMAND ${{Python3_EXECUTABLE}} tests/spc/{script_name}",
        f"    NAME {test_name}\n    COMMAND ${{Python3_EXECUTABLE}} -B -m unittest discover\n        -s tests/spc\n        -p {script_name}",
    )

# The patch script contains both predecessor and successor strings. Compare the
# studio load against the next InitAPU in the successor block, not the old anchor.
replace_once(
    "tests/spc/test_studio_source_transport_patch.py",
    '        self.assertLess(text.index("studio_runtime.load("), text.index("InitAPU();"))',
    '        load = text.index("studio_runtime.load(")\n        init_after_load = text.index("InitAPU();", load)\n        self.assertLess(load, init_after_load)',
)

# Initial device attachment has a proven zero-valued negative-time FM prefix,
# so the first 31 destination frames no longer need protected-reference delay.
replace_once(
    "tests/vgm/studio_source_resampler_test.cpp",
    "    assert(observed_first.startup_reference_frames\n        == studio_source_resampler_kernel::pre_roll);\n    assert(observed_first.newly_ready_studio_frames == 17);",
    "    assert(observed_first.startup_reference_frames == 0);\n    assert(observed_first.newly_ready_studio_frames == 48);",
)
replace_once(
    "tests/vgm/studio_source_resampler_test.cpp",
    "    assert(observer.ready_frames() == 17);",
    "    assert(observer.ready_frames() == 48);",
)
replace_once(
    "tests/vgm/studio_source_resampler_test.cpp",
    "    for (std::uint64_t ordinal = 31; ordinal <= 47; ++ordinal) {",
    "    for (std::uint64_t ordinal = 0; ordinal <= 47; ++ordinal) {",
)

# A period-16 square's aliased energy folds onto the same FFT bins as its legal
# harmonics, so the old off-bin metric was identically zero. Measure closeness
# to the ideal band-limited square harmonic series instead.
path = "tests/vgm/sn76489_alias_test.cpp"
text = read(path)
old_helper = '''double off_harmonic_energy(const buffer& data) {\n    // 3000 Hz is exactly FFT bin 256 at 48 kHz / 4096. A perfect band-limited\n    // 50% square at this fundamental can contain only the odd harmonics below\n    // Nyquist: 3, 9, 15 and 21 kHz. Everything else is alias/noise energy.\n    const std::array<std::size_t, 4> desired_bins{{256, 768, 1280, 1792}};\n    double desired = 0.0;\n    for (std::size_t bin : desired_bins)\n        desired += sinusoid_energy(data, bin);\n    return std::max(0.0, total_ac_energy(data) - desired);\n}\n'''
new_helper = '''double bandlimited_square_shape_error(const buffer& data) {\n    const std::array<std::size_t, 4> bins{{256, 768, 1280, 1792}};\n    const std::array<unsigned, 4> harmonics{{1, 3, 5, 7}};\n    double error = 0.0;\n    for (std::size_t i = 0; i < bins.size(); ++i) {\n        const double ideal_amplitude = 4.0 / (pi * static_cast<double>(harmonics[i]));\n        const double ideal_energy = static_cast<double>(frames)\n            * ideal_amplitude * ideal_amplitude * 0.5;\n        error += std::abs(sinusoid_energy(data, bins[i]) - ideal_energy);\n    }\n    return error;\n}\n'''
if old_helper in text:
    text = text.replace(old_helper, new_helper, 1)
elif new_helper not in text:
    raise SystemExit(f"{path}: alias metric helper anchor not found")
old_main = '''    const buffer naive = naive_square();\n    const double enhanced_alias = off_harmonic_energy(enhanced);\n    const double naive_alias = off_harmonic_energy(naive);\n\n    CHECK(naive_alias > 0.0);\n    CHECK(enhanced_alias < naive_alias * 0.75);\n\n    // Improvement must not come from deleting the musical fundamental.\n    const double enhanced_fundamental = sinusoid_energy(enhanced, 256);\n    const double naive_fundamental = sinusoid_energy(naive, 256);\n    CHECK(enhanced_fundamental > naive_fundamental * 0.70);\n'''
new_main = '''    const buffer naive = naive_square();\n    const double enhanced_shape_error = bandlimited_square_shape_error(enhanced);\n    const double naive_shape_error = bandlimited_square_shape_error(naive);\n\n    CHECK(naive_shape_error > 0.0);\n    CHECK(enhanced_shape_error < naive_shape_error * 0.25);\n\n    // Improvement must not come from deleting the musical fundamental.\n    const double enhanced_fundamental = sinusoid_energy(enhanced, 256);\n    const double naive_fundamental = sinusoid_energy(naive, 256);\n    CHECK(enhanced_fundamental > naive_fundamental * 0.95);\n'''
if old_main in text:
    text = text.replace(old_main, new_main, 1)
elif new_main not in text:
    raise SystemExit(f"{path}: alias metric main anchor not found")
write(path, text)

print("core verification repair staged")
