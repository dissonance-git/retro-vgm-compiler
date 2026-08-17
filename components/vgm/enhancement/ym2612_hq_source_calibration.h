#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

struct ym2612_hq_source_calibration {
    double left_gain = 0.0;
    double right_gain = 0.0;
    double hq_rms = 0.0;
    double reference_left_rms = 0.0;
    double reference_right_rms = 0.0;
    std::size_t active_frames = 0;
    bool valid = false;
};

// Convert the normalized HQ FM stem into the same unit domain as the exact
// source contribution already present in libvgm's protected mix. This is unit
// calibration, not timbre matching: one positive RMS-derived scalar is fitted
// per authored output side and then applied to the whole block.
inline ym2612_hq_source_calibration calibrate_ym2612_hq_source(
    const float* hq_mono,
    const std::int32_t* reference_left,
    const std::int32_t* reference_right,
    std::size_t frames,
    double activity_floor = 1.0e-6) noexcept
{
    ym2612_hq_source_calibration result;
    if (hq_mono == nullptr || reference_left == nullptr || reference_right == nullptr
        || frames == 0 || !std::isfinite(activity_floor) || activity_floor < 0.0)
        return result;

    long double hq_energy = 0.0L;
    long double left_energy = 0.0L;
    long double right_energy = 0.0L;
    std::size_t active = 0;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double hq = static_cast<double>(hq_mono[frame]);
        if (!std::isfinite(hq))
            return {};
        if (std::abs(hq) > activity_floor)
            ++active;
        hq_energy += static_cast<long double>(hq) * hq;
        const long double left = static_cast<long double>(reference_left[frame]);
        const long double right = static_cast<long double>(reference_right[frame]);
        left_energy += left * left;
        right_energy += right * right;
    }

    if (active == 0 || hq_energy <= 0.0L)
        return result;

    const long double count = static_cast<long double>(frames);
    const double hq_rms = std::sqrt(static_cast<double>(hq_energy / count));
    const double left_rms = std::sqrt(static_cast<double>(left_energy / count));
    const double right_rms = std::sqrt(static_cast<double>(right_energy / count));
    if (!(hq_rms > 0.0) || !std::isfinite(hq_rms)
        || !std::isfinite(left_rms) || !std::isfinite(right_rms))
        return result;

    result.left_gain = left_rms / hq_rms;
    result.right_gain = right_rms / hq_rms;
    result.hq_rms = hq_rms;
    result.reference_left_rms = left_rms;
    result.reference_right_rms = right_rms;
    result.active_frames = active;
    result.valid = std::isfinite(result.left_gain)
        && std::isfinite(result.right_gain)
        && result.left_gain >= 0.0
        && result.right_gain >= 0.0;
    return result;
}

template <std::size_t MaxFrames = 8192>
class ym2612_hq_calibrated_source_storage {
public:
    bool build(
        const float* hq_mono,
        const std::int32_t* reference_left,
        const std::int32_t* reference_right,
        std::size_t frames) noexcept
    {
        valid_ = false;
        frames_ = 0;
        if (frames == 0 || frames > MaxFrames)
            return false;
        calibration_ = calibrate_ym2612_hq_source(
            hq_mono, reference_left, reference_right, frames);
        if (!calibration_.valid)
            return false;

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const double mono = static_cast<double>(hq_mono[frame]);
            const double left = mono * calibration_.left_gain;
            const double right = mono * calibration_.right_gain;
            if (!std::isfinite(left) || !std::isfinite(right))
                return false;
            left_[frame] = static_cast<float>(left);
            right_[frame] = static_cast<float>(right);
        }
        frames_ = frames;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t frames() const noexcept { return frames_; }
    const float* left() const noexcept { return left_.data(); }
    const float* right() const noexcept { return right_.data(); }
    const ym2612_hq_source_calibration& calibration() const noexcept {
        return calibration_;
    }

private:
    std::array<float, MaxFrames> left_{};
    std::array<float, MaxFrames> right_{};
    ym2612_hq_source_calibration calibration_{};
    std::size_t frames_ = 0;
    bool valid_ = false;
};

} // namespace gameaudio::vgm
