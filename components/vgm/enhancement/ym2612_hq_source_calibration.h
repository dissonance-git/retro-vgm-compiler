#pragma once

#include "authored_stereo_route.h"

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
// source contribution already present in libvgm's protected mix. This legacy
// helper measures each output side independently. It is useful for bounded
// diagnostics, but realtime replacement should prefer the frozen unit-gain
// calibrator below so decode-block boundaries cannot behave like an automatic
// leveler.
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

struct ym2612_hq_unit_gain_calibration {
    double gain = 0.0;
    double hq_rms = 0.0;
    double reference_rms = 0.0;
    std::size_t active_frames = 0;
    bool valid = false;
};

namespace detail {

inline bool valid_authored_route(const authored_stereo_route& route) noexcept {
    return std::isfinite(route.left) && std::isfinite(route.right)
        && route.left >= 0.0f && route.right >= 0.0f;
}

struct ym2612_hq_unit_gain_observation {
    long double hq_energy = 0.0L;
    long double reference_energy = 0.0L;
    long double reference_weight = 0.0L;
    std::size_t active_frames = 0;
    bool valid = false;
};

inline ym2612_hq_unit_gain_observation observe_ym2612_hq_unit_gain(
    const float* hq_mono,
    const std::int32_t* reference_left,
    const std::int32_t* reference_right,
    std::size_t frames,
    authored_stereo_route route,
    double activity_floor) noexcept
{
    ym2612_hq_unit_gain_observation result;
    if (hq_mono == nullptr || reference_left == nullptr || reference_right == nullptr
        || frames == 0 || !valid_authored_route(route)
        || !std::isfinite(activity_floor) || activity_floor < 0.0)
        return result;

    const long double left_weight =
        static_cast<long double>(route.left) * static_cast<long double>(route.left);
    const long double right_weight =
        static_cast<long double>(route.right) * static_cast<long double>(route.right);
    const long double route_weight = left_weight + right_weight;

    // Both YM2612 pan bits clear is a valid authored state. There is no
    // audible reference evidence to calibrate from in that interval, so treat
    // it as a neutral observation rather than poisoning the source forever.
    if (!(route_weight > 0.0L)) {
        result.valid = true;
        return result;
    }

    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double hq = static_cast<double>(hq_mono[frame]);
        if (!std::isfinite(hq))
            return {};
        const long double sample = static_cast<long double>(hq);
        result.hq_energy += sample * sample;
        if (std::abs(hq) > activity_floor)
            ++result.active_frames;

        const long double left = static_cast<long double>(reference_left[frame]);
        const long double right = static_cast<long double>(reference_right[frame]);
        result.reference_energy += left * left + right * right;
        result.reference_weight += route_weight;
    }

    result.valid = true;
    return result;
}

} // namespace detail

// Fit one source-unit scalar while keeping authored stereo routing outside the
// calibration. When both YM2612 output sides carry the same physical channel,
// their energy is normalized by the route weight instead of being counted as
// two louder sources. This prevents the calibration itself from becoming a pan
// law or timbre shaper. The supplied route must be constant across the block; a
// caller observing a B4-B6 pan change must split the evidence at that boundary.
inline ym2612_hq_unit_gain_calibration calibrate_ym2612_hq_unit_gain(
    const float* hq_mono,
    const std::int32_t* reference_left,
    const std::int32_t* reference_right,
    std::size_t frames,
    authored_stereo_route route,
    double activity_floor = 1.0e-6) noexcept
{
    ym2612_hq_unit_gain_calibration result;
    const auto observation = detail::observe_ym2612_hq_unit_gain(
        hq_mono, reference_left, reference_right, frames, route, activity_floor);
    if (!observation.valid || observation.active_frames == 0
        || observation.hq_energy <= 0.0L || observation.reference_weight <= 0.0L)
        return result;

    const long double count = static_cast<long double>(frames);
    const double hq_rms = std::sqrt(static_cast<double>(observation.hq_energy / count));
    const double reference_rms = std::sqrt(static_cast<double>(
        observation.reference_energy / observation.reference_weight));
    if (!(hq_rms > 0.0) || !std::isfinite(hq_rms) || !std::isfinite(reference_rms))
        return result;

    result.gain = reference_rms / hq_rms;
    result.hq_rms = hq_rms;
    result.reference_rms = reference_rms;
    result.active_frames = observation.active_frames;
    result.valid = std::isfinite(result.gain) && result.gain >= 0.0;
    return result;
}

// Realtime admission helper. It accumulates exact-reference/HQ energy until
// enough active evidence exists, then freezes one unit conversion scalar for
// the source. Once ready, later musical dynamics, patch changes, and decode
// block boundaries cannot retune the gain. Reset only when the exact source/HQ
// gain domain itself is reset or reconstructed.
//
// Until ready(), the protected reference source should remain audible.
class ym2612_hq_frozen_unit_gain {
public:
    explicit ym2612_hq_frozen_unit_gain(
        std::size_t minimum_active_frames = 512,
        double activity_floor = 1.0e-6) noexcept
        : minimum_active_frames_(minimum_active_frames),
          activity_floor_(activity_floor) {}

    void reset() noexcept {
        hq_energy_ = 0.0L;
        reference_energy_ = 0.0L;
        reference_weight_ = 0.0L;
        observed_frames_ = 0;
        active_frames_ = 0;
        gain_ = 0.0;
        ready_ = false;
        failed_ = false;
    }

    bool observe(
        const float* hq_mono,
        const std::int32_t* reference_left,
        const std::int32_t* reference_right,
        std::size_t frames,
        authored_stereo_route route) noexcept
    {
        if (ready_)
            return true;
        if (failed_)
            return false;
        if (minimum_active_frames_ == 0 || !std::isfinite(activity_floor_)
            || activity_floor_ < 0.0) {
            failed_ = true;
            return false;
        }

        const auto observation = detail::observe_ym2612_hq_unit_gain(
            hq_mono, reference_left, reference_right, frames, route, activity_floor_);
        if (!observation.valid) {
            failed_ = true;
            return false;
        }

        // A both-sides-disabled route contributes no calibration evidence and
        // must not dilute the HQ energy denominator.
        if (observation.reference_weight > 0.0L) {
            hq_energy_ += observation.hq_energy;
            reference_energy_ += observation.reference_energy;
            reference_weight_ += observation.reference_weight;
            observed_frames_ += frames;
            active_frames_ += observation.active_frames;
        }

        if (active_frames_ < minimum_active_frames_)
            return true;
        if (!(hq_energy_ > 0.0L) || !(reference_weight_ > 0.0L)
            || observed_frames_ == 0) {
            failed_ = true;
            return false;
        }

        const long double effective_reference_energy =
            reference_energy_ / reference_weight_;
        const long double effective_hq_energy =
            hq_energy_ / static_cast<long double>(observed_frames_);
        if (!(effective_reference_energy >= 0.0L) || !(effective_hq_energy > 0.0L)) {
            failed_ = true;
            return false;
        }

        const double candidate = std::sqrt(static_cast<double>(
            effective_reference_energy / effective_hq_energy));
        if (!std::isfinite(candidate) || candidate < 0.0) {
            failed_ = true;
            return false;
        }

        gain_ = candidate;
        ready_ = true;
        return true;
    }

    bool ready() const noexcept { return ready_; }
    bool failed() const noexcept { return failed_; }
    double gain() const noexcept { return ready_ ? gain_ : 0.0; }
    std::size_t active_frames() const noexcept { return active_frames_; }

private:
    std::size_t minimum_active_frames_ = 512;
    double activity_floor_ = 1.0e-6;
    long double hq_energy_ = 0.0L;
    long double reference_energy_ = 0.0L;
    long double reference_weight_ = 0.0L;
    std::size_t observed_frames_ = 0;
    std::size_t active_frames_ = 0;
    double gain_ = 0.0;
    bool ready_ = false;
    bool failed_ = false;
};

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

template <std::size_t MaxFrames = 8192>
class ym2612_hq_frozen_source_storage {
public:
    bool build(
        const float* hq_mono,
        std::size_t frames,
        const ym2612_hq_frozen_unit_gain& calibration,
        authored_stereo_route route) noexcept
    {
        valid_ = false;
        frames_ = 0;
        if (hq_mono == nullptr || frames == 0 || frames > MaxFrames
            || !calibration.ready() || !detail::valid_authored_route(route))
            return false;

        const double gain = calibration.gain();
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const double mono = static_cast<double>(hq_mono[frame]);
            if (!std::isfinite(mono))
                return false;
            const double unit = mono * gain;
            const double left = unit * static_cast<double>(route.left);
            const double right = unit * static_cast<double>(route.right);
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

private:
    std::array<float, MaxFrames> left_{};
    std::array<float, MaxFrames> right_{};
    std::size_t frames_ = 0;
    bool valid_ = false;
};

} // namespace gameaudio::vgm
