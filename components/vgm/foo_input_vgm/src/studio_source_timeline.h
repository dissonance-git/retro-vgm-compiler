#pragma once

#include "studio_source_resampler.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

// Scheduling companion for studio_source_resampler_kernel.
//
// The FIR itself answers "what does this source sound like at this destination
// instant?". This object answers the separate realtime question "do I possess
// enough exact native source history and lookahead to render that instant yet?"
// Keeping those questions separate lets PlayerA delay the whole Enhanced
// candidate while filtering only the source family that needs reconstruction.
// Unfiltered DAC/PSG/reference material therefore keeps its authored phase
// relationship to FM instead of being independently phase-warped.
struct studio_source_window {
    std::ptrdiff_t center = 0;
    std::ptrdiff_t first = 0;
    std::ptrdiff_t final = 0;
    std::size_t phase = 0;
    bool valid = false;
};

inline studio_source_window plan_studio_source_window(double source_position) noexcept {
    studio_source_window result;
    if (!std::isfinite(source_position)
        || source_position < static_cast<double>(std::numeric_limits<std::ptrdiff_t>::min())
        || source_position > static_cast<double>(std::numeric_limits<std::ptrdiff_t>::max()))
        return result;

    const double base = std::floor(source_position);
    std::ptrdiff_t center = static_cast<std::ptrdiff_t>(base);
    const double fraction = source_position - base;
    std::size_t phase = static_cast<std::size_t>(std::floor(
        fraction * static_cast<double>(studio_source_resampler_kernel::phase_count) + 0.5));
    if (phase >= studio_source_resampler_kernel::phase_count) {
        phase = 0;
        if (center == std::numeric_limits<std::ptrdiff_t>::max())
            return result;
        ++center;
    }

    constexpr auto pre = static_cast<std::ptrdiff_t>(studio_source_resampler_kernel::pre_roll);
    constexpr auto post = static_cast<std::ptrdiff_t>(studio_source_resampler_kernel::post_roll);
    if (center < std::numeric_limits<std::ptrdiff_t>::min() + pre
        || center > std::numeric_limits<std::ptrdiff_t>::max() - post)
        return result;

    result.center = center;
    result.first = center - pre;
    result.final = center + post;
    result.phase = phase;
    result.valid = true;
    return result;
}

class studio_source_timeline {
public:
    bool configure(double source_rate_hz, double destination_rate_hz) noexcept {
        if (!std::isfinite(source_rate_hz) || !std::isfinite(destination_rate_hz)
            || source_rate_hz <= 0.0 || destination_rate_hz <= 0.0) {
            source_rate_hz_ = 0.0;
            destination_rate_hz_ = 0.0;
            source_per_destination_ = 0.0;
            return false;
        }
        source_rate_hz_ = source_rate_hz;
        destination_rate_hz_ = destination_rate_hz;
        source_per_destination_ = source_rate_hz / destination_rate_hz;
        return std::isfinite(source_per_destination_) && source_per_destination_ > 0.0;
    }

    [[nodiscard]] bool configured() const noexcept {
        return source_per_destination_ > 0.0;
    }

    [[nodiscard]] double source_position(std::uint64_t destination_frame) const noexcept {
        if (!configured())
            return std::numeric_limits<double>::quiet_NaN();
        return static_cast<double>(destination_frame) * source_per_destination_;
    }

    [[nodiscard]] studio_source_window window(std::uint64_t destination_frame) const noexcept {
        return plan_studio_source_window(source_position(destination_frame));
    }

    // A negative first index means the FIR would require source history before
    // the preserved stream begins. We do not invent it. The source family stays
    // on protected reference playback during that warm-up region.
    [[nodiscard]] bool has_history(std::uint64_t destination_frame) const noexcept {
        const auto planned = window(destination_frame);
        return planned.valid && planned.first >= 0;
    }

    // captured_source_frames is a count, so the final required source index is
    // available exactly when final < captured_source_frames.
    [[nodiscard]] bool ready(
        std::uint64_t destination_frame,
        std::uint64_t captured_source_frames) const noexcept {
        const auto planned = window(destination_frame);
        if (!planned.valid || planned.first < 0 || planned.final < 0)
            return false;
        return static_cast<std::uint64_t>(planned.final) < captured_source_frames;
    }

    [[nodiscard]] std::uint64_t source_frames_required(
        std::uint64_t destination_frame) const noexcept {
        const auto planned = window(destination_frame);
        if (!planned.valid || planned.first < 0 || planned.final < 0)
            return 0;
        const auto final = static_cast<std::uint64_t>(planned.final);
        if (final == std::numeric_limits<std::uint64_t>::max())
            return 0;
        return final + 1;
    }

    [[nodiscard]] double source_rate_hz() const noexcept { return source_rate_hz_; }
    [[nodiscard]] double destination_rate_hz() const noexcept { return destination_rate_hz_; }

private:
    double source_rate_hz_ = 0.0;
    double destination_rate_hz_ = 0.0;
    double source_per_destination_ = 0.0;
};

} // namespace foobar_vgm::source_audio
