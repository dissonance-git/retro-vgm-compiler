#include "components/spc/spc_upstream_playback_reconstruction.h"

#include <array>
#include <cassert>
#include <cmath>

namespace {
using namespace gameaudio::spc;

void assert_same(
    const spc_sample_restoration_candidate& candidate,
    const spc_game_sample_playback_span& playback,
    const snesapu_source_trajectory_projection& trajectory)
{
    const auto boundaries = detail::resolve_spc_upstream_playback_boundaries(
        candidate, playback);
    assert(boundaries.valid);

    const auto ordinary = reconstruct_spc_upstream_playback_sample(
        candidate, playback, trajectory);
    const auto resolved = detail::reconstruct_spc_upstream_candidate_playback_sample_resolved(
        candidate, playback, trajectory, boundaries);
    assert(ordinary.valid == resolved.valid);
    if (ordinary.valid)
        assert(std::abs(ordinary.sample - resolved.sample) < 1.0e-12);
}

spc_sample_restoration_candidate make_candidate(
    const float* source,
    std::size_t frames,
    bool loop,
    double loop_start)
{
    spc_sample_restoration_candidate candidate;
    candidate.game_brr_identity = {1, 2};
    candidate.upstream_identity = {3, 4};
    candidate.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    candidate.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    candidate.upstream = {source, frames, 48000.0, 1.0};
    candidate.coordinate_map.game_origin = 0.0;
    candidate.coordinate_map.upstream_origin = 0.0;
    candidate.coordinate_map.upstream_frames_per_game_sample = 1.0;
    candidate.coordinate_map.loop_present = loop;
    candidate.coordinate_map.game_loop_start = loop ? loop_start : 0.0;
    candidate.coordinate_map.upstream_loop_start = loop ? loop_start : 0.0;
    candidate.coordinate_map.preparation_chain_exact = true;
    candidate.identity_validation_passed = true;
    return candidate;
}
}

int main() {
    using namespace gameaudio::spc;

    std::array<float, 256> source{};
    for (std::size_t i = 0; i < source.size(); ++i) {
        source[i] = static_cast<float>(
            0.41 * std::sin(static_cast<double>(i) * 0.071)
            - 0.16 * std::cos(static_cast<double>(i) * 0.193));
    }

    snesapu_source_trajectory_tracker tracker;

    // Startup zero-history boundary.
    const auto one_shot = make_candidate(source.data(), source.size(), false, 0.0);
    const spc_game_sample_playback_span one_shot_playback{0.0, 128.0, {}};
    tracker.key_on(snesapu_source_interpolation::none);
    assert(tracker.advance(0x00004000u)); // 0.25
    auto trajectory = tracker.project();
    assert(trajectory.valid);
    assert_same(one_shot, one_shot_playback, trajectory);

    // Steady one-shot interior uses the contiguous 64-tap path.
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 64; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x00004000u)); // 64.25
    trajectory = tracker.project();
    assert(trajectory.valid);
    assert_same(one_shot, one_shot_playback, trajectory);

    // One-shot END boundary zero-pads the FIR's unavailable right side.
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 127; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x0000c000u)); // 127.75
    trajectory = tracker.project();
    assert(trajectory.valid);
    assert_same(one_shot, one_shot_playback, trajectory);

    // First-pass END+LOOP seam keeps pre-loop left history while right lookahead
    // wraps to the authored loop start.
    const auto looped = make_candidate(source.data(), source.size(), true, 64.0);
    const spc_game_sample_playback_span looped_playback{
        0.0, 128.0, {true, 64.0, 128.0}};
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 127; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x0000c000u));
    trajectory = tracker.project(looped_playback.loop);
    assert(trajectory.valid && trajectory.loop_cycle == 0);
    assert_same(looped, looped_playback, trajectory);

    // After wrapping, an interior FIR window is periodic inside the loop body.
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 160; ++i)
        assert(tracker.advance(0x00010000u));
    assert(tracker.advance(0x00004000u)); // canonical loop position 96.25
    trajectory = tracker.project(looped_playback.loop);
    assert(trajectory.valid && trajectory.loop_cycle == 1);
    assert(std::abs(trajectory.canonical_game_sample_position - 96.25) < 1.0e-12);
    assert_same(looped, looped_playback, trajectory);

    // Invalid/fractional geometry still fails during setup resolution and cannot
    // become a cached realtime plan.
    auto fractional = looped;
    fractional.coordinate_map.upstream_frames_per_game_sample = 1.5;
    fractional.coordinate_map.upstream_loop_start = 96.0;
    const spc_game_sample_playback_span fractional_playback{
        0.0, 127.5, {true, 64.0, 127.5}};
    const auto bad = detail::resolve_spc_upstream_playback_boundaries(
        fractional, fractional_playback);
    assert(!bad.valid);

    return 0;
}
