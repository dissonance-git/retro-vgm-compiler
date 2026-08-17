#include "components/vgm/foo_input_vgm/src/studio_source_timeline.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

int main() {
    using namespace foobar_vgm::source_audio;

    studio_source_timeline timeline;
    assert(!timeline.configure(0.0, 44100.0));
    assert(!timeline.configured());
    assert(timeline.configure(53267.0, 44100.0));
    assert(timeline.configured());

    // FIR warm-up is explicit. The first destination frame would require
    // source history before frame zero, so Enhanced must remain reference.
    assert(!timeline.has_history(0));
    assert(timeline.source_frames_required(0) == 0);
    assert(!timeline.ready(0, 1000));

    // Once the center has moved beyond pre_roll, readiness is determined only
    // by whether the exact future source frame required by post_roll exists.
    constexpr std::uint64_t destination_frame = 64;
    const auto planned = timeline.window(destination_frame);
    assert(planned.valid);
    assert(planned.first >= 0);
    assert(planned.final > planned.center);
    const std::uint64_t required = timeline.source_frames_required(destination_frame);
    assert(required == static_cast<std::uint64_t>(planned.final) + 1);
    assert(required > 0);
    assert(!timeline.ready(destination_frame, required - 1));
    assert(timeline.ready(destination_frame, required));

    // Destination ordinals map monotonically into the authoritative native
    // source clock. This is the shared scheduling coordinate for all six FM
    // lanes, not six independent resampler clocks.
    double previous = timeline.source_position(0);
    for (std::uint64_t frame = 1; frame < 1000; ++frame) {
        const double current = timeline.source_position(frame);
        assert(std::isfinite(current));
        assert(current > previous);
        previous = current;
    }

    // Phase quantization can round a source position to the next integer. The
    // scheduling plan must include that same center increment or it would
    // release a destination frame one native sample too early.
    constexpr double almost_next = 100.0
        + (static_cast<double>(studio_source_resampler_kernel::phase_count) - 0.25)
            / static_cast<double>(studio_source_resampler_kernel::phase_count);
    const auto rounded = plan_studio_source_window(almost_next);
    assert(rounded.valid);
    assert(rounded.phase == 0);
    assert(rounded.center == 101);
    assert(rounded.first == 101 - static_cast<std::ptrdiff_t>(
        studio_source_resampler_kernel::pre_roll));
    assert(rounded.final == 101 + static_cast<std::ptrdiff_t>(
        studio_source_resampler_kernel::post_roll));

    // Invalid coordinates fail closed rather than manufacturing a timing plan.
    assert(!plan_studio_source_window(NAN).valid);
    assert(!plan_studio_source_window(INFINITY).valid);

    // Upsampling still carries the same history/lookahead contract even though
    // destination ordinals advance more slowly through the source coordinate.
    studio_source_timeline upsample;
    assert(upsample.configure(44100.0, 96000.0));
    assert(upsample.source_position(2) < 1.0);
    assert(!upsample.has_history(2));

    return 0;
}
