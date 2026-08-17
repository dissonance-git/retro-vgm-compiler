#include "components/spc/spc_snesapu_source_trajectory.h"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace {
bool near(double a, double b) {
    return std::abs(a - b) < 1.0e-12;
}
}

int main() {
    using namespace gameaudio::spc;

    static_assert(snesapu_interpolation_phase_offset_samples(
        snesapu_source_interpolation::none) == 0);
    static_assert(snesapu_interpolation_phase_offset_samples(
        snesapu_source_interpolation::linear) == -1);
    static_assert(snesapu_interpolation_phase_offset_samples(
        snesapu_source_interpolation::cubic) == 1);
    static_assert(snesapu_interpolation_phase_offset_samples(
        snesapu_source_interpolation::gaussian) == 1);
    static_assert(snesapu_interpolation_phase_offset_samples(
        snesapu_source_interpolation::sinc8) == -1);

    snesapu_source_trajectory_tracker tracker;
    tracker.key_on(snesapu_source_interpolation::cubic);
    auto p = tracker.project();
    assert(p.valid);
    assert(near(p.accumulator_sample_position, 0.0));
    assert(near(p.effective_sample_position, 1.0));

    // 1.5 source samples per output frame. This is the exact recurrence used by
    // mDec + whole-sample carry, expressed without decode-window pointer state.
    assert(tracker.advance(0x00018000u));
    p = tracker.project();
    assert(near(p.accumulator_sample_position, 1.5));
    assert(near(p.effective_sample_position, 2.5));

    assert(tracker.advance(0x00018000u));
    p = tracker.project();
    assert(near(p.accumulator_sample_position, 3.0));
    assert(near(p.effective_sample_position, 4.0));

    // Pinned UpdateSrc only consumes one whole-sample byte.
    assert(!tracker.advance(0x01000000u));

    // Linear and 8-point sinc expose a one-sample startup history delay. That
    // must remain explicit so an upstream replacement does not invent audio
    // before key-on.
    tracker.key_on(snesapu_source_interpolation::linear);
    p = tracker.project();
    assert(p.before_key_on);
    assert(near(p.effective_sample_position, -1.0));
    assert(tracker.advance(0x00010000u));
    p = tracker.project();
    assert(!p.before_key_on);
    assert(near(p.effective_sample_position, 0.0));

    // Loop projection uses an unwrapped trajectory. The first pass remains
    // linear, then the canonical game coordinate wraps [16, 32) forever.
    const snesapu_game_loop_span loop{true, 16.0, 32.0};
    tracker.key_on(snesapu_source_interpolation::none);
    for (int i = 0; i < 31; ++i)
        assert(tracker.advance(0x00010000u));
    p = tracker.project(loop);
    assert(near(p.effective_sample_position, 31.0));
    assert(near(p.canonical_game_sample_position, 31.0));
    assert(p.loop_cycle == 0);

    assert(tracker.advance(0x00010000u));
    p = tracker.project(loop);
    assert(near(p.effective_sample_position, 32.0));
    assert(near(p.canonical_game_sample_position, 16.0));
    assert(p.loop_cycle == 1);

    for (int i = 0; i < 16; ++i)
        assert(tracker.advance(0x00010000u));
    p = tracker.project(loop);
    assert(near(p.effective_sample_position, 48.0));
    assert(near(p.canonical_game_sample_position, 16.0));
    assert(p.loop_cycle == 2);

    // Cubic/Gaussian center timing reaches the loop topology one source sample
    // earlier than the raw accumulator because the historical pInter output is
    // centered one sample ahead.
    tracker.key_on(snesapu_source_interpolation::gaussian);
    for (int i = 0; i < 31; ++i)
        assert(tracker.advance(0x00010000u));
    p = tracker.project(loop);
    assert(near(p.accumulator_sample_position, 31.0));
    assert(near(p.effective_sample_position, 32.0));
    assert(near(p.canonical_game_sample_position, 16.0));
    assert(p.loop_cycle == 1);

    return 0;
}
