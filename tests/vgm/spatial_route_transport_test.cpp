#include "components/vgm/enhancement/spatial_route_transport.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;
using namespace vgmtooling::model;

spatial_source_evidence evidence(std::uint64_t id, float left, float right) {
    spatial_source_evidence result{};
    result.source_id = id;
    result.family = spatial_source_family::vgm;
    result.stereo_route.present = true;
    result.stereo_route.left_gain = left;
    result.stereo_route.right_gain = right;
    result.stereo_route.authority = spatial_evidence_authority::device_authored;
    return result;
}
}

int main() {
    using namespace gameaudio::vgm;
    using transport_type = spatial_route_delivery_transport<2, 8, 4>;

    transport_type transport;
    transport.reset();
    transport_type::presence_array present{};
    present[0] = true;
    transport_type::delivered_block block{};

    // Missing exact initial route is a nonfatal presentation miss: transport
    // remains coherent but Spatial must decline for the affected block.
    assert(transport.prepare_delivered_block(0u, 4u, present, block));
    assert(!block.routes_complete);
    assert(transport.last_error() == spatial_route_transport_error::missing_initial_route);
    assert(transport.valid());

    // A first-sample route write becomes initial block evidence. A later write
    // remains a timed event and becomes the following block's initial state.
    transport.reset();
    assert(transport.publish(10u, 0u, evidence(1u, 1.0f, 0.0f)));
    assert(transport.publish(12u, 0u, evidence(1u, 0.0f, 1.0f)));
    assert(transport.prepare_delivered_block(10u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[0].stereo_route.left_gain == 1.0f);
    assert(block.initial_evidence[0].stereo_route.right_gain == 0.0f);
    assert(block.event_count == 1u);
    assert(block.events[0].frame_offset == 2u);
    assert(block.events[0].evidence.stereo_route.right_gain == 1.0f);

    assert(transport.prepare_delivered_block(14u, 2u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);
    assert(block.initial_evidence[0].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[0].stereo_route.right_gain == 1.0f);

    // Route-only transitions preserve source-semantic evidence already bound to
    // the same source generation. A route register write is not evidence that
    // persistent musical identity or presentation state disappeared.
    transport.reset();
    auto bound = evidence(7u, 1.0f, 1.0f);
    bound.generation = 4u;
    bound.persistent_part_present = true;
    bound.persistent_part_id = 99u;
    bound.persistent_part_confidence = 0.73f;
    bound.presentation.foreground = 0.62f;
    bound.presentation.confidence = 0.51f;
    assert(transport.seed(0u, bound));
    auto right_only = bound.stereo_route;
    right_only.left_gain = 0.0f;
    right_only.right_gain = 1.0f;
    assert(transport.publish_stereo_route(20u, 0u, right_only));
    assert(transport.prepare_delivered_block(20u, 2u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[0].source_id == 7u);
    assert(block.initial_evidence[0].generation == 4u);
    assert(block.initial_evidence[0].persistent_part_present);
    assert(block.initial_evidence[0].persistent_part_id == 99u);
    assert(block.initial_evidence[0].persistent_part_confidence == 0.73f);
    assert(block.initial_evidence[0].presentation.foreground == 0.62f);
    assert(block.initial_evidence[0].presentation.confidence == 0.51f);
    assert(block.initial_evidence[0].stereo_route.left_gain == 0.0f);
    assert(block.initial_evidence[0].stereo_route.right_gain == 1.0f);

    // A route-only transition cannot invent source identity from a bare route.
    transport.reset();
    assert(!transport.publish_stereo_route(25u, 0u, right_only));
    assert(!transport.valid());

    // Route events for an absent source still advance state for future blocks,
    // but are not forwarded as presentation events for the current block.
    transport.reset();
    assert(transport.seed(0u, evidence(1u, 1.0f, 1.0f)));
    assert(transport.publish(30u, 1u, evidence(2u, 0.0f, 1.0f)));
    present = {};
    present[0] = true;
    assert(transport.prepare_delivered_block(30u, 4u, present, block));
    assert(block.routes_complete);
    assert(block.event_count == 0u);

    present = {};
    present[1] = true;
    assert(transport.prepare_delivered_block(34u, 1u, present, block));
    assert(block.routes_complete);
    assert(block.initial_evidence[1].stereo_route.right_gain == 1.0f);

    return 0;
}
