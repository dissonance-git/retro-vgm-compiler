#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace foobar_vgm::source_audio {

// Enhanced-only sample-rate conversion primitive for source lanes. The
// protected libvgm mix keeps its historical resampler. This kernel is for a
// later aligned replacement path where the whole Enhanced candidate can carry
// the same explicit FIR latency.
//
// Why this exists: libvgm RSMODE_LINEAR is intentionally small and deterministic
// but is still linear interpolation/box-like downsampling. Once an FM source has
// been lifted into a higher-precision synthesis domain, leaving that source
// behind a linear SRC becomes the next avoidable quality ceiling.
struct studio_stereo_sample {
    double left = 0.0;
    double right = 0.0;
};

struct studio_resample_result {
    studio_stereo_sample sample{};
    bool valid = false;
};

class studio_source_resampler_kernel {
public:
    static constexpr std::size_t tap_count = 64;
    static constexpr std::size_t phase_count = 4096;
    static constexpr std::size_t pre_roll = 31;
    static constexpr std::size_t post_roll = 32;
    static constexpr double kaiser_beta = 8.6;
    static constexpr double pi = 3.141592653589793238462643383279502884;

    bool configure(double source_rate_hz, double destination_rate_hz) {
        if (!std::isfinite(source_rate_hz) || !std::isfinite(destination_rate_hz)
            || source_rate_hz <= 0.0 || destination_rate_hz <= 0.0) {
            coefficients_.clear();
            source_rate_hz_ = destination_rate_hz_ = cutoff_ = 0.0;
            return false;
        }

        source_rate_hz_ = source_rate_hz;
        destination_rate_hz_ = destination_rate_hz;

        // sinc() below is normalized so 1.0 is the input Nyquist limit. Leave
        // a small transition guard on downsampling so a finite 64-tap kernel
        // attenuates before the new Nyquist boundary rather than at it.
        const double rate_ratio = destination_rate_hz / source_rate_hz;
        cutoff_ = rate_ratio < 1.0
            ? std::max(0.01, rate_ratio * 0.96)
            : 1.0;

        try {
            coefficients_.assign(phase_count * tap_count, 0.0f);
        } catch (...) {
            coefficients_.clear();
            return false;
        }

        const double denominator = bessel_i0(kaiser_beta);
        constexpr double radius = static_cast<double>(tap_count) / 2.0;
        constexpr std::ptrdiff_t first_offset = -static_cast<std::ptrdiff_t>(pre_roll);

        for (std::size_t phase = 0; phase < phase_count; ++phase) {
            const double fraction = static_cast<double>(phase)
                / static_cast<double>(phase_count);
            double sum = 0.0;
            for (std::size_t tap = 0; tap < tap_count; ++tap) {
                const std::ptrdiff_t offset = first_offset
                    + static_cast<std::ptrdiff_t>(tap);
                const double distance = static_cast<double>(offset) - fraction;
                const double normalized = std::abs(distance) / radius;

                double coefficient = 0.0;
                if (normalized < 1.0) {
                    const double window = bessel_i0(
                        kaiser_beta * std::sqrt(1.0 - normalized * normalized))
                        / denominator;
                    coefficient = cutoff_ * sinc(cutoff_ * distance) * window;
                }
                coefficients_[phase * tap_count + tap]
                    = static_cast<float>(coefficient);
                sum += coefficient;
            }

            if (!std::isfinite(sum) || std::abs(sum) < 1.0e-12) {
                coefficients_.clear();
                return false;
            }
            const double inverse = 1.0 / sum;
            for (std::size_t tap = 0; tap < tap_count; ++tap) {
                auto& coefficient = coefficients_[phase * tap_count + tap];
                coefficient = static_cast<float>(
                    static_cast<double>(coefficient) * inverse);
            }
        }
        return true;
    }

    [[nodiscard]] bool configured() const noexcept {
        return coefficients_.size() == phase_count * tap_count;
    }
    [[nodiscard]] double source_rate_hz() const noexcept { return source_rate_hz_; }
    [[nodiscard]] double destination_rate_hz() const noexcept { return destination_rate_hz_; }
    [[nodiscard]] double cutoff() const noexcept { return cutoff_; }

    // Evaluate one destination instant at an absolute source-frame coordinate.
    // A symmetric FIR needs lookahead, so callers must preserve pre_roll and
    // post_roll source frames and align the entire Enhanced candidate by the
    // same latency. Returning invalid at an unavailable boundary is deliberate:
    // this primitive must not synthesize edge history or silently desynchronize
    // FM from DAC/PSG/reference families.
    studio_resample_result reconstruct(
        const studio_stereo_sample* source,
        std::size_t frame_count,
        double source_position) const noexcept {
        studio_resample_result result;
        if (!configured() || source == nullptr || frame_count == 0
            || !std::isfinite(source_position))
            return result;

        const double base = std::floor(source_position);
        std::ptrdiff_t center = static_cast<std::ptrdiff_t>(base);
        const double fraction = source_position - base;
        std::size_t phase = static_cast<std::size_t>(
            std::floor(fraction * static_cast<double>(phase_count) + 0.5));
        if (phase >= phase_count) {
            phase = 0;
            ++center;
        }

        if (center < static_cast<std::ptrdiff_t>(pre_roll))
            return result;
        const std::ptrdiff_t final_index = center
            + static_cast<std::ptrdiff_t>(post_roll);
        if (final_index < 0 || static_cast<std::size_t>(final_index) >= frame_count)
            return result;

        const float* coefficients = coefficients_.data() + phase * tap_count;
        const std::ptrdiff_t first = center - static_cast<std::ptrdiff_t>(pre_roll);
        double left = 0.0;
        double right = 0.0;
        for (std::size_t tap = 0; tap < tap_count; ++tap) {
            const auto& input = source[static_cast<std::size_t>(
                first + static_cast<std::ptrdiff_t>(tap))];
            if (!std::isfinite(input.left) || !std::isfinite(input.right))
                return result;
            const double coefficient = static_cast<double>(coefficients[tap]);
            left += input.left * coefficient;
            right += input.right * coefficient;
        }

        result.sample = {left, right};
        result.valid = std::isfinite(left) && std::isfinite(right);
        return result;
    }

private:
    static double sinc(double x) noexcept {
        if (std::abs(x) < 1.0e-12)
            return 1.0;
        const double pix = pi * x;
        return std::sin(pix) / pix;
    }

    static double bessel_i0(double x) noexcept {
        const double ax = std::abs(x);
        if (ax < 3.75) {
            double y = x / 3.75;
            y *= y;
            return 1.0 + y * (3.5156229
                + y * (3.0899424
                + y * (1.2067492
                + y * (0.2659732
                + y * (0.0360768
                + y * 0.0045813)))));
        }
        const double y = 3.75 / ax;
        return (std::exp(ax) / std::sqrt(ax)) * (0.39894228
            + y * (0.01328592
            + y * (0.00225319
            + y * (-0.00157565
            + y * (0.00916281
            + y * (-0.02057706
            + y * (0.02635537
            + y * (-0.01647633
            + y * 0.00392377))))))));
    }

    std::vector<float> coefficients_{};
    double source_rate_hz_ = 0.0;
    double destination_rate_hz_ = 0.0;
    double cutoff_ = 0.0;
};

} // namespace foobar_vgm::source_audio
