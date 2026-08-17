#include "components/spc/spc_studio_sample_reconstruction.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

namespace {

constexpr double pi = 3.141592653589793238462643383279502884;

double sinc(double x) {
    if (std::abs(x) < 1.0e-12)
        return 1.0;
    const double pix = pi * x;
    return std::sin(pix) / pix;
}

double old_lanczos4(const float* source, std::size_t count, double position) {
    const std::ptrdiff_t center = static_cast<std::ptrdiff_t>(std::floor(position));
    double weighted = 0.0;
    double weight_sum = 0.0;
    for (std::ptrdiff_t offset = -3; offset <= 4; ++offset) {
        const std::ptrdiff_t index = center + offset;
        if (index < 0 || static_cast<std::size_t>(index) >= count)
            continue;
        const double distance = static_cast<double>(index) - position;
        double weight = 0.0;
        if (std::abs(distance) < 4.0)
            weight = sinc(distance) * sinc(distance / 4.0);
        weighted += static_cast<double>(source[static_cast<std::size_t>(index)]) * weight;
        weight_sum += weight;
    }
    return weighted / weight_sum;
}

} // namespace

int main() {
    using namespace gameaudio::spc;

    // Explicit setup hook makes the intended realtime contract visible: table
    // construction belongs to player/track preparation, not the audio callback.
    prepare_spc_studio_sample_reconstruction();

    std::array<float, 256> constant{};
    constant.fill(0.375f);
    for (double position : {0.0, 0.125, 7.5, 127.375, 254.875, 255.0}) {
        const auto reconstructed = reconstruct_spc_studio_sample(
            constant.data(), constant.size(), position);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample - 0.375) < 1.0e-6);
    }

    // Integer source coordinates remain source coordinates, not a softened
    // approximation. Test away from boundaries where the full kernel exists.
    std::array<float, 256> arbitrary{};
    for (std::size_t i = 0; i < arbitrary.size(); ++i)
        arbitrary[i] = static_cast<float>(0.7 * std::sin(static_cast<double>(i) * 0.173));
    for (std::size_t i : {40u, 96u, 160u, 220u}) {
        const auto reconstructed = reconstruct_spc_studio_sample(
            arbitrary.data(), arbitrary.size(), static_cast<double>(i));
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample - arbitrary[i]) < 2.0e-6);
    }

    // The old top rung used only eight Lanczos taps. A high but still
    // bandlimited source exposes the difference clearly: the 64-tap studio
    // kernel should track the known continuous sinusoid rather than merely be
    // "different" from the old implementation.
    constexpr double frequency_cycles_per_sample = 0.4;
    std::array<float, 256> high_frequency{};
    for (std::size_t i = 0; i < high_frequency.size(); ++i) {
        high_frequency[i] = static_cast<float>(
            std::sin(2.0 * pi * frequency_cycles_per_sample * static_cast<double>(i)));
    }

    double studio_error_sum = 0.0;
    double old_error_sum = 0.0;
    for (double position : {64.125, 64.25, 64.5, 64.75, 64.875}) {
        const double expected = std::sin(
            2.0 * pi * frequency_cycles_per_sample * position);
        const auto studio = reconstruct_spc_studio_sample(
            high_frequency.data(), high_frequency.size(), position);
        assert(studio.valid);
        const double studio_error = std::abs(studio.sample - expected);
        const double old_error = std::abs(
            old_lanczos4(high_frequency.data(), high_frequency.size(), position)
            - expected);
        assert(studio_error < 5.0e-5);
        studio_error_sum += studio_error;
        old_error_sum += old_error;
    }
    assert(studio_error_sum < old_error_sum * 0.01);

    // Invalid source evidence must fail closed rather than leak NaN into the
    // mixer or silently interpolate across an unknown sample.
    auto invalid = arbitrary;
    invalid[64] = std::numeric_limits<float>::quiet_NaN();
    assert(!reconstruct_spc_studio_sample(
        invalid.data(), invalid.size(), 64.25).valid);
    assert(!reconstruct_spc_studio_sample(nullptr, arbitrary.size(), 32.0).valid);
    assert(!reconstruct_spc_studio_sample(
        arbitrary.data(), arbitrary.size(), -0.25).valid);
    assert(!reconstruct_spc_studio_sample(
        arbitrary.data(), arbitrary.size(), 256.0).valid);

    return 0;
}
