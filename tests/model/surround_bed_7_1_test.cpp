#include "../../model/surround_bed_7_1.h"

#include <array>
#include <cassert>
#include <cmath>

namespace {
bool near(float a, float b, float tolerance = 1.0e-6f) {
    return std::fabs(a - b) <= tolerance;
}
}

int main() {
    using namespace vgmtooling::model;

    constexpr std::size_t frame_count = 2;
    const std::array<double, frame_count * 2> reference{
        0.80, -0.60,
        -0.25, 0.50,
    };
    const std::array<float, frame_count> wet_left{0.20f, -0.05f};
    const std::array<float, frame_count> wet_right{-0.10f, 0.15f};

    surround_7_1_bed_storage<2> bed{};
    assert(bed.begin_from_interleaved_stereo(reference.data(), frame_count));
    assert(bed.valid());
    assert(bed.frame_count() == frame_count);
    assert(bed.move_stereo_to_surround_field(
        wet_left.data(), wet_right.data(), frame_count));

    const float* out = bed.data();
    assert(out != nullptr);
    const auto at = [&](std::size_t frame, surround_7_1_channel channel) {
        return out[frame * surround_7_1_channel_count +
            surround_7_1_bed_storage<2>::index(channel)];
    };

    // The protected front remainder stays in front.
    assert(near(at(0, surround_7_1_channel::front_left), 0.60f));
    assert(near(at(0, surround_7_1_channel::front_right), -0.50f));
    assert(near(at(0, surround_7_1_channel::front_center), 0.0f));
    assert(near(at(0, surround_7_1_channel::lfe), 0.0f));

    // One linked wet field is divided by power across rear + side.
    assert(near(
        at(0, surround_7_1_channel::back_left),
        wet_left[0] * surround_equal_power_split));
    assert(near(
        at(0, surround_7_1_channel::side_left),
        wet_left[0] * surround_equal_power_split));
    assert(near(
        at(0, surround_7_1_channel::back_right),
        wet_right[0] * surround_equal_power_split));
    assert(near(
        at(0, surround_7_1_channel::side_right),
        wet_right[0] * surround_equal_power_split));

    // A conventional -3 dB surround fold reconstructs the original stereo
    // exactly for this shared-field operation.
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const float folded_left =
            at(frame, surround_7_1_channel::front_left) +
            surround_equal_power_split * (
                at(frame, surround_7_1_channel::back_left) +
                at(frame, surround_7_1_channel::side_left));
        const float folded_right =
            at(frame, surround_7_1_channel::front_right) +
            surround_equal_power_split * (
                at(frame, surround_7_1_channel::back_right) +
                at(frame, surround_7_1_channel::side_right));
        assert(near(folded_left, static_cast<float>(reference[frame * 2u])));
        assert(near(folded_right, static_cast<float>(reference[frame * 2u + 1u])));
    }

    // Exact isolated stereo families can instead move to one speaker pair.
    const std::array<float, frame_count> source_left{0.10f, 0.02f};
    const std::array<float, frame_count> source_right{0.05f, -0.03f};
    assert(bed.move_stereo_to_sides(source_left.data(), source_right.data(), frame_count));
    assert(near(at(0, surround_7_1_channel::front_left), 0.50f));
    assert(near(
        at(0, surround_7_1_channel::side_left),
        wet_left[0] * surround_equal_power_split + source_left[0]));

    return 0;
}
