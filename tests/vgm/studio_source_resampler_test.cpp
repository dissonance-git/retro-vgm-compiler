#include "components/vgm/foo_input_vgm/src/studio_source_resampler.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

constexpr double pi = 3.141592653589793238462643383279502884;

} // namespace

int main() {
    using namespace foobar_vgm::source_audio;

    studio_source_resampler_kernel kernel;
    assert(!kernel.configure(0.0, 48000.0));
    assert(kernel.configure(53267.0, 44100.0));
    assert(kernel.configured());
    assert(kernel.cutoff() > 0.0 && kernel.cutoff() < 1.0);

    constexpr std::size_t frames = 1024;
    std::array<studio_stereo_sample, frames> constant{};
    for (auto& sample : constant)
        sample = {0.25, -0.5};

    for (double position : {128.125, 256.5, 512.875}) {
        const auto reconstructed = kernel.reconstruct(
            constant.data(), constant.size(), position);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample.left - 0.25) < 1.0e-6);
        assert(std::abs(reconstructed.sample.right + 0.5) < 1.0e-6);
    }

    // A comfortably in-band sinusoid should retain its continuous-time value
    // at fractional source positions to much tighter error than linear SRC.
    std::array<studio_stereo_sample, frames> passband{};
    constexpr double passband_frequency = 0.20;
    for (std::size_t i = 0; i < passband.size(); ++i) {
        const double value = std::sin(
            2.0 * pi * passband_frequency * static_cast<double>(i));
        passband[i] = {value, -value};
    }
    for (double position : {400.125, 400.25, 400.5, 400.75, 400.875}) {
        const double expected = std::sin(
            2.0 * pi * passband_frequency * position);
        const auto reconstructed = kernel.reconstruct(
            passband.data(), passband.size(), position);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample.left - expected) < 3.0e-5);
        assert(std::abs(reconstructed.sample.right + expected) < 3.0e-5);
    }

    // 53.267 kHz -> 44.1 kHz moves the destination Nyquist below 0.45 cycles
    // per source sample. The Enhanced kernel must therefore reject that energy
    // instead of folding it into the audible band. Measure RMS over a run of
    // destination-spaced samples; a linear interpolator leaves a very large
    // alias here, while this bounded FIR should be far below it.
    std::array<studio_stereo_sample, frames> stopband{};
    constexpr double stopband_frequency = 0.45;
    for (std::size_t i = 0; i < stopband.size(); ++i) {
        const double value = std::sin(
            2.0 * pi * stopband_frequency * static_cast<double>(i));
        stopband[i] = {value, value};
    }

    const double source_step = 53267.0 / 44100.0;
    double energy = 0.0;
    std::size_t count = 0;
    for (double position = 300.0; position < 700.0; position += source_step) {
        const auto reconstructed = kernel.reconstruct(
            stopband.data(), stopband.size(), position);
        assert(reconstructed.valid);
        energy += reconstructed.sample.left * reconstructed.sample.left;
        ++count;
    }
    assert(count != 0);
    const double rms = std::sqrt(energy / static_cast<double>(count));
    assert(rms < 1.0e-3);

    // Symmetric bandlimited reconstruction has a real lookahead obligation.
    // Fail closed at unavailable edges rather than invent history or advance the
    // authoritative chip just to satisfy the filter.
    assert(!kernel.reconstruct(constant.data(), constant.size(), 12.0).valid);
    assert(!kernel.reconstruct(constant.data(), constant.size(), 1000.0).valid);

    return 0;
}
