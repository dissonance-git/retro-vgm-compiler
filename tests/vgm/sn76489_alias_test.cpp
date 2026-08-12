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

double total_ac_energy(const buffer& data) {
    double mean = 0.0;
    for (float sample : data)
        mean += sample;
    mean /= static_cast<double>(data.size());

    double energy = 0.0;
    for (float sample : data) {
        const double centered = static_cast<double>(sample) - mean;
        energy += centered * centered;
    }
    return energy;
}

double sinusoid_energy(const buffer& data, std::size_t bin) {
    std::complex<double> projection{0.0, 0.0};
    for (std::size_t n = 0; n < data.size(); ++n) {
        const double phase = -2.0 * pi * static_cast<double>(bin) *
            static_cast<double>(n) / static_cast<double>(data.size());
        projection += static_cast<double>(data[n]) *
            std::complex<double>(std::cos(phase), std::sin(phase));
    }
    return 2.0 * std::norm(projection) / static_cast<double>(data.size());
}

double off_harmonic_energy(const buffer& data) {
    // 3000 Hz is exactly FFT bin 256 at 48 kHz / 4096. A perfect band-limited
    // 50% square at this fundamental can contain only the odd harmonics below
    // Nyquist: 3, 9, 15 and 21 kHz. Everything else is alias/noise energy.
    const std::array<std::size_t, 4> desired_bins{{256, 768, 1280, 1792}};
    double desired = 0.0;
    for (std::size_t bin : desired_bins)
        desired += sinusoid_energy(data, bin);
    return std::max(0.0, total_ac_energy(data) - desired);
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
    const double enhanced_alias = off_harmonic_energy(enhanced);
    const double naive_alias = off_harmonic_energy(naive);

    CHECK(naive_alias > 0.0);
    CHECK(enhanced_alias < naive_alias * 0.75);

    // Improvement must not come from deleting the musical fundamental.
    const double enhanced_fundamental = sinusoid_energy(enhanced, 256);
    const double naive_fundamental = sinusoid_energy(naive, 256);
    CHECK(enhanced_fundamental > naive_fundamental * 0.70);

    return 0;
}
