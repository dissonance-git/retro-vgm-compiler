#include "components/vgm/enhancement/genesis_spatial_route_transport.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;

command_event command(
    std::uint8_t opcode,
    const std::uint8_t* payload,
    std::size_t payload_size) {
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = opcode;
    event.payload = payload;
    event.payload_size = payload_size;
    return event;
}

constexpr std::size_t fm1 =
    static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
constexpr std::size_t fm6 =
    static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm6);
constexpr std::size_t dac =
    static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac);
constexpr std::size_t psg0 =
    static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);
constexpr std::size_t psg3 =
    static_cast<std::size_t>(genesis_recomposition_source::sn76489_noise);
}

int main() {
    using transport_type = genesis_spatial_route_transport<32, 16>;
    transport_type transport;
    transport.reset();

    // The source-aware backend requires pinned Nuked OPN2, whose reset sets all
    // six pan_l/pan_r values to one. FM and channel-6 DAC therefore have exact
    // both-output routing before the first B4-B6 write.
    transport_type::presence_array present{};
    present[fm1] = true;
    present[fm6] = true;
    present[dac] = true;
    transport_type::delivered_block block{};
    assert(transport.prepare_delivered_block(0u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[fm6].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm6].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[dac].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[dac].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[fm6].source_id != block.initial_evidence[dac].source_id);

    // Pinned libvgm SN76496 resets stereo_mask to 0xFF. All four PSG lanes also
    // have an exact both-output route before any 0x4F write.
    transport.reset();
    present = {};
    present[psg0] = true;
    present[psg3] = true;
    assert(transport.prepare_delivered_block(0u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[psg0].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[psg0].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[psg3].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[psg3].stereo_route.right_gain == 1.0f);

    // A route write at the first audible sample replaces reset evidence rather
    // than becoming a redundant timed event at offset zero.
    transport.reset();
    const std::uint8_t fm1_left_only[] = {0xB4u, 0x80u};
    assert(transport.observe(command(0x52u, fm1_left_only, 2u), 100u));
    present = {};
    present[fm1] = true;
    assert(transport.prepare_delivered_block(100u, 8u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);
    assert(block.initial_evidence[fm1].stereo_route.present);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 0.0f);

    // Port 1 B6 is physical FM channel 6 and therefore also establishes the
    // distinct DAC source's authored output route.
    transport.reset();
    const std::uint8_t fm6_right_only[] = {0xB6u, 0x40u};
    assert(transport.observe(command(0x53u, fm6_right_only, 2u), 200u));
    present = {};
    present[fm6] = true;
    present[dac] = true;
    assert(transport.prepare_delivered_block(200u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm6].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[fm6].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[dac].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[dac].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[fm6].source_id != block.initial_evidence[dac].source_id);

    // Game Gear stereo command overrides all four exact reset routes from the
    // documented bit layout. 0x11 = tone0 both sides, all other lanes disabled.
    transport.reset();
    const std::uint8_t psg_mask[] = {0x11u};
    assert(transport.observe(command(0x4Fu, psg_mask, 1u), 300u));
    present = {};
    present[psg0] = true;
    present[psg3] = true;
    assert(transport.prepare_delivered_block(300u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[psg0].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[psg0].stereo_route.right_gain == 1.0f);
    assert(block.initial_evidence[psg3].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[psg3].stereo_route.right_gain == 0.0f);

    // Later in-block changes survive as timed evidence rather than being
    // collapsed to one state for the whole block.
    transport.reset();
    const std::uint8_t fm1_both[] = {0xB4u, 0xC0u};
    const std::uint8_t fm1_right[] = {0xB4u, 0x40u};
    assert(transport.observe(command(0x52u, fm1_both, 2u), 400u));
    assert(transport.observe(command(0x52u, fm1_right, 2u), 403u));
    present = {};
    present[fm1] = true;
    assert(transport.prepare_delivered_block(400u, 8u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);
    assert(block.event_count == 1u);
    assert(block.events[0].frame_offset == 3u);
    assert(block.events[0].lane_index == fm1);
    assert(block.events[0].evidence.stereo_route.left_gain == 0.0f);
    assert(block.events[0].evidence.stereo_route.right_gain == 1.0f);

    // The in-block transition becomes next block's initial state even if a
    // caller chose ordinary stereo for the previous block.
    assert(transport.prepare_delivered_block(408u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);

    // Second-instance YM commands are not folded into the primary 11-lane
    // topology. They leave the primary lane on its exact reset route.
    transport.reset();
    const std::uint8_t second_instance[] = {0xB4u, 0x00u};
    assert(transport.observe(command(0xA2u, second_instance, 2u), 500u));
    present = {};
    present[fm1] = true;
    assert(transport.prepare_delivered_block(500u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);

    return 0;
}
