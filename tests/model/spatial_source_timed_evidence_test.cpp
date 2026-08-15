#include "model/spatial_source.h"

#include <cstddef>
#include <cstdint>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    float mono[4] = {0.0f, 0.25f, -0.25f, 0.0f};
    std::uint8_t availability[4] = {0u, 1u, 1u, 1u};

    spatial_source_evidence initial;
    initial.family = spatial_source_family::vgm;
    initial.source_id = 7;
    initial.stereo_route.present = true;
    initial.stereo_route.left_gain = 1.0f;
    initial.stereo_route.right_gain = 0.0f;
    initial.stereo_route.authority = spatial_evidence_authority::device_authored;

    spatial_audio_lane_view lane;
    lane.kind = spatial_audio_lane_kind::dry_source;
    lane.mono_pcm = mono;
    lane.evidence = initial;
    lane.availability = availability;

    spatial_source_evidence changed = initial;
    changed.stereo_route.left_gain = 0.0f;
    changed.stereo_route.right_gain = 1.0f;

    spatial_source_evidence_event event;
    event.frame_offset = 2;
    event.lane_index = 0;
    event.evidence = changed;

    spatial_source_block_view block;
    block.lanes = &lane;
    block.lane_count = 1;
    block.frame_count = 4;
    block.evidence_events = &event;
    block.evidence_event_count = 1;

    CHECK(block.lanes[0].availability[0] == 0u);
    CHECK(block.lanes[0].availability[1] == 1u);
    CHECK(block.lanes[0].evidence.stereo_route.left_gain == 1.0f);
    CHECK(block.evidence_events[0].frame_offset == 2u);
    CHECK(block.evidence_events[0].lane_index == 0u);
    CHECK(block.evidence_events[0].evidence.source_id == initial.source_id);
    CHECK(block.evidence_events[0].evidence.stereo_route.right_gain == 1.0f);
    CHECK(!may_claim_authored_3d(block.evidence_events[0].evidence));

    // Existing sources that have complete audio and static evidence need no
    // extra sidebands. Null availability means all lane PCM is source evidence;
    // no events means the frame-zero evidence stays in force for the block.
    spatial_audio_lane_view static_lane{
        spatial_audio_lane_kind::dry_source,
        mono,
        initial,
    };
    spatial_source_block_view static_block{&static_lane, 1, 4};
    CHECK(static_lane.availability == nullptr);
    CHECK(static_block.evidence_events == nullptr);
    CHECK(static_block.evidence_event_count == 0u);

    return 0;
}
