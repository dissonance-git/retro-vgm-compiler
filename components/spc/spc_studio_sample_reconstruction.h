#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace gameaudio::spc {

// Modern source-domain interpolation for the highest-confidence SPC Enhanced
// rung. This is deliberately not an S-DSP reference model. It is the sampler we
// want when an actual upstream/source waveform has been proven and the goal is
// to realize that same instrument with a contemporary reconstruction ceiling.
//
// The table is generated once and then costs one 64-tap dot product per sample.
// 16,384 fractional phases keep phase quantization below the old 8-point
// interpolation error by a wide margin without putting trigonometry in the
// realtime hot loop. Call prepare_spc_studio_sample_reconstruction() during
// track/player setup when this path is going to be used so table construction
// never lands on the audio callback.
struct spc_studio_reconstruction_result {
    double sample = 0.0;
    bool valid = false;
};

constexpr std::size_t spc_studio_tap_count = 64;
constexpr std::size_t spc_studio_phase_count = 16384;
constexpr double spc_studio_kaiser_beta = 8.6;
constexpr double spc_studio_pi = 3.141592653589793238462643383279502884;

inline double spc_studio_sinc(double x) noexcept {
    if (std::abs(x) < 1.0e-12)
        return 1.0;
    const double pix = spc_studio_pi * x;
    return std::sin(pix) / pix;
}

// Portable I0 approximation used only while the coefficient table is built.
// This is the classic piecewise polynomial/asymptotic form used for Kaiser
// windows; the audio hot loop only reads the resulting float coefficients.
inline double spc_studio_bessel_i0(double x) noexcept {
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

class spc_studio_sinc_table {
public:
    using phase_coefficients = std::array<float, spc_studio_tap_count>;

    spc_studio_sinc_table() noexcept {
        const double denominator = spc_studio_bessel_i0(spc_studio_kaiser_beta);
        constexpr double radius = static_cast<double>(spc_studio_tap_count) / 2.0;
        constexpr std::ptrdiff_t first_offset =
            -static_cast<std::ptrdiff_t>(spc_studio_tap_count / 2 - 1);

        for (std::size_t phase = 0; phase < spc_studio_phase_count; ++phase) {
            const double fraction = static_cast<double>(phase)
                / static_cast<double>(spc_studio_phase_count);
            double coefficient_sum = 0.0;

            for (std::size_t tap = 0; tap < spc_studio_tap_count; ++tap) {
                const std::ptrdiff_t offset = first_offset
                    + static_cast<std::ptrdiff_t>(tap);
                const double distance = static_cast<double>(offset) - fraction;
                const double normalized = std::abs(distance) / radius;

                double coefficient = 0.0;
                if (normalized < 1.0) {
                    const double window = spc_studio_bessel_i0(
                        spc_studio_kaiser_beta
                        * std::sqrt(1.0 - normalized * normalized))
                        / denominator;
                    coefficient = spc_studio_sinc(distance) * window;
                }

                m_coefficients[phase][tap] = static_cast<float>(coefficient);
                coefficient_sum += coefficient;
            }

            // Preserve DC exactly for a full neighborhood. Boundary-truncated
            // neighborhoods are normalized again at evaluation time.
            if (std::isfinite(coefficient_sum)
                && std::abs(coefficient_sum) > 1.0e-12) {
                const double inverse = 1.0 / coefficient_sum;
                for (float& coefficient : m_coefficients[phase])
                    coefficient = static_cast<float>(
                        static_cast<double>(coefficient) * inverse);
            }
        }
    }

    const phase_coefficients& phase(std::size_t index) const noexcept {
        return m_coefficients[index];
    }

private:
    std::array<phase_coefficients, spc_studio_phase_count> m_coefficients{};
};

inline const spc_studio_sinc_table& spc_studio_table() noexcept {
    static const spc_studio_sinc_table table;
    return table;
}

inline void prepare_spc_studio_sample_reconstruction() noexcept {
    (void)spc_studio_table();
}

// Random-access source sampler for a proven upstream waveform. It intentionally
// changes only reconstruction quality. Source identity, loop traversal, pitch,
// modulation, envelope, routing and timing remain owned by the game/runtime.
inline spc_studio_reconstruction_result reconstruct_spc_studio_sample(
    const float* source,
    std::size_t frame_count,
    double position) noexcept
{
    spc_studio_reconstruction_result result;
    if (source == nullptr || frame_count == 0 || !std::isfinite(position)
        || position < 0.0
        || position > static_cast<double>(frame_count - 1u))
        return result;

    double base = std::floor(position);
    std::ptrdiff_t center = static_cast<std::ptrdiff_t>(base);
    const double fraction = position - base;
    std::size_t phase = static_cast<std::size_t>(
        std::floor(fraction * static_cast<double>(spc_studio_phase_count) + 0.5));

    // Nearest-phase quantization can round a value just below the next integer
    // onto phase zero. Advance the integer coordinate with it so the source
    // position remains continuous.
    if (phase >= spc_studio_phase_count) {
        phase = 0;
        ++center;
    }

    const auto& coefficients = spc_studio_table().phase(phase);
    constexpr std::ptrdiff_t first_offset =
        -static_cast<std::ptrdiff_t>(spc_studio_tap_count / 2 - 1);

    double weighted = 0.0;
    double weight_sum = 0.0;
    for (std::size_t tap = 0; tap < spc_studio_tap_count; ++tap) {
        const std::ptrdiff_t offset = first_offset
            + static_cast<std::ptrdiff_t>(tap);
        const std::ptrdiff_t index = center + offset;
        if (index < 0 || static_cast<std::size_t>(index) >= frame_count)
            continue;

        const float sample = source[static_cast<std::size_t>(index)];
        if (!std::isfinite(sample))
            return result;

        const double coefficient = static_cast<double>(coefficients[tap]);
        weighted += static_cast<double>(sample) * coefficient;
        weight_sum += coefficient;
    }

    if (!std::isfinite(weighted) || !std::isfinite(weight_sum)
        || std::abs(weight_sum) < 1.0e-12)
        return result;

    result.sample = weighted / weight_sum;
    result.valid = std::isfinite(result.sample);
    return result;
}

} // namespace gameaudio::spc
