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

    assert(!timeline.has_history(0));
    assert(timeline.source_frames_required(0) == 0);
    assert(!timeline.ready(0, 1000));

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

    double previous = timeline.source_position(0);
    for (std::uint64_t frame = 1; frame < 1000; ++frame) {
        const double current = timeline.source_position(frame);
        assert(std::isfinite(current));
        assert(current > previous);
        previous = current;
    }

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

    assert(!plan_studio_source_window(NAN).valid);
    assert(!plan_studio_source_window(INFINITY).valid);

    studio_source_timeline upsample;
    assert(upsample.configure(44100.0, 96000.0));
    assert(upsample.source_position(2) < 1.0);
    assert(!upsample.has_history(2));

    // Exact libvgm-linear upsampling phase: Resmpl_Init pre-generates one real
    // native sample into history.next; history.last is the protected startup
    // zero. Newly_captured[0] is therefore native ordinal one, and destination
    // frame zero still points at that one-sample startup history coordinate.
    studio_linear_timing_snapshot up{};
    up.source_rate_hz = 53267;
    up.destination_rate_hz = 96000;
    up.sample_p = 0;
    up.sample_next = 1;
    const auto up0 = studio_linear_source_position(up, 1, 0);
    assert(up0.valid);
    assert(up0.phase_units == -static_cast<std::int64_t>(
        studio_source_resampler_kernel::phase_count));

    const auto up1 = studio_linear_source_position(up, 1, 1);
    assert(up1.valid);
    const std::uint64_t up_fp =
        (static_cast<std::uint64_t>(1) * (1u << 11) * up.source_rate_hz)
        / up.destination_rate_hz;
    assert(up1.phase_units == static_cast<std::int64_t>(up_fp * 2u)
        - static_cast<std::int64_t>(studio_source_resampler_kernel::phase_count));

    // The same destination instant is invariant if the source capture is split
    // after frame 50. native_base advances by exactly the native frames pulled
    // by the first segment, while sample_p/sample_next carry libvgm's phase.
    constexpr std::uint64_t split = 50;
    const std::uint64_t split_fp =
        ((split - 1u) * (1u << 11) * up.source_rate_hz) / up.destination_rate_hz;
    const std::uint32_t split_next = static_cast<std::uint32_t>(
        (split_fp + (1u << 11) - 1u) / (1u << 11));
    studio_linear_timing_snapshot up_after = up;
    up_after.sample_p = static_cast<std::uint32_t>(split);
    up_after.sample_next = split_next;
    const std::uint64_t pulled = split_next - up.sample_next;

    const auto whole = studio_linear_source_position(up, 1, split + 17);
    const auto split_view = studio_linear_source_position(
        up_after, 1 + pulled, 17);
    assert(whole.valid && split_view.valid);
    assert(whole.phase_units == split_view.phase_units);

    // Downsampling uses the midpoint of libvgm's exact box-average interval.
    // Its 1/2048 boundaries become exact 1/4096 Studio phase units.
    studio_linear_timing_snapshot down{};
    down.source_rate_hz = 53267;
    down.destination_rate_hz = 44100;
    down.sample_p = 0;
    down.sample_last = 0;
    const auto down0 = studio_linear_source_position(down, 0, 0);
    assert(down0.valid);
    const std::uint64_t rate_fp = (1u << 11) * down.source_rate_hz;
    const std::uint64_t b0 = (1u << 11);
    const std::uint64_t b1 = b0 + rate_fp / down.destination_rate_hz;
    assert(down0.phase_units == static_cast<std::int64_t>(b0 + b1)
        - static_cast<std::int64_t>(studio_source_resampler_kernel::phase_count));

    // Quantized negative startup coordinates remain representable for planning
    // but correctly expose missing history.
    studio_source_phase_position negative{
        -static_cast<std::int64_t>(studio_source_resampler_kernel::phase_count) / 2,
        true
    };
    const auto negative_window = plan_studio_source_window(negative);
    assert(negative_window.valid);
    assert(negative_window.center == -1);
    assert(negative_window.phase == studio_source_resampler_kernel::phase_count / 2);
    assert(negative_window.first < 0);

    return 0;
}
