#include "../../components/vgm/enhancement/ym2612_dac_block_capture.h"

#include <cstdint>

using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::ym2612_dac_block_capture;
using gameaudio::vgm::ym2612_dac_event_kind;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    ym2612_dac_block_capture capture;
    capture.begin_block(1000);

    const std::uint8_t enable[] = {0x2B, 0x80};
    const std::uint8_t direct[] = {0x2A, 0x66};
    const std::uint8_t second_chip[] = {0x2A, 0x77};
    const std::uint8_t resolved = 0x88;
    const std::uint8_t unrelated[] = {0x22, 0x08};

    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, enable, 2}, 1000);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, direct, 2}, 1004);
    capture.observe(command_event{command_event_kind::ym2612_dac, 0, 0, 0x8A, &resolved, 1}, 1007);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0xA2, second_chip, 2}, 1009);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, unrelated, 2}, 1010);

    CHECK(capture.count(0) == 3);
    CHECK(capture.count(1) == 1);

    CHECK(capture.events(0)[0].sample_offset == 0);
    CHECK(capture.events(0)[0].kind == ym2612_dac_event_kind::enable);
    CHECK(capture.events(0)[0].value == 0x80);

    CHECK(capture.events(0)[1].sample_offset == 4);
    CHECK(capture.events(0)[1].kind == ym2612_dac_event_kind::data);
    CHECK(capture.events(0)[1].value == 0x66);

    CHECK(capture.events(0)[2].sample_offset == 7);
    CHECK(capture.events(0)[2].kind == ym2612_dac_event_kind::data);
    CHECK(capture.events(0)[2].value == 0x88);

    CHECK(capture.events(1)[0].sample_offset == 9);
    CHECK(capture.events(1)[0].value == 0x77);
    CHECK(!capture.overflowed(0));
    CHECK(!capture.overflowed(1));

    // Pre-block seek replay clamps to sample zero rather than underflowing.
    capture.begin_block(2000);
    capture.observe(command_event{command_event_kind::ym2612_dac, 0, 0, 0x80, &resolved, 1}, 1990);
    CHECK(capture.count(0) == 1);
    CHECK(capture.events(0)[0].sample_offset == 0);

    return 0;
}
