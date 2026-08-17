#include "components/spc/spc_runtime_host_pipeline.h"

#include <cstdint>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static spc_runtime_capture_record make_record(
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
        spc_runtime_host_pipeline<16, 8, 32, 96> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 1000);
        CHECK(pipeline.active());
        CHECK(pipeline.session_epoch() == 1);

        spc_runtime_capture_record records[3];
        records[0] = make_record(
            0,
            spc_voice_runtime_event_kind::routing_state_changed,
            100,
            2);
        records[0].fields = records[0].fields |
            spc_runtime_capture_field::route_gain_left |
            spc_runtime_capture_field::route_gain_right |
            spc_runtime_capture_field::echo_send_enabled;
        records[0].route_gain_left = -128;
        records[0].route_gain_right = 64;
        records[0].echo_send_enabled = true;

        records[1] = make_record(
            1,
            spc_voice_runtime_event_kind::key_on_accepted,
            102,
            2);
        records[1].fields = records[1].fields |
            spc_runtime_capture_field::source_index;
        records[1].source_index = 7;

        records[2] = make_record(
            2,
            spc_voice_runtime_event_kind::routing_state_changed,
            105,
            2);
        records[2].fields = records[2].fields |
            spc_runtime_capture_field::route_gain_right |
            spc_runtime_capture_field::echo_send_enabled;
        records[2].route_gain_right = -64;
        records[2].echo_send_enabled = false;

        CHECK(pipeline.consume_reference_window(
            1000,
            8,
            100,
            1000,
            1000,
            {records, 3, false}));
        CHECK(pipeline.expected_reference_frame() == 1008);
        CHECK(pipeline.buffered_frames() == 8);

        const auto first = pipeline.pull(8);
        CHECK(first.reference_frame_start == 1000);
        CHECK(first.session_epoch == 1);
        CHECK(first.sources.frame_count == 2);
        CHECK(pipeline.last_pull_identity_limited());
        CHECK(first.sources.lane_count == 8);
        CHECK(first.sources.lanes[2].evidence.source_id == 3);
        CHECK(first.sources.lanes[2].evidence.generation == 0);
        CHECK(first.sources.lanes[2].evidence.stereo_route.left_gain == -1.0f);
        CHECK(first.sources.lanes[2].evidence.stereo_route.right_gain == 0.5f);
        CHECK(first.sources.lanes[2].availability[0] == 0u);

        const auto second = pipeline.pull(8);
        CHECK(second.reference_frame_start == 1002);
        CHECK(second.session_epoch == 1);
        CHECK(second.sources.frame_count == 6);
        CHECK(second.sources.lanes[2].evidence.generation == 1);
        CHECK(second.sources.evidence_event_count == 1);
        CHECK(second.sources.evidence_events[0].frame_offset == 3);
        CHECK(second.sources.evidence_events[0].lane_index == 2);
        CHECK(second.sources.evidence_events[0].evidence.stereo_route.right_gain == -0.5f);
        CHECK(!second.sources.evidence_events[0].evidence.effect_send_enabled);
        CHECK(pipeline.buffered_frames() == 0);
    }

    {
        // The protected reference decoder must drain one semantic render window
        // before accepting the next. Downstream may still pull that window in any
        // smaller chunk sizes it wants.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 0);
        CHECK(pipeline.consume_reference_window(
            0, 4, 0, 1000, 1000, {nullptr, 0, false}));
        CHECK(!pipeline.consume_reference_window(
            4, 4, 4, 1000, 1000, {nullptr, 0, false}));
        CHECK(pipeline.last_error() ==
            spc_runtime_host_pipeline_error::undrained_reference_window);
        CHECK(pipeline.expected_reference_frame() == 4);
        CHECK(pipeline.buffered_frames() == 4);

        const auto a = pipeline.pull(1);
        CHECK(a.sources.frame_count == 1);
        const auto b = pipeline.pull(3);
        CHECK(b.sources.frame_count == 3);
        CHECK(pipeline.buffered_frames() == 0);
        CHECK(pipeline.consume_reference_window(
            4, 4, 4, 1000, 1000, {nullptr, 0, false}));
    }

    {
        // Reference mismatch is rejected before the SPC adapter consumes the
        // capture ordinal or mutates the physical-voice generation.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 20);
        auto onset = make_record(
            0,
            spc_voice_runtime_event_kind::key_on_accepted,
            10,
            1);
        CHECK(!pipeline.consume_reference_window(
            21, 2, 10, 1000, 1000, {&onset, 1, false}));
        CHECK(pipeline.last_error() ==
            spc_runtime_host_pipeline_error::noncontiguous_reference_window);
        CHECK(pipeline.spatial_state().voices[1].generation == 0);

        CHECK(pipeline.consume_reference_window(
            20, 2, 10, 1000, 1000, {&onset, 1, false}));
        CHECK(pipeline.spatial_state().voices[1].generation == 1);
    }

    {
        // A seek is a new controlled execution. Buffered pre-seek semantics are
        // discarded and capture ordinals may restart from zero without being
        // mistaken for continuity with the old execution.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 0);
        auto first_onset = make_record(
            0,
            spc_voice_runtime_event_kind::key_on_accepted,
            0,
            0);
        CHECK(pipeline.consume_reference_window(
            0, 4, 0, 1000, 1000, {&first_onset, 1, false}));
        CHECK(pipeline.spatial_state().voices[0].generation == 1);

        pipeline.reset(spatial_source_host_discontinuity::seek, 5000);
        CHECK(pipeline.session_epoch() == 2);
        CHECK(pipeline.buffered_frames() == 0);
        CHECK(pipeline.spatial_state().voices[0].generation == 0);

        auto seek_onset = make_record(
            0,
            spc_voice_runtime_event_kind::key_on_accepted,
            200,
            0);
        CHECK(pipeline.consume_reference_window(
            5000, 4, 200, 1000, 1000, {&seek_onset, 1, false}));
        const auto after_seek = pipeline.pull(4);
        CHECK(after_seek.reference_frame_start == 5000);
        CHECK(after_seek.session_epoch == 2);
        CHECK(after_seek.sources.lanes[0].evidence.generation == 1);
    }

    {
        // Capture continuity failure leaves the accepted reference cursor and
        // persistent voice state unchanged so a caller can explicitly reset or
        // recover rather than silently stitching over the gap.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 0);
        auto first = make_record(
            10,
            spc_voice_runtime_event_kind::key_on_accepted,
            0,
            0);
        CHECK(pipeline.consume_reference_window(
            0, 2, 0, 1000, 1000, {&first, 1, false}));
        (void)pipeline.pull(2);
        CHECK(pipeline.expected_reference_frame() == 2);
        CHECK(pipeline.spatial_state().voices[0].generation == 1);

        auto gap = make_record(
            12,
            spc_voice_runtime_event_kind::key_on_accepted,
            2,
            0);
        CHECK(!pipeline.consume_reference_window(
            2, 2, 2, 1000, 1000, {&gap, 1, false}));
        CHECK(pipeline.last_error() ==
            spc_runtime_host_pipeline_error::spatial_adapter_rejected);
        CHECK(pipeline.last_spatial_error() ==
            spc_runtime_spatial_adapter_error::continuity_lost_requires_reset);
        CHECK(pipeline.expected_reference_frame() == 2);
        CHECK(pipeline.spatial_state().voices[0].generation == 1);
    }

    {
        // Capacity rejection is also pre-adapter and therefore transactional.
        spc_runtime_host_pipeline<4, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 0);
        auto onset = make_record(
            0,
            spc_voice_runtime_event_kind::key_on_accepted,
            0,
            3);
        CHECK(!pipeline.consume_reference_window(
            0, 5, 0, 1000, 1000, {&onset, 1, false}));
        CHECK(pipeline.last_error() ==
            spc_runtime_host_pipeline_error::reference_window_too_large);
        CHECK(pipeline.spatial_state().voices[3].generation == 0);
        CHECK(pipeline.expected_reference_frame() == 0);
    }

    return 0;
}
