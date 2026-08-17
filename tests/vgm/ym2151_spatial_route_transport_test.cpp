#include "components/vgm/enhancement/ym2151_spatial_route_transport.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;

command_event command(std::uint8_t opcode, const std::uint8_t* payload) {
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = opcode;
    event.payload = payload;
    event.payload_size = 2u;
    return event;
}
}

int main() {
    using namespace gameaudio::vgm;
    using transport_type = ym2151_spatial_route_transport<16, 8>;

    constexpr std::size_t fm1 =
        static_cast<std::size_t>(ym2151_recomposition_source::fm1);
    constexpr std::size_t fm8 =
        static_cast<std::size_t>(ym2151_recomposition_source::fm8);

    transport_type transport;
    transport.reset();
    transport_type::presence_array present{};
    present[fm1] = true;
    transport_type::delivered_block block{};

    // OPM reset routing is intentionally not guessed. An audible source with no
    // authored route evidence declines Spatial but leaves the transport valid.
    assert(transport.prepare_delivered_block(0u, 4u, present, block));
    assert(!block.routes_complete);
    assert(transport.last_error() ==
        ym2151_spatial_route_transport_error::missing_initial_route);
    assert(transport.valid());

    // Primary VGM YM2151 command 0x54, register 0x20 channel 0. The pinned core
    // maps bit 0x40 to left and 0x80 to right output enable.
    transport.reset();
    const std::uint8_t fm1_left[] = {0x20u, 0x40u};
    assert(transport.observe(command(0x54u, fm1_left), 100u));
    assert(transport.prepare_delivered_block(100u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 0.0f);

    // Later authored route change is preserved at the exact delivered offset.
    const std::uint8_t fm1_right[] = {0x20u, 0x80u};
    assert(transport.observe(command(0x54u, fm1_right), 106u));
    assert(transport.prepare_delivered_block(104u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 1u);
    assert(block.events[0].frame_offset == 2u);
    assert(block.events[0].lane_index == fm1);
    assert(block.events[0].evidence.stereo_route.left_gain == 0.0f);
    assert(block.events[0].evidence.stereo_route.right_gain == 1.0f);

    assert(transport.prepare_delivered_block(108u, 2u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);

    // State replay can seed an exact current OPM route without inventing old
    // command ordinals. Channel 8 proves all eight source identities share it.
    transport.reset();
    assert(transport.seed_channel_route(fm8, 0xC0u));
    present = {};
    present[fm8] = true;
    assert(transport.prepare_delivered_block(200u, 1u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm8].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm8].stereo_route.right_gain == 1.0f);

    // The dual-chip mirror 0xA4 is outside this primary eight-lane topology.
    transport.reset();
    const std::uint8_t second_instance[] = {0x20u, 0xC0u};
    assert(transport.observe(command(0xA4u, second_instance), 300u));
    present = {};
    present[fm1] = true;
    assert(transport.prepare_delivered_block(300u, 1u, present, block));
    assert(!block.routes_complete);

    return 0;
}
