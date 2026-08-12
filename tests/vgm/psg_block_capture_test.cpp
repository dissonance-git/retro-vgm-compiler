#include "../../components/vgm/enhancement/psg_block_capture.h"

#include <cstdint>

using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::psg_block_capture;
using gameaudio::vgm::sn76489_write_kind;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    psg_block_capture capture;
    capture.begin_block(1000);

    const std::uint8_t first = 0x90;
    const std::uint8_t second = 0xA4;
    const std::uint8_t stereo = 0xF3;
    const std::uint8_t ignored = 0x22;

    capture.observe(command_event{command_event_kind::command, 0, 0, 0x50, &first, 1}, 1007);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x30, &second, 1}, 1011);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x4F, &stereo, 1}, 1015);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x52, &ignored, 1}, 1016);

    CHECK(capture.count(0) == 2);
    CHECK(capture.count(1) == 1);
    CHECK(capture.writes(0)[0].sample_offset == 7);
    CHECK(capture.writes(0)[0].kind == sn76489_write_kind::register_write);
    CHECK(capture.writes(0)[0].data == first);
    CHECK(capture.writes(0)[1].sample_offset == 15);
    CHECK(capture.writes(0)[1].kind == sn76489_write_kind::stereo_mask);
    CHECK(capture.writes(1)[0].sample_offset == 11);
    CHECK(capture.writes(1)[0].data == second);
    CHECK(!capture.overflowed(0));
    CHECK(!capture.overflowed(1));

    // Commands replayed before the current block start clamp to offset zero.
    capture.begin_block(2000);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x50, &first, 1}, 1990);
    CHECK(capture.count(0) == 1);
    CHECK(capture.writes(0)[0].sample_offset == 0);

    // Reset events and malformed payloads do not become synthetic PSG writes.
    capture.observe(command_event{command_event_kind::reset}, 2000);
    capture.observe(command_event{command_event_kind::command, 0, 0, 0x50, nullptr, 0}, 2000);
    CHECK(capture.count(0) == 1);

    // New blocks discard the prior event list without allocating or rebuilding
    // the collector itself.
    capture.begin_block(3000);
    CHECK(capture.count(0) == 0);
    CHECK(capture.count(1) == 0);
    CHECK(capture.dropped(0) == 0);

    return 0;
}
