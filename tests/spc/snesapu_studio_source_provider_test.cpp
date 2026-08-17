#include "components/spc/snesapu_studio_source_provider.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>

namespace {
bool near(double a, double b, double tolerance = 2.0e-5) {
    return std::abs(a - b) <= tolerance;
}
}

int main() {
    using namespace gameaudio::spc;

    std::array<float, 96> source{};
    for (std::size_t index = 0; index < source.size(); ++index) {
        source[index] = static_cast<float>(
            0.31 * std::sin(static_cast<double>(index) * 0.27)
            + 0.13 * std::cos(static_cast<double>(index) * 0.61));
    }

    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {1, 2};
    candidate.upstream_identity = {3, 4};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {source.data(), source.size(), 48000.0, 1.0};
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 0.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.0;
    candidate.coordinate_map.loop_present = true;
    candidate.coordinate_map.game_loop_start = 32.0;
    candidate.coordinate_map.upstream_loop_start = 32.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;

    const spc_game_sample_playback_span playback{
        0.0, 64.0, {true, 32.0, 64.0}};
    // 32 decoded samples = two BRR blocks, so the loop target is first + 18.
    const snesapu_studio_source_binding binding{
        7u, 0x4200u, 0x4212u, &candidate, playback};

    snesapu_studio_source_provider<4> provider;
    assert(provider.add(binding));
    assert(provider.source_count() == 1);
    assert(!provider.add(binding)); // one exact runtime identity, one binding
    assert(!provider.begin_voice(0, 7, 0x4201, 0x4212, 0x3c, 0));
    assert(!provider.begin_voice(0, 7, 0x4200, 0x421b, 0x3c, 0));
    assert(!provider.voice_active(0));
    assert(provider.begin_voice(0, 7, 0x4200, 0x4212, 0x3c, 0));
    assert(provider.voice_active(0));

    snesapu_source_trajectory_tracker reference;
    reference.key_on(snesapu_source_interpolation::none);
    const std::array<std::uint32_t, 8> rates{
        0x00010000u,
        0x00008000u,
        0x00018000u,
        0x0000c000u,
        0x00014000u,
        0x00010000u,
        0x00006000u,
        0x0001a000u,
    };

    for (const std::uint32_t rate : rates) {
        const auto projection = reference.project(playback.loop);
        const auto expected = reconstruct_spc_upstream_playback_sample(
            candidate, playback, projection);
        assert(expected.valid);

        float rendered = 0.0f;
        assert(provider.render_voice(0, rate, 0x3c, 0, &rendered));
        assert(near(rendered, expected.sample));
        assert(reference.advance(rate));
    }

    // A live interpolation-mode change changes only the pInter timing center;
    // it must not reset the accumulated PMON/pitch trajectory.
    reference.set_interpolation(snesapu_source_interpolation::gaussian);
    auto projection = reference.project(playback.loop);
    auto expected = reconstruct_spc_upstream_playback_sample(
        candidate, playback, projection);
    assert(expected.valid);
    float rendered = 0.0f;
    assert(provider.render_voice(0, 0x00010000u, 0x3c, 3, &rendered));
    assert(near(rendered, expected.sample));
    assert(reference.advance(0x00010000u));

    // Linear interpolation's historical center begins one source sample before
    // key-on. The provider emits the exact zero-history sample and still
    // advances, so it cannot become permanently stuck at the startup boundary.
    assert(provider.begin_voice(1, 7, 0x4200, 0x4212, 0x3c, 1));
    rendered = 123.0f;
    assert(provider.render_voice(1, 0x00010000u, 0x3c, 1, &rendered));
    assert(rendered == 0.0f);
    assert(provider.render_voice(1, 0x00010000u, 0x3c, 1, &rendered));
    snesapu_source_trajectory_tracker linear_reference;
    linear_reference.key_on(snesapu_source_interpolation::linear);
    assert(linear_reference.advance(0x00010000u));
    projection = linear_reference.project(playback.loop);
    expected = reconstruct_spc_upstream_playback_sample(candidate, playback, projection);
    assert(expected.valid);
    assert(near(rendered, expected.sample));

    // SNESAPU consults DIR again when an END+LOOP is followed. A live DIR change
    // therefore forfeits our authority before the restored trajectory can
    // diverge from the renderer's actual loop target.
    assert(!provider.render_voice(1, 0x00010000u, 0x3d, 1, &rendered));
    assert(!provider.voice_active(1));

    // An impossible rate is also a permanent loss of phase authority for this
    // key-on, so Enhanced substitution deactivates rather than drifting.
    assert(provider.begin_voice(1, 7, 0x4200, 0x4212, 0x3c, 1));
    assert(!provider.render_voice(1, 0x01000000u, 0x3c, 1, &rendered));
    assert(!provider.voice_active(1));

    // A loop locator that disagrees with its 16-sample BRR topology is rejected
    // before playback, even if the upstream candidate itself is valid.
    auto wrong_loop = binding;
    wrong_loop.first_brr_block_address = 0x4300u;
    wrong_loop.loop_brr_block_address = 0x431bu;
    assert(!provider.add(wrong_loop));

    // Fractional upstream loop boundaries require the future virtual-ring path
    // and are rejected at setup rather than during the audio callback.
    auto fractional = candidate;
    fractional.coordinate_map.upstream_frames_per_game_sample = 1.5;
    fractional.coordinate_map.upstream_loop_start = 48.0;
    const snesapu_studio_source_binding unsupported{
        8u,
        0x5000u,
        0x5012u,
        &fractional,
        {0.0, 63.5, {true, 32.0, 63.5}},
    };
    assert(!provider.add(unsupported));

    assert(snesapu_studio_source_provider<4>::begin_callback(
        nullptr, 0, 7, 0x4200, 0x4212, 0x3c, 0) == 0u);
    assert(snesapu_studio_source_provider<4>::sample_callback(
        nullptr, 0, 0x00010000u, 0x3c, 0, &rendered) == 0u);

    provider.stop_voice(0);
    assert(!provider.voice_active(0));
    provider.clear();
    assert(provider.source_count() == 0);

    return 0;
}
