#pragma once

#include "studio_source_resampler.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

struct studio_source_window {
    std::ptrdiff_t center = 0;
    std::ptrdiff_t first = 0;
    std::ptrdiff_t final = 0;
    std::size_t phase = 0;
    bool valid = false;
};

// Exact source coordinate on the same 1/4096-sample phase grid used by the
// Studio FIR. Negative values are valid during startup planning and fail closed
// later when their required source history does not exist.
struct studio_source_phase_position {
    std::int64_t phase_units = 0;
    bool valid = false;

    [[nodiscard]] double as_double() const noexcept {
        return valid
            ? static_cast<double>(phase_units)
                / static_cast<double>(studio_source_resampler_kernel::phase_count)
            : std::numeric_limits<double>::quiet_NaN();
    }
};

// Project-owned copy of the timing fields that matter from libvgm's
// pre-Resmpl_Execute RESMPL_STATE. Keeping the adapter POD here lets the exact
// Studio scheduler be tested without linking libvgm into the core test suite.
struct studio_linear_timing_snapshot {
    std::uint32_t source_rate_hz = 0;
    std::uint32_t destination_rate_hz = 0;
    std::uint32_t sample_p = 0;
    std::uint32_t sample_last = 0;
    std::uint32_t sample_next = 0;

    [[nodiscard]] bool valid() const noexcept {
        return source_rate_hz != 0 && destination_rate_hz != 0;
    }
};

namespace detail {
constexpr std::uint64_t linear_fixed_factor = 1u << 11;
static_assert(studio_source_resampler_kernel::phase_count
    == linear_fixed_factor * 2u);

inline bool checked_mul(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a)
        return false;
    out = a * b;
    return true;
}

inline bool checked_add(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (b > std::numeric_limits<std::uint64_t>::max() - a)
        return false;
    out = a + b;
    return true;
}

inline studio_source_phase_position combine_native_base(
    std::uint64_t native_base,
    std::int64_t local_phase_units) noexcept {
    studio_source_phase_position result;
    constexpr auto phase_count = static_cast<std::uint64_t>(
        studio_source_resampler_kernel::phase_count);
    if (native_base > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) / phase_count)
        return result;
    const std::int64_t base = static_cast<std::int64_t>(native_base * phase_count);
    if (local_phase_units > 0
        && base > std::numeric_limits<std::int64_t>::max() - local_phase_units)
        return result;
    if (local_phase_units < 0
        && base < std::numeric_limits<std::int64_t>::min() - local_phase_units)
        return result;
    result.phase_units = base + local_phase_units;
    result.valid = true;
    return result;
}

inline std::int64_t floor_phase_center(std::int64_t units, std::size_t& phase) noexcept {
    constexpr std::int64_t denominator = static_cast<std::int64_t>(
        studio_source_resampler_kernel::phase_count);
    std::int64_t center = units / denominator;
    std::int64_t remainder = units % denominator;
    if (remainder < 0) {
        remainder += denominator;
        --center;
    }
    phase = static_cast<std::size_t>(remainder);
    return center;
}
} // namespace detail

inline studio_source_window plan_studio_source_window(
    studio_source_phase_position position) noexcept {
    studio_source_window result;
    if (!position.valid)
        return result;

    std::size_t phase = 0;
    const std::int64_t center64 = detail::floor_phase_center(position.phase_units, phase);
    if (center64 < static_cast<std::int64_t>(std::numeric_limits<std::ptrdiff_t>::min())
        || center64 > static_cast<std::int64_t>(std::numeric_limits<std::ptrdiff_t>::max()))
        return result;
    const std::ptrdiff_t center = static_cast<std::ptrdiff_t>(center64);
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

// Mirror the exact source-time coordinate represented by one destination frame
// from libvgm's pre-execution linear timing snapshot. native_base is the
// absolute ordinal of newly_captured[0] for this Resmpl_Execute segment.
//
// Upsampling uses the exact interpolation instant. Downsampling's historical
// linear path averages an input interval, so Studio uses that interval's exact
// midpoint. Because libvgm's fixed-point grid is 1/2048 sample and Studio has
// 4096 phases, both coordinates land exactly on the Studio phase grid.
inline studio_source_phase_position studio_linear_source_position(
    const studio_linear_timing_snapshot& before,
    std::uint64_t native_base,
    std::uint64_t destination_offset) noexcept {
    studio_source_phase_position invalid;
    if (!before.valid())
        return invalid;

    constexpr std::uint64_t fixed = detail::linear_fixed_factor;
    constexpr std::int64_t studio_phase = static_cast<std::int64_t>(
        studio_source_resampler_kernel::phase_count);

    if (before.source_rate_hz == before.destination_rate_hz) {
        std::uint64_t native = 0;
        if (!detail::checked_add(native_base, destination_offset, native)
            || native > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()) /
                static_cast<std::uint64_t>(studio_phase))
            return invalid;
        return {
            static_cast<std::int64_t>(
                native * static_cast<std::uint64_t>(studio_phase)),
            true
        };
    }

    std::uint64_t chip_rate_fp = 0;
    if (!detail::checked_mul(fixed, before.source_rate_hz, chip_rate_fp))
        return invalid;

    if (before.source_rate_hz < before.destination_rate_hz) {
        std::uint64_t sample_p = 0;
        if (!detail::checked_add(before.sample_p, destination_offset, sample_p))
            return invalid;
        std::uint64_t product = 0;
        if (!detail::checked_mul(sample_p, chip_rate_fp, product))
            return invalid;
        const std::uint64_t input_position_fp =
            product / before.destination_rate_hz;
        if (input_position_fp > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max() / 2))
            return invalid;

        const std::uint64_t history_samples =
            static_cast<std::uint64_t>(before.sample_next) + 1u;
        if (history_samples > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()) /
                static_cast<std::uint64_t>(studio_phase))
            return invalid;

        const std::int64_t local =
            static_cast<std::int64_t>(input_position_fp * 2u)
            - static_cast<std::int64_t>(
                history_samples * static_cast<std::uint64_t>(studio_phase));
        return detail::combine_native_base(native_base, local);
    }

    std::uint64_t begin_product = 0;
    if (!detail::checked_mul(before.sample_p, chip_rate_fp, begin_product))
        return invalid;
    const std::uint64_t begin_position_fp =
        begin_product / before.destination_rate_hz;
    const std::uint64_t last_fp =
        static_cast<std::uint64_t>(before.sample_last) * fixed;
    std::uint64_t begin_with_history = 0;
    if (!detail::checked_add(begin_position_fp, fixed, begin_with_history)
        || begin_with_history < last_fp)
        return invalid;
    const std::uint64_t input_base_fp = begin_with_history - last_fp;

    std::uint64_t offset_product = 0;
    if (!detail::checked_mul(destination_offset, chip_rate_fp, offset_product)
        || destination_offset == std::numeric_limits<std::uint64_t>::max())
        return invalid;
    std::uint64_t next_product = 0;
    if (!detail::checked_mul(destination_offset + 1u, chip_rate_fp, next_product))
        return invalid;

    std::uint64_t boundary0 = 0;
    std::uint64_t boundary1 = 0;
    if (!detail::checked_add(
            input_base_fp,
            offset_product / before.destination_rate_hz,
            boundary0)
        || !detail::checked_add(
            input_base_fp,
            next_product / before.destination_rate_hz,
            boundary1))
        return invalid;
    std::uint64_t midpoint_phase_units = 0;
    if (!detail::checked_add(boundary0, boundary1, midpoint_phase_units)
        || midpoint_phase_units > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
        return invalid;

    const std::int64_t local =
        static_cast<std::int64_t>(midpoint_phase_units) - studio_phase;
    return detail::combine_native_base(native_base, local);
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

    [[nodiscard]] bool has_history(std::uint64_t destination_frame) const noexcept {
        const auto planned = window(destination_frame);
        return planned.valid && planned.first >= 0;
    }

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
