#include "components/vgm/enhancement/spatial_source_bus.h"
#include "components/vgm/enhancement/selected_source_transport.h"
#include "components/vgm/enhancement/ym2151_spatial_route_transport.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

int main() {
    using namespace gameaudio::vgm;

    constexpr std::size_t fm1 =
        static_cast<std::size_t>(ym2151_recomposition_source::fm1);

    // Source quality has already been chosen when the lane enters transport.
    selected_source_queue<ym2151_recomposition_source_count, 4> queue;
    queue.reset(10u);
    selected_source_frame<ym2151_recomposition_source_count> source_frame{};
    source_frame.ordinal = 10u;
    source_frame.source[fm1] = {2.0, 0.0, true, true};
    assert(queue.push_reference(source_frame));

    selected_source_block_storage<ym2151_recomposition_source_count, 2> selected;
    assert(selected.consume(queue, 10u, 1u));
    assert(selected.valid());
    assert(selected.source_present(fm1));

    // The authored OPM route reaches the same delivered ordinal independently.
    ym2151_spatial_route_transport<8, 4> routes;
    routes.reset();
    const std::uint8_t left_only[] = {0x20u, 0x40u};
    command_event route_event{};
    route_event.kind = command_event_kind::command;
    route_event.command = 0x54u;
    route_event.payload = left_only;
    route_event.payload_size = 2u;
    assert(routes.observe(route_event, 10u));

    ym2151_spatial_route_transport<8, 4>::presence_array present{};
    present[fm1] = true;
    ym2151_spatial_route_transport<8, 4>::delivered_block route_block{};
    assert(routes.prepare_delivered_block(10u, 1u, present, route_block));
    assert(route_block.routes_complete);

    // Presentation consumes the selected lane and route evidence without any
    // reference/enhanced quality switch. A left-only value of 2 projects to the
    // same-energy mono object sqrt((2^2 + 0^2) / 2) = sqrt(2).
    spatial_source_bus_storage<ym2151_recomposition_source_count, 2> bus;
    assert(bus.build(selected.sources(), route_block.initial_evidence, 1u));
    assert(bus.valid());
    assert(bus.lane_count() == 1u);
    assert(bus.canonical_source_index(0u) == fm1);
    assert(std::fabs(
        bus.block().lanes[0].mono_pcm[0] - std::sqrt(2.0f)) < 1.0e-6f);
    assert(bus.block().lanes[0].evidence.stereo_route.gain_preapplied);
    assert(bus.block().lanes[0].evidence.stereo_route.left_gain == 1.0f);
    assert(bus.block().lanes[0].evidence.stereo_route.right_gain == 0.0f);

    return 0;
}
