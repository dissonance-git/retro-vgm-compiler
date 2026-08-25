#include "components/vgm/enhancement/genesis_spatial_route_transport.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

using namespace gameaudio::vgm;

namespace {
command_event command(std::uint8_t opcode, const std::uint8_t* payload, std::size_t size) {
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = opcode;
    event.payload = payload;
    event.payload_size = size;
    return event;
}
}

int main() {
    using transport_type = genesis_spatial_route_transport<32, 16>;
    transport_type transport;
    transport.reset();

    constexpr std::size_t fm1 =
        static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
    constexpr std::size_t psg0 =
        static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);

    auto fm_evidence = make_genesis_spatial_source(
        genesis_spatial_device::ym2612_fm,
        0,
        0,
        1,
        ym2612_authored_route(true, true));
    fm_evidence.persistent_part_present = true;
    fm_evidence.persistent_part_id = 4242u;
    fm_evidence.persistent_part_confidence = 0.81f;
    assert(transport.seed(fm1, fm_evidence));

    const std::uint8_t fm_right_only[] = {0xB4u, 0x40u};
    assert(transport.observe(command(0x52u, fm_right_only, 2u), 100u));

    transport_type::presence_array present{};
    present[fm1] = true;
    transport_type::delivered_block block{};
    assert(transport.prepare_delivered_block(100u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[fm1].persistent_part_present);
    assert(block.initial_evidence[fm1].persistent_part_id == 4242u);
    assert(block.initial_evidence[fm1].persistent_part_confidence == 0.81f);
    assert(block.initial_evidence[fm1].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[fm1].stereo_route.right_gain == 1.0f);

    auto psg_evidence = make_genesis_spatial_source(
        genesis_spatial_device::sn76489_tone,
        0,
        0,
        1,
        sn76489_authored_route(0xFFu, 0u));
    psg_evidence.persistent_part_present = true;
    psg_evidence.persistent_part_id = 5252u;
    psg_evidence.persistent_part_confidence = 0.77f;
    assert(transport.seed(psg0, psg_evidence));

    const std::uint8_t psg_left_only[] = {0x10u};
    assert(transport.observe(command(0x4Fu, psg_left_only, 1u), 110u));
    present = {};
    present[psg0] = true;
    assert(transport.prepare_delivered_block(110u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[psg0].persistent_part_present);
    assert(block.initial_evidence[psg0].persistent_part_id == 5252u);
    assert(block.initial_evidence[psg0].persistent_part_confidence == 0.77f);
    assert(block.initial_evidence[psg0].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[psg0].stereo_route.right_gain == 0.0f);

    return 0;
}
