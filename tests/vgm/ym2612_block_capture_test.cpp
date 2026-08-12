#include "../../components/vgm/enhancement/ym2612_block_capture.h"

#include <cstdint>

using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::ym2612_block_capture;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    ym2612_block_capture capture;
    capture.begin_block(1000);

    const std::uint8_t a[] = {0x30, 0x71};
    const std::uint8_t b[] = {0xB4, 0xC0};
    const std::uint8_t c[] = {0x28, 0xF0};
    const std::uint8_t d[] = {0x2A, 0x88};
    const std::uint8_t psg = 0x90;

    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, a, 2}, 1002);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x53, b, 2}, 1004);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0xA2, c, 2}, 1007);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0xA3, d, 2}, 1010);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x50, &psg, 1}, 1011);

    CHECK(capture.count(0) == 2);
    CHECK(capture.count(1) == 2);

    CHECK(capture.writes(0)[0].sample_offset == 2);
    CHECK(capture.writes(0)[0].port == 0);
    CHECK(capture.writes(0)[0].reg == 0x30);
    CHECK(capture.writes(0)[0].data == 0x71);

    CHECK(capture.writes(0)[1].sample_offset == 4);
    CHECK(capture.writes(0)[1].port == 1);
    CHECK(capture.writes(0)[1].reg == 0xB4);

    CHECK(capture.writes(1)[0].sample_offset == 7);
    CHECK(capture.writes(1)[0].port == 0);
    CHECK(capture.writes(1)[0].reg == 0x28);

    CHECK(capture.writes(1)[1].sample_offset == 10);
    CHECK(capture.writes(1)[1].port == 1);
    CHECK(capture.writes(1)[1].reg == 0x2A);

    // Commands replayed before the block start during a seek clamp safely to 0.
    capture.begin_block(2000);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, a, 2}, 1990);
    CHECK(capture.count(0) == 1);
    CHECK(capture.writes(0)[0].sample_offset == 0);
    CHECK(!capture.overflowed(0));

    return 0;
}
