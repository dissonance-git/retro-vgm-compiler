#include "components/vgm/enhancement/genesis_enhanced_recomposition.h"
#include "components/vgm/enhancement/genesis_spatial_source.h"
#include "components/vgm/enhancement/spatial_route_transport.h"
#include "components/vgm/enhancement/timed_spatial_source_bus.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;

constexpr std::size_t fm1_index =
    static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
constexpr std::size_t psg0_index =
    static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);

vgmtooling::model::spatial_source_evidence fm_evidence(bool left, bool right) {
    return make_genesis_spatial_source(
        genesis_spatial_device::ym2612_fm,
        0,
        0,
        1,
        ym2612_authored_route(left, right));
}

vgmtooling::model::spatial_source_evidence psg_evidence(std::uint8_t mask) {
    return make_genesis_spatial_source(
        genesis_spatial_device::sn76489_tone,
        0,
        0,
        1,
        sn76489_authored_route(mask, 0));
}
}

int main() {
    using namespace gameaudio::vgm;
    using vgmtooling::model::spatial_source_evidence_event;

    // The render-ahead sideband keeps absolute output ordinals authoritative.
    // Same-ordinal transitions keep insertion order, while an event exactly at
    // the block end remains queued for the following delivered block.
    spatial_evidence_queue<genesis_recomposition_source_count, 8> queue;
    queue.reset();
    assert(queue.push({1002u, fm1_index, fm_evidence(true, false)}));
    assert(queue.push({1002u, psg0_index, psg_evidence(0x10u)}));
    assert(queue.push({1004u, psg0_index, psg_evidence(0x11u)}));

    std::array<spatial_source_evidence_event, 8> drained{};
    std::size_t drained_count = 0;
    assert(queue.drain_block(1000u, 4u, drained, drained_count));
    assert(drained_count == 2u);
    assert(drained[0].frame_offset == 2u && drained[0].lane_index == fm1_index);
    assert(drained[1].frame_offset == 2u && drained[1].lane_index == psg0_index);
    assert(queue.size() == 1u);

    assert(queue.drain_block(1004u, 2u, drained, drained_count));
    assert(drained_count == 1u);
    assert(drained[0].frame_offset == 0u && drained[0].lane_index == psg0_index);
    assert(queue.size() == 0u);

    // A stale event proves the evidence clock was not drained with the audible
    // clock and must invalidate the sideband rather than shift the transition.
    queue.reset();
    assert(queue.push({99u, fm1_index, fm_evidence(true, true)}));
    assert(!queue.drain_block(100u, 4u, drained, drained_count));
    assert(!queue.valid());

    // Producer ordering is equally authoritative.
    queue.reset();
    assert(queue.push({200u, fm1_index, fm_evidence(true, true)}));
    assert(!queue.push({199u, fm1_index, fm_evidence(false, true)}));
    assert(!queue.valid());

    // Bad source metadata is rejected without advancing queue time/state.
    queue.reset();
    assert(!queue.push({300u, genesis_recomposition_source_count,
        fm_evidence(true, true)}));
    assert(queue.valid());
    assert(queue.size() == 0u);

    // The timed bus compacts absent canonical sources while remapping sparse
    // evidence events to the corresponding compact lane index.
    constexpr std::size_t frames = 4u;
    const float fm_left[frames] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float fm_right[frames] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float psg_left[frames] = {0.5f, 0.5f, 0.5f, 0.5f};
    const float psg_right[frames] = {0.5f, 0.5f, 0.5f, 0.5f};

    timed_spatial_source_bus_storage<genesis_recomposition_source_count, frames, 8>::source_array sources{};
    timed_spatial_source_bus_storage<genesis_recomposition_source_count, frames, 8>::evidence_array evidence{};
    sources[fm1_index] = {fm_left, fm_right, true};
    sources[psg0_index] = {psg_left, psg_right, true};
    evidence[fm1_index] = fm_evidence(true, true);
    evidence[psg0_index] = psg_evidence(0x11u);

    const std::array<spatial_source_evidence_event, 2> events{{
        {1u, psg0_index, psg_evidence(0x10u)},
        {2u, fm1_index, fm_evidence(false, true)},
    }};

    timed_spatial_source_bus_storage<genesis_recomposition_source_count, frames, 8> bus;
    assert(bus.build(sources, evidence, frames, events.data(), events.size()));
    assert(bus.valid());
    const auto& block = bus.block();
    assert(block.lane_count == 2u);
    assert(block.frame_count == frames);
    assert(block.evidence_event_count == 2u);
    assert(block.evidence_events != nullptr);

    // Canonical FM1 is compact lane 0; canonical PSG0 is compact lane 1.
    assert(block.evidence_events[0].frame_offset == 1u);
    assert(block.evidence_events[0].lane_index == 1u);
    assert(block.evidence_events[1].frame_offset == 2u);
    assert(block.evidence_events[1].lane_index == 0u);
    assert(block.evidence_events[0].evidence.stereo_route.gain_preapplied);
    assert(block.evidence_events[1].evidence.stereo_route.gain_preapplied);

    // An event for a canonical source that is absent from this selected block
    // is not silently retargeted to another compact lane.
    auto absent_event = events[0];
    absent_event.lane_index = static_cast<std::size_t>(
        genesis_recomposition_source::sn76489_tone1);
    assert(!bus.build(sources, evidence, frames, &absent_event, 1u));
    assert(bus.last_error() ==
        timed_spatial_source_bus_error::event_for_missing_source);

    // Source-specific adapters preserve event order rather than sorting it.
    const std::array<spatial_source_evidence_event, 2> unordered{{
        {3u, fm1_index, fm_evidence(true, false)},
        {1u, fm1_index, fm_evidence(false, true)},
    }};
    assert(!bus.build(sources, evidence, frames, unordered.data(), unordered.size()));
    assert(bus.last_error() == timed_spatial_source_bus_error::unordered_events);

    return 0;
}
