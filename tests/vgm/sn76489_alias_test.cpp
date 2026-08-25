#include "../../components/vgm/enhancement/sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>

using gameaudio::vgm::sn76489_enhanced;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {
constexpr std::size_t frames = 4096;
constexpr double sample_rate = 48000.0;
constexpr double fundamental_hz = 3000.0;
constexpr std::size_t period = 16;
constexpr double pi = 3.141592653589793238462643383279502884;
using buffer = std::array<float, frames>;

void set_period(sn76489_enhanced& psg, std::uint16_t value) {
    psg.write(static_cast<std::uint8_t>(0x80u | (value & 0x0Fu)));
    psg.write(static_cast<std::uint8_t>((value >> 4) & 0x3Fu));
}

void set_full_volume(sn76489_enhanced& psg) {
    psg.write(0x90);
}

double sinusoid_amplitude(const buffer& data, std::size_t bin) {
    std::complex<double> projection{0.0, 0.0};
    for (std::size_t n = 0; n < data.size(); ++n) {
        const double phase = -2.0 * pi * static_cast<double>(bin) *
            static_cast<double>(n) / static_cast<double>(data.size());
        projection += static_cast<double>(data[n]) *
            std::complex<double>(std::cos(phase), std::sin(phase));
    }
    return 2.0 * std::abs(projection) / static_cast<double>(data.size());
}

double bandlimited_square_error(const buffer& data) {
    constexpr std::array<std::size_t, 4> harmonics{{1, 3, 5, 7}};
    double error = 0.0;
    for (std::size_t harmonic : harmonics) {
        const std::size_t bin = 256u * harmonic;
        const double ideal = 4.0 / (pi * static_cast<double>(harmonic));
        error += std::abs(sinusoid_amplitude(data, bin) - ideal) / ideal;
    }
    return error;
}

buffer naive_square() {
    buffer out{};
    for (std::size_t n = 0; n < out.size(); ++n)
        out[n] = (n % period) < (period / 2) ? 1.0f : -1.0f;
    return out;
}
}

int main() {
    sn76489_enhanced::config cfg;
    cfg.sample_rate_hz = sample_rate;
    cfg.oversample = 4;

    // Choose a synthetic chip clock so encoded PSG period 16 lands exactly at
    // 3 kHz. The test is about reconstruction quality, not a console region.
    cfg.chip_clock_hz = fundamental_hz * 32.0 * static_cast<double>(period);

    sn76489_enhanced psg(cfg);
    set_period(psg, period);
    set_full_volume(psg);

    buffer enhanced{};
    float* outputs[sn76489_enhanced::stem_count] = {enhanced.data(), nullptr, nullptr, nullptr};
    psg.render(outputs, enhanced.size());

    const buffer naive = naive_square();
    const double enhanced_error = bandlimited_square_error(enhanced);
    const double naive_error = bandlimited_square_error(naive);

    CHECK(naive_error > 0.0);
    CHECK(enhanced_error < naive_error * 0.25);

    // Improvement must not come from deleting the musical fundamental.
    const double enhanced_fundamental = sinusoid_amplitude(enhanced, 256);
    const double ideal_fundamental = 4.0 / pi;
    CHECK(enhanced_fundamental > ideal_fundamental * 0.95);

    return 0;
}
