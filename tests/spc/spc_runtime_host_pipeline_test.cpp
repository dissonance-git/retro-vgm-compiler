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

    {
        // At native 32 kHz, the exact pre-pan source capture and the runtime
        // route/identity evidence share one frame-for-frame protected window.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 100);

        spc_native_source_capture native;
        native.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        const std::int16_t voice0[4] = {-32768, 16384, 8192, 0};
        for (std::uint64_t frame = 0; frame < 4; ++frame) {
            voices[0] = voice0[frame];
            native.observe(
                spc_native_sample_rate,
                900 + frame,
                voices,
                spc_native_voice_count);
        }
        CHECK(native.valid());

        spc_runtime_capture_record runtime[2];
        runtime[0] = make_record(
            0,
            spc_voice_runtime_event_kind::routing_state_changed,
            0,
            0);
        runtime[0].tick_rate = 32000;
        runtime[0].fields = runtime[0].fields |
            spc_runtime_capture_field::route_gain_left |
            spc_runtime_capture_field::route_gain_right;
        runtime[0].route_gain_left = -128;
        runtime[0].route_gain_right = 64;

        runtime[1] = make_record(
            1,
            spc_voice_runtime_event_kind::key_on_accepted,
            1,
            0);
        runtime[1].tick_rate = 32000;

        CHECK(pipeline.consume_native_reference_window(
            100,
            4,
            0,
            32000,
            32000,
            {runtime, 2, false},
            native,
            900));
        CHECK(pipeline.expected_reference_frame() == 104);
        CHECK(pipeline.buffered_frames() == 4);

        const auto first = pipeline.pull(4);
        CHECK(first.reference_frame_start == 100);
        CHECK(first.sources.frame_count == 1);
        CHECK(pipeline.last_pull_identity_limited());
        CHECK(first.sources.lanes[0].evidence.generation == 0);
        CHECK(first.sources.lanes[0].evidence.stereo_route.left_gain == -1.0f);
        CHECK(first.sources.lanes[0].evidence.stereo_route.right_gain == 0.5f);
        CHECK(first.sources.lanes[0].availability[0] == 1u);
        CHECK(first.sources.lanes[0].mono_pcm[0] == -1.0f);

        const auto second = pipeline.pull(4);
        CHECK(second.reference_frame_start == 101);
        CHECK(second.sources.frame_count == 3);
        CHECK(second.sources.lanes[0].evidence.generation == 1);
        CHECK(second.sources.lanes[0].availability[0] == 1u);
        CHECK(second.sources.lanes[0].availability[2] == 1u);
        CHECK(second.sources.lanes[0].mono_pcm[0] == 0.5f);
        CHECK(second.sources.lanes[0].mono_pcm[1] == 0.25f);
        CHECK(second.sources.lanes[0].mono_pcm[2] == 0.0f);
    }

    {
        // Native PCM rejection precedes runtime-state mutation. Resampled host
        // rates stay on the evidence-only path until FIR phase is observable.
        spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
        pipeline.reset(spatial_source_host_discontinuity::initialize, 0);
        spc_native_source_capture native;
        native.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        native.observe(spc_native_sample_rate, 0, voices, spc_native_voice_count);

        auto onset = make_record(
            0,
            spc_voice_runtime_event_kind::key_on_accepted,
            0,
            1);
        onset.tick_rate = 48000;
        CHECK(!pipeline.consume_native_reference_window(
            0,
            1,
            0,
            48000,
            48000,
            {&onset, 1, false},
            native,
            0));
        CHECK(pipeline.last_error() ==
            spc_runtime_host_pipeline_error::native_source_rejected);
        CHECK(pipeline.last_native_error() ==
            spc_native_exact_source_error::unsupported_output_rate);
        CHECK(pipeline.spatial_state().voices[1].generation == 0);
        CHECK(pipeline.expected_reference_frame() == 0);

        const std::uint64_t wrapped_looking_rate = (1ull << 32u) + 32000u;
        onset.tick_rate = wrapped_looking_rate;
        CHECK(!pipeline.consume_native_reference_window(
            0,
            1,
            0,
            wrapped_looking_rate,
            wrapped_looking_rate,
            {&onset, 1, false},
            native,
            0));
        CHECK(pipeline.last_native_error() ==
            spc_native_exact_source_error::unsupported_output_rate);
        CHECK(pipeline.spatial_state().voices[1].generation == 0);
    }

    return 0;
}
