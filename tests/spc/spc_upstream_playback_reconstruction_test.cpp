#include "components/spc/spc_upstream_playback_reconstruction.h"

#include <array>
#include <cassert>
#include <cmath>

namespace {
bool far(double a, double b) {
    return std::abs(a - b) > 1.0e-3;
}
}

int main() {
    using namespace gameaudio::spc;

    std::array<float, 96> source{};
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (i < 32)
            source[i] = static_cast<float>(-0.2 + 0.005 * static_cast<double>(i));
        else if (i < 64)
            source[i] = static_cast<float>(0.35 * std::sin(static_cast<double>(i) * 0.31));
        else
            source[i] = 0.9f; // deliberately not part of the authored loop
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

    snesapu_source_trajectory_tracker tracker;
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 63; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x0000c000u)); // 63.75: FIR right side crosses END+LOOP
    auto trajectory = tracker.project(playback.loop);
    assert(trajectory.valid && trajectory.loop_cycle == 0);

    const auto topology_tail = reconstruct_spc_upstream_playback_sample(
        candidate, playback, trajectory);
    const auto contiguous_tail = reconstruct_spc_studio_sample(
        source.data(), source.size(), 63.75);
    assert(topology_tail.valid && contiguous_tail.valid);
    assert(far(topology_tail.sample, contiguous_tail.sample));

    // Just after the first wrap, left-side FIR history must come from the loop
    // tail, not from pre-loop frame 31.
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 64; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x00004000u)); // unwrapped 64.25 -> canonical 32.25
    trajectory = tracker.project(playback.loop);
    assert(trajectory.valid && trajectory.loop_cycle == 1);
    assert(std::abs(trajectory.canonical_game_sample_position - 32.25) < 1.0e-12);

    const auto topology_head = reconstruct_spc_upstream_playback_sample(
        candidate, playback, trajectory);
    const auto contiguous_head = reconstruct_spc_studio_sample(
        source.data(), source.size(), 32.25);
    assert(topology_head.valid && contiguous_head.valid);
    assert(far(topology_head.sample, contiguous_head.sample));

    // Key-on is an authored boundary. A trimmed source beginning at upstream
    // frame 16 must not leak frames 0..15 into the long symmetric FIR.
    std::array<float, 96> trimmed{};
    for (std::size_t i = 0; i < 16; ++i)
        trimmed[i] = 1.0f;
    spc_sample_restoration_candidate trimmed_candidate = candidate;
    trimmed_candidate.upstream = {trimmed.data(), trimmed.size(), 48000.0, 1.0};
    trimmed_candidate.coordinate_map.upstream_origin = 16.0;
    trimmed_candidate.coordinate_map.loop_present = false;
    trimmed_candidate.coordinate_map.upstream_loop_start = 0.0;

    const spc_game_sample_playback_span one_shot{0.0, 48.0, {}};
    tracker.key_on(snesapu_source_interpolation::none);
    assert(tracker.advance(0x00004000u)); // game 0.25 -> upstream 16.25
    trajectory = tracker.project();
    const auto topology_start = reconstruct_spc_upstream_playback_sample(
        trimmed_candidate, one_shot, trajectory);
    const auto contiguous_start = reconstruct_spc_studio_sample(
        trimmed.data(), trimmed.size(), 16.25);
    assert(topology_start.valid && contiguous_start.valid);
    assert(std::abs(topology_start.sample) < 1.0e-9);
    assert(std::abs(contiguous_start.sample) > 1.0e-3);

    // Fractional upstream loop boundaries are not approximated. They require a
    // resampled virtual ring and deliberately fail closed in this implementation.
    auto fractional = candidate;
    fractional.coordinate_map.upstream_frames_per_game_sample = 1.5;
    fractional.coordinate_map.upstream_loop_start = 48.0;
    const spc_game_sample_playback_span fractional_playback{
        0.0, 63.5, {true, 32.0, 63.5}};
    tracker.key_on(snesapu_source_interpolation::none);
    trajectory = tracker.project(fractional_playback.loop);
    assert(!reconstruct_spc_upstream_playback_sample(
        fractional, fractional_playback, trajectory).valid);

    return 0;
}
