#include "../../components/vgm/enhancement/psg_block_capture.h"
#include "../../components/vgm/enhancement/psg_spatial_routing.h"

#include <array>
#include <cstdint>

using gameaudio::vgm::build_sn76489_route_segments;
using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::psg_block_capture;
using gameaudio::vgm::sn76489_route_segment;
using gameaudio::vgm::sn76489_timed_write;
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

    // Routing is segmented at exact sample offsets. Two mask writes at the same
    // sample collapse to the last mask and never create a zero-length interval.
    const std::array<sn76489_timed_write, 5> route_writes{{
        {2, sn76489_write_kind::register_write, 0x91},
        {3, sn76489_write_kind::stereo_mask, 0x10},
        {3, sn76489_write_kind::stereo_mask, 0x11},
        {7, sn76489_write_kind::stereo_mask, 0x01},
        {10, sn76489_write_kind::stereo_mask, 0x88},
    }};
    std::array<sn76489_route_segment, 4> segments{};
    const auto segmented = build_sn76489_route_segments(
        0xFF,
        route_writes.data(),
        route_writes.size(),
        10,
        segments.data(),
        segments.size());
    CHECK(segmented.valid_order);
    CHECK(!segmented.overflowed);
    CHECK(segmented.segment_count == 3);
    CHECK(segments[0].begin_frame == 0 && segments[0].end_frame == 3 && segments[0].stereo_mask == 0xFF);
    CHECK(segments[1].begin_frame == 3 && segments[1].end_frame == 7 && segments[1].stereo_mask == 0x11);
    CHECK(segments[2].begin_frame == 7 && segments[2].end_frame == 10 && segments[2].stereo_mask == 0x01);
    CHECK(segmented.final_stereo_mask == 0x88);

    // Out-of-order timing is not repaired silently. The source observer is
    // expected to preserve execution order, so this becomes an explicit invalid
    // block instead of ambiguous spatial metadata.
    const std::array<sn76489_timed_write, 2> bad_order{{
        {8, sn76489_write_kind::stereo_mask, 0x10},
        {4, sn76489_write_kind::stereo_mask, 0x01},
    }};
    const auto invalid = build_sn76489_route_segments(
        0xFF,
        bad_order.data(),
        bad_order.size(),
        10,
        segments.data(),
        segments.size());
    CHECK(!invalid.valid_order);

    return 0;
}
