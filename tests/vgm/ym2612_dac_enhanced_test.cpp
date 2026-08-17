#include "../../components/vgm/enhancement/ym2612_dac_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::ym2612_dac_enhanced;
using gameaudio::vgm::ym2612_dac_event_kind;
using gameaudio::vgm::ym2612_dac_timed_event;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    constexpr std::size_t frames = 12;
    const ym2612_dac_timed_event events[] = {
        {0, ym2612_dac_event_kind::enable, 0x80},
        {0, ym2612_dac_event_kind::data, 0x80},
        {4, ym2612_dac_event_kind::data, 0xC0},
        {8, ym2612_dac_event_kind::data, 0x40},
        {10, ym2612_dac_event_kind::enable, 0x00},
    };

    // Arbitrary direct $2A writes preserve every authored hold exactly.
    ym2612_dac_enhanced direct;
    std::array<float, frames> hold{};
    direct.render_exact_hold(events, std::size(events), hold.data(), hold.size());
    for (std::size_t i = 0; i < 4; ++i) CHECK(hold[i] == 0.0f);
    for (std::size_t i = 4; i < 8; ++i) CHECK(std::abs(hold[i] - 0.5f) < 1e-6f);
    for (std::size_t i = 8; i < 10; ++i) CHECK(std::abs(hold[i] + 0.5f) < 1e-6f);
    CHECK(hold[10] == 0.0f);
    CHECK(hold[11] == 0.0f);
    CHECK(!direct.enabled());
    CHECK(direct.last_byte() == 0x40);

    // The legacy interpolation helper remains explicitly available only for a
    // caller that independently knows these points belong to one PCM stream.
    ym2612_dac_enhanced pcm;
    std::array<float, frames> interpolated{};
    pcm.render_timed(events, std::size(events), interpolated.data(), interpolated.size());
    CHECK(interpolated[0] == 0.0f);
    CHECK(std::abs(interpolated[1] - 0.125f) < 1e-6f);
    CHECK(std::abs(interpolated[2] - 0.25f) < 1e-6f);
    CHECK(std::abs(interpolated[3] - 0.375f) < 1e-6f);
    CHECK(std::abs(interpolated[4] - 0.5f) < 1e-6f);
    CHECK(std::abs(interpolated[5] - 0.25f) < 1e-6f);
    CHECK(std::abs(interpolated[6]) < 1e-6f);
    CHECK(std::abs(interpolated[7] + 0.25f) < 1e-6f);
    CHECK(std::abs(interpolated[8] + 0.5f) < 1e-6f);
    CHECK(std::abs(interpolated[9] + 0.5f) < 1e-6f);
    CHECK(interpolated[10] == 0.0f);
    CHECK(interpolated[11] == 0.0f);

    direct.reset();
    CHECK(!direct.enabled());
    CHECK(direct.last_byte() == 0x80);
    CHECK(direct.last_level() == 0.0f);
    return 0;
}
