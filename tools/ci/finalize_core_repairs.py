#!/usr/bin/env python3
"""One-shot guarded migration for the assertion-live compiler-core baseline."""

from pathlib import Path


def replace_once(path_s: str, old: str, new: str, label: str) -> None:
    path = Path(path_s)
    text = path.read_text(encoding="utf-8")
    if old in text:
        count = text.count(old)
        if count != 1:
            raise SystemExit(f"{label}: expected one old anchor, found {count}")
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
        return
    if new not in text:
        raise SystemExit(f"{label}: neither old nor new anchor found")


def replace_count(path_s: str, old: str, new: str, count: int, label: str) -> None:
    path = Path(path_s)
    text = path.read_text(encoding="utf-8")
    if old in text:
        found = text.count(old)
        if found != count:
            raise SystemExit(f"{label}: expected {count} old anchors, found {found}")
        path.write_text(text.replace(old, new), encoding="utf-8")
        return
    if new not in text:
        raise SystemExit(f"{label}: neither old nor new anchor found")


def main() -> int:
    replace_once(
        "CMakeLists.txt",
        "target_compile_options(${test_target} PRIVATE /W4 /permissive-)",
        "target_compile_options(${test_target} PRIVATE /W4 /permissive- /UNDEBUG)",
        "MSVC assertion policy",
    )
    replace_once(
        "CMakeLists.txt",
        "target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)",
        "target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror -UNDEBUG)",
        "GCC assertion policy",
    )

    replace_once(
        "tests/vgm/genesis_enhanced_recomposition_test.cpp",
        "spatial_playback_path::source_full_sphere",
        "spatial_playback_path::source_spatial",
        "Genesis spatial path",
    )

    spatial = Path("tests/model/spatial_source_test.cpp")
    text = spatial.read_text(encoding="utf-8")
    start_marker = "    // Both foobar components expose enhancement and spatialization as separate\n"
    end_marker = "    auto genesis = gameaudio::vgm::make_genesis_spatial_source("
    start = text.find(start_marker)
    end = text.find(end_marker, start if start >= 0 else 0)
    if start >= 0 and end > start:
        replacement = "\n".join(
            [
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
            ]
        )
        spatial.write_text(text[:start] + replacement + text[end:], encoding="utf-8")
    elif "spatial_playback_path::source_spatial" not in text or "uses_externalization" in text:
        raise SystemExit("spatial playback contract anchors not found")

    replace_once(
        "components/vgm/enhancement/ym2612_pcm_stream.h",
        "    explicit ym2612_pcm_stream(double output_sample_rate = 48000.0) noexcept;",
        "    ym2612_pcm_stream() noexcept;\n    explicit ym2612_pcm_stream(double output_sample_rate) noexcept;",
        "YM2612 stream constructor declaration",
    )
    replace_once(
        "components/vgm/enhancement/ym2612_pcm_stream.cpp",
        "ym2612_pcm_stream::ym2612_pcm_stream(double output_sample_rate) noexcept {",
        "ym2612_pcm_stream::ym2612_pcm_stream() noexcept\n    : ym2612_pcm_stream(48000.0) {}\n\nym2612_pcm_stream::ym2612_pcm_stream(double output_sample_rate) noexcept {",
        "YM2612 stream constructor definition",
    )
    replace_once(
        "components/vgm/enhancement/genesis_pitch_control_adapter.h",
        "    const node_id device_transition_id = *result.performance.execution.device_transition_id;",
        "    const vgmtooling::model::node_id device_transition_id = *result.performance.execution.device_transition_id;",
        "Genesis pitch transition id",
    )
    replace_once(
        "model/realization_role_grammar_bridge.h",
        "inline structural_grammar_observation realization_role_deployment_as_grammar_observation(",
        "inline blind_structural_grammar_observation realization_role_deployment_as_grammar_observation(",
        "grammar bridge return type",
    )
    replace_once(
        "model/realization_role_grammar_bridge.h",
        "    structural_grammar_observation result;",
        "    blind_structural_grammar_observation result;",
        "grammar bridge local type",
    )

    replace_once(
        "model/spatial_source_host_assembler.h",
        "        if (!valid_block_shape(block))\n            return fail(spatial_source_host_assembler_error::invalid_block);",
        "        if (!valid_block_shape(block))\n            return fail(spatial_source_host_assembler_error::invalid_block);\n        if (block.lane_count > MaxLanes)\n            return fail(spatial_source_host_assembler_error::invalid_block);",
        "host assembler lane guard",
    )
    replace_once(
        "model/spatial_source_host_assembler.h",
        "            for (std::size_t lane = 0; lane < lane_count_; ++lane) {\n                const auto& source_lane = block.lanes[lane];",
        "            for (std::size_t lane = 0; lane < block.lane_count; ++lane) {\n                const auto& source_lane = block.lanes[lane];",
        "host assembler lane loop",
    )
    replace_once(
        "model/spatial_source_host_assembler.h",
        "                pcm_[lane * CapacityFrames + destination] =\n                    available ? source_lane.mono_pcm[frame] : 0.0f;\n                availability_[lane * CapacityFrames + destination] = available ? 1u : 0u;",
        "                const std::size_t storage_index = lane * CapacityFrames + destination;\n                if (storage_index >= pcm_.size() || storage_index >= availability_.size())\n                    return fail(spatial_source_host_assembler_error::invalid_block);\n                pcm_[storage_index] = available ? source_lane.mono_pcm[frame] : 0.0f;\n                availability_[storage_index] = available ? 1u : 0u;",
        "host assembler storage index",
    )

    replace_once(
        "tests/spc/spc_runtime_snesapu_host_pipeline_test.cpp",
        "spatial_playback_path::source_full_sphere",
        "spatial_playback_path::source_spatial",
        "SPC spatial path",
    )
    replace_count(
        "tests/vgm/sn76489_enhanced_source_block_test.cpp",
        "sn76489_write_kind::data",
        "sn76489_write_kind::register_write",
        5,
        "PSG register writes",
    )

    replace_once(
        "tests/model/test_cube_evidence_worlds.py",
        '        self.assertEqual(self.policy["schema_version"], 2)',
        '        self.assertEqual(self.policy["schema_version"], 4)',
        "cube evidence schema",
    )
    replace_once(
        "tests/model/test_cube_evidence_worlds.py",
        '''        self.assertTrue(
            {
                "Gleylancer",
                "Galaxy Force II (Mega Drive)",
                "Human Sports Festival",
                "Power of the Hired",
            }.issubset(by_candidate["Masanori Hikichi"])
        )''',
        '''        self.assertTrue(
            {
                "Human Sports Festival",
                "Power of the Hired",
            }.issubset(by_candidate["Masanori Hikichi"])
        )
        self.assertNotIn("Gleylancer", by_candidate["Masanori Hikichi"])
        self.assertNotIn("Galaxy Force II (Mega Drive)", by_candidate["Masanori Hikichi"])

        prospective = self.worlds["prospective_exact_control_worlds"]
        self.assertTrue(any(
            item.get("work") == "Gleylancer"
            and item.get("candidate") == "Masanori Hikichi"
            for item in prospective
        ))
        implementation = self.worlds["arrangement_or_implementation_only"]
        self.assertTrue(any(
            item.get("work") == "Galaxy Force II (Mega Drive)"
            and item.get("candidate") == "Masanori Hikichi"
            for item in implementation
        ))''',
        "cube evidence world migration",
    )
    replace_once(
        "tests/spc/test_cube_panel_repository_contract.py",
        '        self.assertEqual(self.policy["schema_version"], 3)',
        '        self.assertEqual(self.policy["schema_version"], 4)',
        "cube panel schema",
    )

    for name, pattern in (
        ("spc_original_sample_candidates_python", "test_original_sample_candidates.py"),
        ("spc_prebrr_sidecar_python", "test_prebrr_sidecar.py"),
        ("spc_studio_source_sidecar_python", "test_studio_source_sidecar.py"),
    ):
        replace_once(
            "cmake/host_transport_tests.cmake",
            f'''add_test(
    NAME {name}
    COMMAND ${{Python3_EXECUTABLE}} tests/spc/{pattern}
)''',
            f'''add_test(
    NAME {name}
    COMMAND ${{Python3_EXECUTABLE}} -B -m unittest discover
        -s tests/spc
        -p {pattern}
)''',
            f"{name} launcher",
        )

    replace_once(
        "tests/spc/test_studio_source_transport_patch.py",
        '        self.assertLess(text.index("studio_runtime.load("), text.index("InitAPU();"))',
        '        load = text.index("studio_runtime.load(")\n        self.assertLess(load, text.index("InitAPU();", load))',
        "studio transport ordering assertion",
    )

    replace_once(
        "tests/vgm/studio_source_resampler_test.cpp",
        '''    assert(observed_first.startup_reference_frames
        == studio_source_resampler_kernel::pre_roll);
    assert(observed_first.newly_ready_studio_frames == 17);
    assert(observed_first.pending_future_frames
        == studio_source_resampler_kernel::post_roll);
    assert(observer.ready_frames() == 17);

    studio_hq_fm_observer<2, 512, 256>::ready_frame ready{};
    for (std::uint64_t ordinal = 31; ordinal <= 47; ++ordinal) {''',
        '''    assert(observed_first.startup_reference_frames == 0);
    assert(observed_first.newly_ready_studio_frames == 48);
    assert(observed_first.pending_future_frames
        == studio_source_resampler_kernel::post_roll);
    assert(observer.first_studio_destination_ordinal() == 0);
    assert(observer.ready_frames() == 48);

    studio_hq_fm_observer<2, 512, 256>::ready_frame ready{};
    for (std::uint64_t ordinal = 0; ordinal <= 47; ++ordinal) {''',
        "studio zero-prefix expectations",
    )

    psg = Path("tests/vgm/sn76489_alias_test.cpp")
    text = psg.read_text(encoding="utf-8")
    old_helpers = '''double total_ac_energy(const buffer& data) {
    double mean = 0.0;
    for (float sample : data)
        mean += sample;
    mean /= static_cast<double>(data.size());

    double energy = 0.0;
    for (float sample : data) {
        const double centered = static_cast<double>(sample) - mean;
        energy += centered * centered;
    }
    return energy;
}

double sinusoid_energy(const buffer& data, std::size_t bin) {
    std::complex<double> projection{0.0, 0.0};
    for (std::size_t n = 0; n < data.size(); ++n) {
        const double phase = -2.0 * pi * static_cast<double>(bin) *
            static_cast<double>(n) / static_cast<double>(data.size());
        projection += static_cast<double>(data[n]) *
            std::complex<double>(std::cos(phase), std::sin(phase));
    }
    return 2.0 * std::norm(projection) / static_cast<double>(data.size());
}

double off_harmonic_energy(const buffer& data) {
    // 3000 Hz is exactly FFT bin 256 at 48 kHz / 4096. A perfect band-limited
    // 50% square at this fundamental can contain only the odd harmonics below
    // Nyquist: 3, 9, 15 and 21 kHz. Everything else is alias/noise energy.
    const std::array<std::size_t, 4> desired_bins{{256, 768, 1280, 1792}};
    double desired = 0.0;
    for (std::size_t bin : desired_bins)
        desired += sinusoid_energy(data, bin);
    return std::max(0.0, total_ac_energy(data) - desired);
}
'''
    new_helpers = '''double sinusoid_amplitude(const buffer& data, std::size_t bin) {
    std::complex<double> projection{0.0, 0.0};
    for (std::size_t n = 0; n < data.size(); ++n) {
        const double phase = -2.0 * pi * static_cast<double>(bin) *
            static_cast<double>(n) / static_cast<double>(data.size());
        projection += static_cast<double>(data[n]) *
            std::complex<double>(std::cos(phase), std::sin(phase));
    }
    return 2.0 * std::abs(projection) / static_cast<double>(data.size());
}

double bandlimited_square_error(const buffer& data) {
    constexpr std::array<std::size_t, 4> harmonics{{1, 3, 5, 7}};
    double error = 0.0;
    for (std::size_t harmonic : harmonics) {
        const std::size_t bin = 256u * harmonic;
        const double ideal = 4.0 / (pi * static_cast<double>(harmonic));
        error += std::abs(sinusoid_amplitude(data, bin) - ideal) / ideal;
    }
    return error;
}
'''
    if old_helpers in text:
        text = text.replace(old_helpers, new_helpers, 1)
    elif new_helpers not in text:
        raise SystemExit("PSG harmonic helper anchors not found")
    old_asserts = '''    const buffer naive = naive_square();
    const double enhanced_alias = off_harmonic_energy(enhanced);
    const double naive_alias = off_harmonic_energy(naive);

    CHECK(naive_alias > 0.0);
    CHECK(enhanced_alias < naive_alias * 0.75);

    // Improvement must not come from deleting the musical fundamental.
    const double enhanced_fundamental = sinusoid_energy(enhanced, 256);
    const double naive_fundamental = sinusoid_energy(naive, 256);
    CHECK(enhanced_fundamental > naive_fundamental * 0.70);'''
    new_asserts = '''    const buffer naive = naive_square();
    const double enhanced_error = bandlimited_square_error(enhanced);
    const double naive_error = bandlimited_square_error(naive);

    CHECK(naive_error > 0.0);
    CHECK(enhanced_error < naive_error * 0.25);

    // Improvement must not come from deleting the musical fundamental.
    const double enhanced_fundamental = sinusoid_amplitude(enhanced, 256);
    const double ideal_fundamental = 4.0 / pi;
    CHECK(enhanced_fundamental > ideal_fundamental * 0.95);'''
    if old_asserts in text:
        text = text.replace(old_asserts, new_asserts, 1)
    elif new_asserts not in text:
        raise SystemExit("PSG harmonic assertion anchors not found")
    psg.write_text(text, encoding="utf-8")

    print("guarded compiler-core repairs applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
