#include "../../components/vgm/enhancement/sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::sn76489_enhanced;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {
constexpr std::size_t frames = 4096;
using buffer = std::array<float, frames>;

void set_period(sn76489_enhanced& psg, std::size_t channel, std::uint16_t period) {
    psg.write(static_cast<std::uint8_t>(0x80u | ((channel & 3u) << 5) | (period & 0x0Fu)));
    psg.write(static_cast<std::uint8_t>((period >> 4) & 0x3Fu));
}

void set_volume(sn76489_enhanced& psg, std::size_t channel, std::uint8_t attenuation) {
    psg.write(static_cast<std::uint8_t>(0x90u | ((channel & 3u) << 5) | (attenuation & 0x0Fu)));
}

std::size_t negative_crossings(const buffer& data) {
    std::size_t count = 0;
    for (std::size_t i = 1; i < data.size(); ++i)
        if (data[i - 1] >= 0.0f && data[i] < 0.0f)
            ++count;
    return count;
}
}

int main() {
    // SN76489A/SN76496-style 17-bit LFSR must not be truncated to Sega's 16-bit state.
    sn76489_enhanced::config wide;
    wide.shift_register_width = 17;
    wide.white_noise_feedback = 0x000C;
    wide.sega_style_psg = false;
    sn76489_enhanced wide_psg(wide);
    CHECK(wide_psg.supported());
    CHECK(wide_psg.noise_lfsr() == 0x10000u);

    // Non-Sega variants treat an encoded tone period of zero as 0x400 rather
    // than forcing it to 1.
    set_period(wide_psg, 0, 0);
    CHECK(wide_psg.tone_period(0) == 0x400u);

    // Divider-1 parts run eight times faster than the common divider-8 Sega
    // configuration for the same clock and encoded period.
    sn76489_enhanced::config fast;
    fast.clock_divider = 1;
    fast.sample_rate_hz = 48000.0;
    fast.oversample = 4;
    sn76489_enhanced fast_psg(fast);
    set_period(fast_psg, 0, 0x100);
    set_volume(fast_psg, 0, 0);
    buffer output{};
    float* stems[sn76489_enhanced::stem_count] = {output.data(), nullptr, nullptr, nullptr};
    fast_psg.render(stems, frames);
    const double seconds = static_cast<double>(frames) / fast.sample_rate_hz;
    const double expected = (fast.chip_clock_hz / (4.0 * 256.0)) * seconds;
    CHECK(std::abs(static_cast<double>(negative_crossings(output)) - expected) < 2.0);

    // NCR-style PSG behavior is deliberately not approximated. It must stay
    // on the reference libvgm path until a dedicated enhanced implementation exists.
    sn76489_enhanced::config ncr;
    ncr.ncr_style_psg = true;
    sn76489_enhanced ncr_psg(ncr);
    CHECK(!ncr_psg.supported());
    set_period(ncr_psg, 0, 0x100);
    set_volume(ncr_psg, 0, 0);
    output.fill(1.0f);
    ncr_psg.render(stems, frames);
    for (float sample : output)
        CHECK(sample == 0.0f);

    return 0;
}
