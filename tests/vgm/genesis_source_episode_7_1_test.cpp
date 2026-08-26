#include "components/vgm/enhancement/genesis_source_episode_7_1.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

using namespace gameaudio::vgm;

command_event ym_write(
    std::uint8_t reg,
    std::uint8_t data,
    std::uint8_t payload[2])
{
    payload[0] = reg;
    payload[1] = data;
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = 0x52u;
    event.payload = payload;
    event.payload_size = 2u;
    return event;
}

command_event psg_write(std::uint8_t data, std::uint8_t payload[1])
{
    payload[0] = data;
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = 0x50u;
    event.payload = payload;
    event.payload_size = 1u;
    return event;
}

bool near(float a, float b, float tolerance = 1.0e-6f)
{
    return std::fabs(a - b) <= tolerance;
}

} // namespace

int main()
{
    genesis_source_episode_transport<64, 16> transport;

    genesis_source_episode_transport<64, 16>::block_type initial{};
    assert(transport.prepare_delivered_block(0u, 8u, initial));
    for (float depth : initial.initial_depth)
        assert(near(depth, genesis_episode_default_depth));

    std::uint8_t ym_payload[2]{};
    auto event = ym_write(0x28u, 0xF0u, ym_payload);
    assert(transport.observe(event, 8u));

    genesis_source_episode_transport<64, 16>::block_type fm1{};
    assert(transport.prepare_delivered_block(8u, 1u, fm1));
    assert(near(fm1.initial_depth[0], genesis_episode_depth_slots[0]));

    event = ym_write(0x28u, 0x00u, ym_payload);
    assert(transport.observe(event, 9u));
    genesis_source_episode_transport<64, 16>::block_type release{};
    assert(transport.prepare_delivered_block(9u, 1u, release));
    assert(near(release.initial_depth[0], genesis_episode_depth_slots[0]));

    event = ym_write(0x28u, 0xF1u, ym_payload);
    assert(transport.observe(event, 10u));
    genesis_source_episode_transport<64, 16>::block_type fm2{};
    assert(transport.prepare_delivered_block(10u, 1u, fm2));
    assert(near(fm2.initial_depth[1], genesis_episode_depth_slots[1]));

    event = ym_write(0x28u, 0xF0u, ym_payload);
    assert(transport.observe(event, 11u));
    genesis_source_episode_transport<64, 16>::block_type fm1_again{};
    assert(transport.prepare_delivered_block(11u, 1u, fm1_again));
    assert(near(fm1_again.initial_depth[0], genesis_episode_depth_slots[2]));
    assert(transport.producer_assignment(0).generation == 2u);

    std::uint8_t psg_payload[1]{};
    auto psg = psg_write(0x90u, psg_payload);
    assert(transport.observe(psg, 12u));
    genesis_source_episode_transport<64, 16>::block_type psg_block{};
    assert(transport.prepare_delivered_block(12u, 1u, psg_block));
    constexpr std::size_t psg0 = static_cast<std::size_t>(
        genesis_recomposition_source::sn76489_tone0);
    assert(!near(psg_block.initial_depth[psg0], genesis_episode_default_depth));

    event = ym_write(0x2Bu, 0x80u, ym_payload);
    assert(transport.observe(event, 13u));
    genesis_source_episode_transport<64, 16>::block_type dac_block{};
    assert(transport.prepare_delivered_block(13u, 1u, dac_block));
    constexpr std::size_t dac = static_cast<std::size_t>(
        genesis_recomposition_source::ym2612_dac);
    assert(!near(dac_block.initial_depth[dac], genesis_episode_default_depth));

    transport.begin_replay();
    event = ym_write(0x28u, 0xF0u, ym_payload);
    assert(transport.observe(event, 200u));
    event = ym_write(0x28u, 0x00u, ym_payload);
    assert(transport.observe(event, 220u));
    event = ym_write(0x28u, 0xF1u, ym_payload);
    assert(transport.observe(event, 240u));
    assert(transport.replay_mode());
    transport.end_replay();
    assert(!transport.replay_mode());

    genesis_source_episode_transport<64, 16>::block_type seek_block{};
    assert(transport.prepare_delivered_block(240u, 16u, seek_block));
    assert(near(
        seek_block.initial_depth[1],
        genesis_episode_depth_slots[
            transport.producer_assignment(1).depth_slot]));

    return 0;
}
