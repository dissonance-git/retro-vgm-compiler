#include "../../components/vgm/enhancement/qsound_spatial_source_bus.h"

#include <cmath>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static bool near(float a, float b, float tolerance = 1.0e-6f) {
    return std::fabs(a - b) <= tolerance;
}

static qsound_native_source_frame make_native(std::uint64_t index, std::int16_t value) {
    qsound_native_source_frame frame;
    frame.native_sample = index;
    frame.source.fill(value);
    return frame;
}

int main() {
    qsound_native_time_map map;
    CHECK(map.configure(2, 4));

    qsound_native_source_window window;
    qsound_native_source_frame native[] = {
        make_native(0, 0),
        make_native(1, 1000),
        make_native(2, 2000),
    };
    CHECK(window.begin_block(native, 3));

    qsound_consumer_source_storage audio;
    CHECK(audio.render(map, window, 0, 4).structurally_valid);
    CHECK(audio.valid());
    CHECK(window.end_block());

    qsound_control_state initial;
    qsound_timed_source_control writes[4];
    writes[0].sample_offset = 1;
    writes[0].write = {qsound_source_control_kind::pan, 0, 0x110};
    writes[1].sample_offset = 2;
    writes[1].write = {qsound_source_control_kind::pan, 1, 0x130};
    writes[2].sample_offset = 3;
    writes[2].write = {qsound_source_control_kind::pcm_echo_contribution, 0, 0x0040};
    writes[3].sample_offset = 4;
    writes[3].write = {qsound_source_control_kind::pan, 2, 0x110};

    qsound_spatial_source_bus_storage bus;
    CHECK(bus.build(audio, initial, writes, 4, true, 4));
    CHECK(bus.valid());

    const auto& view = bus.view();
    CHECK(view.lane_count == qsound_source_count);
    CHECK(view.frame_count == 4u);
    CHECK(view.evidence_event_count == 3u); // frame-4 write is carry state only
    CHECK(view.lanes[0].mono_pcm == audio.lane(0));
    CHECK(view.lanes[0].availability == audio.availability());
    CHECK(view.lanes[0].kind == vgmtooling::model::spatial_audio_lane_kind::dry_source);
    CHECK(view.lanes[0].evidence.stereo_route.present);
    CHECK(near(view.lanes[0].evidence.stereo_route.left_gain, 1.0f));
    CHECK(near(view.lanes[0].evidence.stereo_route.right_gain, 1.0f));

    CHECK(view.evidence_events[0].frame_offset == 1u);
    CHECK(view.evidence_events[0].lane_index == 0u);
    CHECK(near(view.evidence_events[0].evidence.stereo_route.left_gain, 1.0f));
    CHECK(near(view.evidence_events[0].evidence.stereo_route.right_gain, 0.0f));

    CHECK(view.evidence_events[1].frame_offset == 2u);
    CHECK(view.evidence_events[1].lane_index == 1u);
    CHECK(near(view.evidence_events[1].evidence.stereo_route.left_gain, 0.0f));
    CHECK(near(view.evidence_events[1].evidence.stereo_route.right_gain, 1.0f));

    CHECK(view.evidence_events[2].frame_offset == 3u);
    CHECK(view.evidence_events[2].lane_index == 0u);
    CHECK(view.evidence_events[2].evidence.effect_send_known);
    CHECK(view.evidence_events[2].evidence.effect_send_enabled);
    CHECK(!vgmtooling::model::may_claim_authored_3d(view.evidence_events[2].evidence));

    // The block-end control is absent from current-frame automation but present
    // in final carry state for the next block.
    CHECK(bus.final_state().pan(2) == 0x110u);

    qsound_timed_source_control bad_order[2] = {writes[1], writes[0]};
    CHECK(!bus.build(audio, initial, bad_order, 2, true, 4));
    CHECK(!bus.valid());

    CHECK(!bus.build(audio, initial, writes, 4, false, 4));
    CHECK(!bus.valid());

    return 0;
}
