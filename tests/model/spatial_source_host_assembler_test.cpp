#include "model/spatial_source_host_assembler.h"

#include <cstddef>
#include <cstdint>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static spatial_source_evidence evidence(
    std::uint64_t source_id,
    std::uint64_t generation,
    float left,
    float right)
{
    spatial_source_evidence value;
    value.family = spatial_source_family::vgm;
    value.source_id = source_id;
    value.generation = generation;
    value.physical_slot_present = true;
    value.physical_slot = 0;
    value.stereo_route.present = true;
    value.stereo_route.left_gain = left;
    value.stereo_route.right_gain = right;
    value.stereo_route.authority = spatial_evidence_authority::device_authored;
    return value;
}

int main() {
    {
        spatial_source_host_assembler<2, 16, 16> assembler;
        float lane0[5] = {0, 1, 2, 3, 4};
        float lane1[5] = {10, 11, 12, 13, 14};
        std::uint8_t lane1_available[5] = {1, 1, 0, 1, 1};

        const auto initial = evidence(7, 1, 1.0f, 0.0f);
        auto routed = initial;
        routed.stereo_route.left_gain = 0.0f;
        routed.stereo_route.right_gain = 1.0f;

        spatial_audio_lane_view lanes[2];
        lanes[0].mono_pcm = lane0;
        lanes[0].evidence = initial;
        lanes[1].mono_pcm = lane1;
        lanes[1].availability = lane1_available;
        lanes[1].evidence = evidence(8, 1, 0.5f, 0.5f);

        spatial_source_evidence_event route_event;
        route_event.frame_offset = 3;
        route_event.lane_index = 0;
        route_event.evidence = routed;

        spatial_source_block_view producer;
        producer.lanes = lanes;
        producer.lane_count = 2;
        producer.frame_count = 5;
        producer.evidence_events = &route_event;
        producer.evidence_event_count = 1;
        CHECK(assembler.push(producer));
        CHECK(assembler.buffered_frames() == 5);

        auto first = assembler.pull(2);
        CHECK(first.frame_count == 2);
        CHECK(first.lane_count == 2);
        CHECK(first.evidence_event_count == 0);
        CHECK(first.lanes[0].mono_pcm[0] == 0.0f);
        CHECK(first.lanes[0].mono_pcm[1] == 1.0f);
        CHECK(first.lanes[1].mono_pcm[1] == 11.0f);
        CHECK(first.lanes[0].evidence.stereo_route.left_gain == 1.0f);

        auto second = assembler.pull(2);
        CHECK(second.frame_count == 2);
        CHECK(second.lanes[0].mono_pcm[0] == 2.0f);
        CHECK(second.lanes[0].mono_pcm[1] == 3.0f);
        CHECK(second.lanes[1].availability[0] == 0u);
        CHECK(second.evidence_event_count == 1);
        CHECK(second.evidence_events[0].frame_offset == 1);
        CHECK(second.evidence_events[0].lane_index == 0);
        CHECK(second.evidence_events[0].evidence.stereo_route.right_gain == 1.0f);

        auto third = assembler.pull(9);
        CHECK(third.frame_count == 1);
        CHECK(third.lanes[0].mono_pcm[0] == 4.0f);
        CHECK(third.lanes[0].evidence.stereo_route.left_gain == 0.0f);
        CHECK(third.lanes[0].evidence.stereo_route.right_gain == 1.0f);
        CHECK(assembler.buffered_frames() == 0);
    }

    {
        // Two producer blocks with the same source episode merge cleanly. A
        // block-boundary evidence change becomes a correctly rebased host event.
        spatial_source_host_assembler<1, 16, 16> assembler;
        float a[2] = {1, 2};
        float b[2] = {3, 4};
        const auto left = evidence(11, 2, 1.0f, 0.0f);
        const auto right = evidence(11, 2, 0.0f, 1.0f);
        spatial_audio_lane_view lane_a{spatial_audio_lane_kind::dry_source, a, left};
        spatial_audio_lane_view lane_b{spatial_audio_lane_kind::dry_source, b, right};
        CHECK(assembler.push(spatial_source_block_view{&lane_a, 1, 2}));
        CHECK(assembler.push(spatial_source_block_view{&lane_b, 1, 2}));
        auto combined = assembler.pull(4);
        CHECK(combined.frame_count == 4);
        CHECK(combined.evidence_event_count == 1);
        CHECK(combined.evidence_events[0].frame_offset == 2);
        CHECK(combined.evidence_events[0].evidence.source_id == 11);
        CHECK(combined.evidence_events[0].evidence.stereo_route.right_gain == 1.0f);
    }

    {
        // Reuse of a physical lane by another source episode is not rewritten as
        // a state event. The assembler makes it a hard host-chunk boundary.
        spatial_source_host_assembler<1, 16, 16> assembler;
        float a[3] = {1, 2, 3};
        float b[3] = {4, 5, 6};
        auto first_identity = evidence(21, 4, 1.0f, 1.0f);
        auto second_identity = evidence(22, 1, 1.0f, 1.0f);
        spatial_audio_lane_view lane_a{spatial_audio_lane_kind::dry_source, a, first_identity};
        spatial_audio_lane_view lane_b{spatial_audio_lane_kind::dry_source, b, second_identity};
        CHECK(assembler.push(spatial_source_block_view{&lane_a, 1, 3}));
        CHECK(assembler.push(spatial_source_block_view{&lane_b, 1, 3}));

        auto first = assembler.pull(6);
        CHECK(first.frame_count == 3);
        CHECK(assembler.last_pull_identity_limited());
        CHECK(first.evidence_event_count == 0);
        CHECK(first.lanes[0].evidence.source_id == 21);

        auto second = assembler.pull(6);
        CHECK(second.frame_count == 3);
        CHECK(!assembler.last_pull_identity_limited());
        CHECK(second.evidence_event_count == 0);
        CHECK(second.lanes[0].evidence.source_id == 22);
        CHECK(second.lanes[0].mono_pcm[0] == 4.0f);
    }

    {
        // Evidence-only SPC-style lanes stay explicitly unavailable rather than
        // being mistaken for observed digital silence.
        spatial_source_host_assembler<1, 8, 8> assembler;
        spatial_source_evidence spc;
        spc.family = spatial_source_family::spc;
        spc.source_id = 3;
        spatial_audio_lane_view lane;
        lane.mono_pcm = nullptr;
        lane.evidence = spc;
        CHECK(assembler.push(spatial_source_block_view{&lane, 1, 3}));
        auto block = assembler.pull(3);
        CHECK(block.frame_count == 3);
        CHECK(block.lanes[0].mono_pcm != nullptr);
        CHECK(block.lanes[0].availability[0] == 0u);
        CHECK(block.lanes[0].availability[1] == 0u);
        CHECK(block.lanes[0].availability[2] == 0u);
    }

    {
        // An in-block evidence event is forbidden from changing source identity;
        // that would violate spatial_source_evidence_event's contract.
        spatial_source_host_assembler<1, 8, 8> assembler;
        float pcm[2] = {0, 0};
        auto first = evidence(30, 1, 1.0f, 1.0f);
        auto illegal = evidence(31, 1, 1.0f, 1.0f);
        spatial_audio_lane_view lane{spatial_audio_lane_kind::dry_source, pcm, first};
        spatial_source_evidence_event event{1, 0, illegal};
        spatial_source_block_view block{&lane, 1, 2, &event, 1};
        CHECK(!assembler.push(block));
        CHECK(assembler.last_error() == spatial_source_host_assembler_error::evidence_identity_violation);
        CHECK(assembler.buffered_frames() == 0);
    }

    {
        // Capacity failure is transactional: the already-buffered prefix remains
        // intact and can still be consumed after an oversized push is rejected.
        spatial_source_host_assembler<1, 3, 4> assembler;
        float a[2] = {1, 2};
        float b[2] = {3, 4};
        auto state = evidence(40, 1, 1.0f, 1.0f);
        spatial_audio_lane_view lane_a{spatial_audio_lane_kind::dry_source, a, state};
        spatial_audio_lane_view lane_b{spatial_audio_lane_kind::dry_source, b, state};
        CHECK(assembler.push(spatial_source_block_view{&lane_a, 1, 2}));
        CHECK(!assembler.push(spatial_source_block_view{&lane_b, 1, 2}));
        CHECK(assembler.last_error() == spatial_source_host_assembler_error::capacity_exceeded);
        CHECK(assembler.buffered_frames() == 2);
        auto preserved = assembler.pull(3);
        CHECK(preserved.frame_count == 2);
        CHECK(preserved.lanes[0].mono_pcm[0] == 1.0f);
        CHECK(preserved.lanes[0].mono_pcm[1] == 2.0f);
    }

    return 0;
}
