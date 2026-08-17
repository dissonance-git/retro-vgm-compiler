#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// One deliberately narrow enhancement boundary for sampled S-DSP voices.
//
// The reference renderer remains the hardware-compatible S-DSP Gaussian path.
// This structure describes the decoded BRR source neighborhood *before* that
// interpolation stage so an enhanced path can change reconstruction without
// changing sample identity, source traversal, pitch, envelope, routing, echo,
// or musical timing at the same time.
struct spc_decoded_source_window {
    static constexpr std::size_t tap_count = 8;
    static constexpr std::size_t integer_sample_index = 3;

    // Samples are ordered relative to the current integer source coordinate:
    //   [-3, -2, -1, 0, +1, +2, +3, +4]
    // The desired position lies between indices 3 and 4.
    std::array<std::int16_t, tap_count> samples{};

    // Fractional source position in the S-DSP sample interval. This is the
    // low 12-bit fraction of the source coordinate, not a host-output phase.
    // 0 means exactly samples[integer_sample_index].
    std::uint16_t fraction_q12 = 0;
};

struct spc_reconstruction_result {
    double sample = 0.0;
    bool valid = false;
};

constexpr std::uint16_t spc_fraction_q12_max = 0x0FFFu;
constexpr double spc_pi = 3.141592653589793238462643383279502884;

inline double spc_sinc(double x) noexcept {
    if (std::abs(x) < 1.0e-12)
        return 1.0;
    const double pix = spc_pi * x;
    return std::sin(pix) / pix;
}

inline double spc_lanczos4_weight(double distance) noexcept {
    constexpr double radius = 4.0;
    if (std::abs(distance) >= radius)
        return 0.0;
    return spc_sinc(distance) * spc_sinc(distance / radius);
}

// Constraint-relaxed source reconstruction. This is intentionally *not*
// called reference-equivalent. It replaces only the reconstruction filter,
// using an 8-tap Lanczos-windowed sinc over decoded BRR samples.
//
// Normalizing by the finite-window coefficient sum gives exact DC preservation
// for constant input and makes the operation well behaved at arbitrary phase.
// No heap allocation, resampling of the final stereo bus, or semantic inference
// occurs here.
inline spc_reconstruction_result reconstruct_spc_lanczos4(
    const spc_decoded_source_window& input) noexcept
{
    spc_reconstruction_result result;
    if (input.fraction_q12 > spc_fraction_q12_max)
        return result;

    const double fraction = static_cast<double>(input.fraction_q12) / 4096.0;
    double weighted = 0.0;
    double weight_sum = 0.0;

    for (std::size_t i = 0; i < input.tap_count; ++i) {
        const double relative = static_cast<double>(i) -
            static_cast<double>(input.integer_sample_index);
        const double weight = spc_lanczos4_weight(relative - fraction);
        weighted += static_cast<double>(input.samples[i]) * weight;
        weight_sum += weight;
    }

    if (!std::isfinite(weighted) || !std::isfinite(weight_sum) ||
        std::abs(weight_sum) < 1.0e-12)
        return result;

    result.sample = weighted / weight_sum;
    result.valid = std::isfinite(result.sample);
    return result;
}

// Portable spelling of the arithmetic right-shift used by historical S-DSP
// implementations. C++17 does not require signed right shift of a negative
// value to round toward negative infinity, so keep that semantic explicit.
inline std::int64_t spc_floor_divide_2048(std::int64_t value) noexcept {
    std::int64_t quotient = value / 2048;
    const std::int64_t remainder = value % 2048;
    if (value < 0 && remainder != 0)
        --quotient;
    return quotient;
}

// Keep envelope arithmetic as a separate intervention. This helper applies the
// historical S-DSP/libgme envelope quantization to a reconstructed source value
// so reconstruction-filter experiments do not silently also become arithmetic-
// precision experiments. A later enhanced mixer may choose a distinct explicit
// high-precision envelope/mix path.
inline std::int16_t apply_spc_reference_envelope_quantization(
    double reconstructed,
    std::uint16_t envelope) noexcept
{
    if (!std::isfinite(reconstructed))
        return 0;

    if (envelope > 0x07FFu)
        envelope = 0x07FFu;

    // The sampled-voice path performs arithmetic division by 2^11 and clears
    // the low bit. Round the replacement interpolator to the same integer source
    // domain first, then retain that arithmetic boundary without relying on a
    // compiler-specific signed-shift rule.
    long source = std::lround(reconstructed);
    if (source < -32768L)
        source = -32768L;
    else if (source > 32767L)
        source = 32767L;

    const std::int64_t product = static_cast<std::int64_t>(source) *
        static_cast<std::int64_t>(envelope);
    std::int64_t scaled = spc_floor_divide_2048(product);
    if ((scaled % 2) != 0)
        --scaled;

    if (scaled < -32768)
        scaled = -32768;
    else if (scaled > 32767)
        scaled = 32767;
    return static_cast<std::int16_t>(scaled);
}

} // namespace gameaudio::spc
