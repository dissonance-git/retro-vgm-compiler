#include "components/vgm/enhancement/ym2612_dac_block_capture.h"

#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    ym2612_dac_block_capture capture;
    capture.begin_block(1000);
    CHECK(!capture.pan_changed(0));
    CHECK(!capture.pan_changed(1));

    const std::uint8_t pan0[] = {0xB6, 0x40};
    const std::uint8_t pan1[] = {0xB6, 0x80};
    capture.observe(
        command_event{command_event_kind::command, 0, 0, 0x53, pan0, 2}, 1004);
    CHECK(capture.pan_changed(0));
    CHECK(!capture.pan_changed(1));
    CHECK(capture.count(0) == 0);

    capture.observe(
        command_event{command_event_kind::command, 0, 0, 0xA3, pan1, 2}, 1005);
    CHECK(capture.pan_changed(1));
    CHECK(capture.count(1) == 0);

    capture.begin_block(2000);
    CHECK(!capture.pan_changed(0));
    CHECK(!capture.pan_changed(1));
    return 0;
}
