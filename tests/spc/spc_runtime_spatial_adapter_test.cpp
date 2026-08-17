#include "components/spc/spc_runtime_spatial_adapter.h"
#include "model/spatial_source_host_session.h"

#include <cstddef>
#include <cstdint>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static spc_runtime_capture_record record(
    std::uint64_t trace,
    spc_voice_runtime_event_kind kind,
    std::int64_t tick,
    std::uint8_t voice)
{
    spc_runtime_capture_record value;
    value.trace_index = trace;
    value.kind = kind;
    value.tick = tick;
    value.tick_rate = 1000;
    value.voice = voice;
    value.fields = to_fields(spc_runtime_capture_field::voice);
    return value;
}

int main() {
    {
        spc_runtime_spatial_adapter<8, 32> adapter;
        adapter.reset();

        spc_runtime_capture_record records[4];
        records[0] = record(0, spc_voice_runtime_event_kind::routing_state_changed, 100, 2);
        records[0].fields = records[0].fields |
            spc_runtime_capture_field::route_gain_left |
            spc_runtime_capture_field::route_gain_right |
            spc_runtime_capture_field::echo_send_enabled;
        records[0].route_gain_left = -128;
        records[0].route_gain_right = 64;
        records[0].echo_send_enabled = true;

        records[1] = record(1, spc_voice_runtime_event_kind::key_on_accepted, 102, 2);
        records[1].fields = records[1].fields | spc_runtime_capture_field::source_index;
        records[1].source_index = 7;

        // SRCN changes inside the same physical voice episode. It must not create
        // a new renderer/source generation by itself.
        records[2] = record(2, spc_voice_runtime_event_kind::source_latched, 104, 2);
        records[2].fields = records[2].fields | spc_runtime_capture_field::source_index;
        records[2].source_index = 8;

        records[3] = record(3, spc_voice_runtime_event_kind::routing_state_changed, 105, 2);
        records[3].fields = records[3].fields |
            spc_runtime_capture_field::route_gain_right |
            spc_runtime_capture_field::echo_send_enabled;
        records[3].route_gain_right = -64;
        records[3].echo_send_enabled = false;

        const spc_runtime_spatial_capture_view capture{records, 4, false};
        CHECK(adapter.build_window(capture, 100, 1000, 1000, 10));
        CHECK(adapter.segment_count() == 2);

        const auto* segments = adapter.segments();
        CHECK(segments[0].reference_frame_offset == 0);
        CHECK(segments[0].sources.frame_count == 2);
        CHECK(segments[0].sources.lane_count == 8);
        CHECK(segments[0].sources.evidence_event_count == 0);
        CHECK(segments[0].sources.lanes[2].mono_pcm == nullptr);
        CHECK(segments[0].sources.lanes[2].evidence.family == spatial_source_family::spc);
        CHECK(segments[0].sources.lanes[2].evidence.source_id == 3);
        CHECK(segments[0].sources.lanes[2].evidence.generation == 0);
        CHECK(segments[0].sources.lanes[2].evidence.stereo_route.present);
        CHECK(segments[0].sources.lanes[2].evidence.stereo_route.left_gain == -1.0f);
        CHECK(segments[0].sources.lanes[2].evidence.stereo_route.right_gain == 0.5f);
        CHECK(segments[0].sources.lanes[2].evidence.effect_send_known);
        CHECK(segments[0].sources.lanes[2].evidence.effect_send_enabled);

        CHECK(segments[1].reference_frame_offset == 2);
        CHECK(segments[1].sources.frame_count == 8);
        CHECK(segments[1].sources.lanes[2].evidence.generation == 1);
        CHECK(segments[1].sources.evidence_event_count == 1);
        CHECK(segments[1].sources.evidence_events[0].frame_offset == 3);
        CHECK(segments[1].sources.evidence_events[0].lane_index == 2);
        CHECK(segments[1].sources.evidence_events[0].evidence.generation == 1);
        CHECK(segments[1].sources.evidence_events[0].evidence.stereo_route.left_gain == -1.0f);
        CHECK(segments[1].sources.evidence_events[0].evidence.stereo_route.right_gain == -0.5f);
        CHECK(!segments[1].sources.evidence_events[0].evidence.effect_send_enabled);

        // Feed the exact same segments through the generic host session. The
        // generation change must become a hard output boundary at reference 502.
        spatial_source_host_session<8, 32, 64> session;
        session.reset(spatial_source_host_discontinuity::initialize, 500);
        CHECK(session.push_at(500, segments[0].sources));
        CHECK(session.push_at(502, segments[1].sources));

        const auto first = session.pull(10);
        CHECK(first.reference_frame_start == 500);
        CHECK(first.sources.frame_count == 2);
        CHECK(session.last_pull_identity_limited());
        CHECK(first.sources.lanes[2].evidence.generation == 0);
        CHECK(first.sources.lanes[2].availability[0] == 0u);
        CHECK(first.sources.lanes[2].availability[1] == 0u);

        const auto second = session.pull(10);
        CHECK(second.reference_frame_start == 502);
        CHECK(second.sources.frame_count == 8);
        CHECK(second.sources.lanes[2].evidence.generation == 1);
        CHECK(second.sources.evidence_event_count == 1);
        CHECK(second.sources.evidence_events[0].frame_offset == 3);
        CHECK(second.sources.evidence_events[0].evidence.stereo_route.right_gain == -0.5f);
    }

    {
        // Capture ordinal continuity crosses window boundaries. A hidden gap is
        // rejected before the persistent voice state can mutate.
        spc_runtime_spatial_adapter<4, 8> adapter;
        adapter.reset();
        auto first = record(10, spc_voice_runtime_event_kind::key_on_accepted, 0, 0);
        CHECK(adapter.build_window({&first, 1, false}, 0, 1000, 1000, 4));
        CHECK(adapter.state().voices[0].generation == 1);

        auto gap = record(12, spc_voice_runtime_event_kind::key_on_accepted, 4, 0);
        CHECK(!adapter.build_window({&gap, 1, false}, 4, 1000, 1000, 4));
        CHECK(adapter.last_error() == spc_runtime_spatial_adapter_error::continuity_lost_requires_reset);
        CHECK(adapter.state().voices[0].generation == 1);
        CHECK(adapter.segment_count() == 0);
    }

    {
        spc_runtime_spatial_adapter<4, 8> adapter;
        adapter.reset();
        auto lost = record(0, spc_voice_runtime_event_kind::continuation_lost, 0, 0);
        lost.fields = to_fields(spc_runtime_capture_field::none);
        CHECK(!adapter.build_window({&lost, 1, false}, 0, 1000, 1000, 4));
        CHECK(adapter.last_error() == spc_runtime_spatial_adapter_error::continuity_lost_requires_reset);

        adapter.reset();
        auto reset_event = record(0, spc_voice_runtime_event_kind::execution_reset, 0, 0);
        reset_event.fields = to_fields(spc_runtime_capture_field::none);
        CHECK(!adapter.build_window({&reset_event, 1, false}, 0, 1000, 1000, 4));
        CHECK(adapter.last_error() == spc_runtime_spatial_adapter_error::execution_reset_requires_reset);

        adapter.reset();
        CHECK(!adapter.build_window({nullptr, 0, true}, 0, 1000, 1000, 4));
        CHECK(adapter.last_error() == spc_runtime_spatial_adapter_error::capture_overflow);
    }

    {
        // Signed S-DSP volumes are evidence, not inferred coordinates. Even hard
        // inversion stays a native stereo route and never becomes authored 3-D.
        spc_runtime_spatial_adapter<4, 8> adapter;
        adapter.reset();
        auto route = record(0, spc_voice_runtime_event_kind::routing_state_changed, 0, 7);
        route.fields = route.fields |
            spc_runtime_capture_field::route_gain_left |
            spc_runtime_capture_field::route_gain_right;
        route.route_gain_left = 127;
        route.route_gain_right = -128;
        CHECK(adapter.build_window({&route, 1, false}, 0, 1000, 1000, 2));
        const auto& evidence = adapter.segments()[0].sources.lanes[7].evidence;
        CHECK(evidence.stereo_route.present);
        CHECK(evidence.stereo_route.left_gain == 127.0f / 128.0f);
        CHECK(evidence.stereo_route.right_gain == -1.0f);
        CHECK(!evidence.authored_position_present);
        CHECK(!may_claim_authored_3d(evidence));
    }

    return 0;
}
