#include "../../components/vgm/enhancement/ym2612_dac_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::ym2612_dac_enhanced;
using gameaudio::vgm::ym2612_dac_event_kind;
using gameaudio::vgm::ym2612_dac_timed_event;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    ym2612_dac_enhanced dac;
    CHECK(!dac.enabled());
    CHECK(dac.last_byte() == 0x80);
    CHECK(dac.last_level() == 0.0f);

    constexpr std::size_t frames = 12;
    std::array<float, frames> output{};
    const ym2612_dac_timed_event events[] = {
        {0, ym2612_dac_event_kind::enable, 0x80},
        {0, ym2612_dac_event_kind::data, 0x80},
        {4, ym2612_dac_event_kind::data, 0xC0},
        {8, ym2612_dac_event_kind::data, 0x40},
        {10, ym2612_dac_event_kind::enable, 0x00},
    };

    dac.render_timed(events, std::size(events), output.data(), output.size());

    // 0x80 is exact zero. The next known byte, 0xC0, is +0.5 and should be
    // joined linearly over the exact four-sample source interval.
    CHECK(output[0] == 0.0f);
    CHECK(std::abs(output[1] - 0.125f) < 1e-6f);
    CHECK(std::abs(output[2] - 0.25f) < 1e-6f);
    CHECK(std::abs(output[3] - 0.375f) < 1e-6f);

    // Then interpolate from +0.5 to -0.5 using the next real source byte.
    CHECK(std::abs(output[4] - 0.5f) < 1e-6f);
    CHECK(std::abs(output[5] - 0.25f) < 1e-6f);
    CHECK(std::abs(output[6]) < 1e-6f);
    CHECK(std::abs(output[7] + 0.25f) < 1e-6f);

    // Disable is a hard authored boundary, not a smoothing target.
    CHECK(std::abs(output[8] + 0.5f) < 1e-6f);
    CHECK(std::abs(output[9] + 0.5f) < 1e-6f);
    CHECK(output[10] == 0.0f);
    CHECK(output[11] == 0.0f);
    CHECK(!dac.enabled());
    CHECK(dac.last_byte() == 0x40);

    // Reset must discard stream history rather than leaking a prior drum into
    // the next song or seek replay.
    dac.reset();
    CHECK(!dac.enabled());
    CHECK(dac.last_byte() == 0x80);
    CHECK(dac.last_level() == 0.0f);

    return 0;
}
