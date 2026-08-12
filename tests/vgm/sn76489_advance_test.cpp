#include "../../components/vgm/enhancement/sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::sn76489_enhanced;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {
constexpr std::size_t skipped_frames = 48000 * 3 + 137;
constexpr std::size_t compare_frames = 2048;
using buffer = std::array<float, compare_frames>;

void seed(sn76489_enhanced& psg) {
    // Tone 1 period 0x123, full volume.
    psg.write(0x83);
    psg.write(0x12);
    psg.write(0x90);

    // Tone 3 controls white-noise rate, which forces advance() to preserve both
    // oscillator phase and a nontrivial LFSR trajectory.
    psg.write(0xC7);
    psg.write(0x1A);
    psg.write(0xD2);
    psg.write(0xE7);
    psg.write(0xF0);
}
}

int main() {
    sn76489_enhanced::config cfg;
    cfg.sample_rate_hz = 48000.0;
    cfg.oversample = 4;

    sn76489_enhanced rendered(cfg);
    sn76489_enhanced advanced(cfg);
    seed(rendered);
    seed(advanced);

    float* discard[sn76489_enhanced::stem_count] = {nullptr, nullptr, nullptr, nullptr};
    rendered.render(discard, skipped_frames);
    advanced.advance(skipped_frames);

    CHECK(rendered.noise_lfsr() == advanced.noise_lfsr());
    CHECK(rendered.tone_period(0) == advanced.tone_period(0));
    CHECK(rendered.noise_control() == advanced.noise_control());

    std::array<buffer, sn76489_enhanced::stem_count> a{};
    std::array<buffer, sn76489_enhanced::stem_count> b{};
    float* out_a[sn76489_enhanced::stem_count] = {
        a[0].data(), a[1].data(), a[2].data(), a[3].data()
    };
    float* out_b[sn76489_enhanced::stem_count] = {
        b[0].data(), b[1].data(), b[2].data(), b[3].data()
    };
    rendered.render(out_a, compare_frames);
    advanced.render(out_b, compare_frames);

    for (std::size_t stem = 0; stem < sn76489_enhanced::stem_count; ++stem) {
        for (std::size_t frame = 0; frame < compare_frames; ++frame) {
            CHECK(std::abs(a[stem][frame] - b[stem][frame]) < 1e-5f);
        }
    }

    return 0;
}
