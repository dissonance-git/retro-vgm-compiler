#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace vgmtooling::model {

// Standard foobar / WAVEFORMATEXTENSIBLE 7.1 channel order. This is a speaker
// bed contract, not a source ontology or inferred 3-D scene.
enum class surround_7_1_channel : std::size_t {
    front_left = 0,
    front_right,
    front_center,
    lfe,
    back_left,
    back_right,
    side_left,
    side_right,
};

constexpr std::size_t surround_7_1_channel_count = 8u;
constexpr float surround_equal_power_split = 0.7071067811865475f;

template <std::size_t MaxFrames>
class surround_7_1_bed_storage {
    static_assert(MaxFrames > 0, "7.1 bed requires frame capacity");

public:
    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
    }

    template <typename Sample>
    bool begin_from_interleaved_stereo(
        const Sample* reference,
        std::size_t frame_count) noexcept
    {
        reset();
        if (reference == nullptr || frame_count == 0 || frame_count > MaxFrames)
            return false;

        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            const double left = static_cast<double>(reference[frame * 2u]);
            const double right = static_cast<double>(reference[frame * 2u + 1u]);
            if (!std::isfinite(left) || !std::isfinite(right))
                return false;

            float* destination = data_.data() + frame * surround_7_1_channel_count;
            for (std::size_t channel = 0; channel < surround_7_1_channel_count; ++channel)
                destination[channel] = 0.0f;
            destination[index(surround_7_1_channel::front_left)] = static_cast<float>(left);
            destination[index(surround_7_1_channel::front_right)] = static_cast<float>(right);
        }

        frame_count_ = frame_count;
        valid_ = true;
        return true;
    }

    // Remove one exact stereo source contribution from the protected front mix
    // and place it on the side pair at unity. The source must already be in the
    // same final gain/routing domain as the protected stereo.
    bool move_stereo_to_sides(
        const float* left,
        const float* right,
        std::size_t frame_count) noexcept
    {
        return move_stereo_to_pair(
            left, right, frame_count,
            surround_7_1_channel::side_left,
            surround_7_1_channel::side_right,
            1.0f);
    }

    // Same law for the rear pair.
    bool move_stereo_to_backs(
        const float* left,
        const float* right,
        std::size_t frame_count) noexcept
    {
        return move_stereo_to_pair(
            left, right, frame_count,
            surround_7_1_channel::back_left,
            surround_7_1_channel::back_right,
            1.0f);
    }

    // Rebalance one exact source from the protected front stereo into a
    // front-anchored horizontal 7.1 spread. The three gains describe the
    // destination amplitude on the same left/right half of the bed. This is
    // intentionally phase-free and delay-free so callers can implement
    // constant-power synth-style spreading without changing source timbre.
    bool redistribute_stereo_to_depth(
        const float* left,
        const float* right,
        std::size_t frame_count,
        float front_gain,
        float side_gain,
        float back_gain) noexcept
    {
        if (!source_valid(left, right, frame_count)
            || !std::isfinite(front_gain)
            || !std::isfinite(side_gain)
            || !std::isfinite(back_gain)
            || front_gain < 0.0f
            || side_gain < 0.0f
            || back_gain < 0.0f)
            return false;

        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            const float source_left = left[frame];
            const float source_right = right[frame];
            if (!std::isfinite(source_left) || !std::isfinite(source_right))
                return false;

            float* destination = data_.data() + frame * surround_7_1_channel_count;

            // begin_from_interleaved_stereo() already contains this source at
            // unity in front. Replace that unity contribution with the chosen
            // front anchor, then add side/back energy.
            destination[index(surround_7_1_channel::front_left)] +=
                source_left * (front_gain - 1.0f);
            destination[index(surround_7_1_channel::front_right)] +=
                source_right * (front_gain - 1.0f);
            destination[index(surround_7_1_channel::side_left)] +=
                source_left * side_gain;
            destination[index(surround_7_1_channel::side_right)] +=
                source_right * side_gain;
            destination[index(surround_7_1_channel::back_left)] +=
                source_left * back_gain;
            destination[index(surround_7_1_channel::back_right)] +=
                source_right * back_gain;
        }
        return finite_output();
    }

    // Shared environmental returns are fields rather than point sources. Remove
    // the exact wet contribution from the protected front mix once, then divide
    // it equally by power between side and rear speakers on the same authored
    // left/right half. No synthetic decorrelation, delay or phase trick is added.
    bool move_stereo_to_surround_field(
        const float* left,
        const float* right,
        std::size_t frame_count) noexcept
    {
        if (!source_valid(left, right, frame_count))
            return false;

        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            const float source_left = left[frame];
            const float source_right = right[frame];
            if (!std::isfinite(source_left) || !std::isfinite(source_right))
                return false;

            float* destination = data_.data() + frame * surround_7_1_channel_count;
            destination[index(surround_7_1_channel::front_left)] -= source_left;
            destination[index(surround_7_1_channel::front_right)] -= source_right;
            destination[index(surround_7_1_channel::back_left)] +=
                source_left * surround_equal_power_split;
            destination[index(surround_7_1_channel::back_right)] +=
                source_right * surround_equal_power_split;
            destination[index(surround_7_1_channel::side_left)] +=
                source_left * surround_equal_power_split;
            destination[index(surround_7_1_channel::side_right)] +=
                source_right * surround_equal_power_split;
        }
        return finite_output();
    }

    bool valid() const noexcept { return valid_; }
    std::size_t frame_count() const noexcept { return valid_ ? frame_count_ : 0u; }
    const float* data() const noexcept { return valid_ ? data_.data() : nullptr; }
    float* data() noexcept { return valid_ ? data_.data() : nullptr; }

    static constexpr std::size_t index(surround_7_1_channel channel) noexcept {
        return static_cast<std::size_t>(channel);
    }

private:
    bool source_valid(
        const float* left,
        const float* right,
        std::size_t frame_count) const noexcept
    {
        return valid_ && left != nullptr && right != nullptr &&
            frame_count == frame_count_;
    }

    bool move_stereo_to_pair(
        const float* left,
        const float* right,
        std::size_t frame_count,
        surround_7_1_channel left_destination,
        surround_7_1_channel right_destination,
        float gain) noexcept
    {
        if (!source_valid(left, right, frame_count) || !std::isfinite(gain))
            return false;

        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            const float source_left = left[frame];
            const float source_right = right[frame];
            if (!std::isfinite(source_left) || !std::isfinite(source_right))
                return false;

            float* destination = data_.data() + frame * surround_7_1_channel_count;
            destination[index(surround_7_1_channel::front_left)] -= source_left;
            destination[index(surround_7_1_channel::front_right)] -= source_right;
            destination[index(left_destination)] += source_left * gain;
            destination[index(right_destination)] += source_right * gain;
        }
        return finite_output();
    }

    bool finite_output() const noexcept {
        if (!valid_)
            return false;
        const std::size_t samples = frame_count_ * surround_7_1_channel_count;
        for (std::size_t sample = 0; sample < samples; ++sample) {
            if (!std::isfinite(data_[sample]))
                return false;
        }
        return true;
    }

    std::array<float, MaxFrames * surround_7_1_channel_count> data_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
};

} // namespace vgmtooling::model
