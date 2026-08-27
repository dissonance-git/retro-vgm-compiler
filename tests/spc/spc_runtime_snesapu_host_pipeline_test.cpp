#include "components/spc/spc_realtime_musical_omniphony_pipeline.h"
#include "components/spc/spc_runtime_host_pipeline.h"
#include "model/spatial_playback_options.h"

#include <array>
#include <cmath>
#include <cstddef>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    constexpr std::size_t frames = 4;

    snesapu_source_transport_v2::header header{};
    header.magic = snesapu_source_transport_v2::magic;
    header.version = snesapu_source_transport_v2::version;
    header.header_size = sizeof(header);
    header.block_samples = frames;
    header.plane_count = snesapu_source_transport_v2::plane_count;
    header.sample_format = snesapu_source_transport_v2::format_float32;
    header.audio_lanes = snesapu_source_transport_v2::audio_lane_count;

    std::array<float, snesapu_source_transport_v2::plane_count * frames> planes{};
    auto plane = [&](std::size_t index) -> float* {
        return planes.data() + index * frames;
    };

    for (std::size_t voice = 0; voice < 8; ++voice) {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            plane(snesapu_source_transport_v2::dry_base + voice)[frame] =
                voice == 0 ? static_cast<float>(frame + 1) : 0.0f;
            plane(snesapu_source_transport_v2::gain_left_base + voice)[frame] = 1.0f;
            plane(snesapu_source_transport_v2::gain_right_base + voice)[frame] =
                voice == 0 ? 0.0f : 1.0f;
        }
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        plane(snesapu_source_transport_v2::echo_left_plane)[frame] =
            0.1f * static_cast<float>(frame + 1);
        plane(snesapu_source_transport_v2::echo_right_plane)[frame] =
            -0.1f * static_cast<float>(frame + 1);
    }

    const snesapu_source_transport_v2::view source{&header, planes.data()};
    CHECK(source.valid());

    // A second producer block stands in for a source realization selected by
    // the independent quality layer. The host/spatial pipeline must not care why
    // this exact source waveform was selected.
    auto higher_quality_planes = planes;
    for (std::size_t frame = 0; frame < frames; ++frame)
        higher_quality_planes[snesapu_source_transport_v2::dry_base * frames + frame] *= 2.0f;
    const snesapu_source_transport_v2::view higher_quality_source{
        &header,
        higher_quality_planes.data(),
    };
    CHECK(higher_quality_source.valid());

    // The transport itself remains source-realization agnostic. An explicitly
    // supplied alternate producer can still traverse this seam even though the
    // current product selector never chooses Enhanced.
    spc_runtime_host_pipeline<8, 4, 8, 32> alternate_source_pipeline;
    alternate_source_pipeline.reset(spatial_source_host_discontinuity::initialize, 100);
    CHECK(alternate_source_pipeline.consume_snesapu_reference_window(
        100,
        frames,
        0,
        48000,
        48000,
        {nullptr, 0, false},
        higher_quality_source));
    const auto alternate_chunk = alternate_source_pipeline.pull(frames);
    const float alternate_sqrt_half = static_cast<float>(std::sqrt(0.5));
    CHECK(std::abs(
        alternate_chunk.sources.lanes[0].mono_pcm[0]
        - 2.0f * alternate_sqrt_half) < 1.0e-6f);

    // SRCE v2 is already at the protected host render rate, so unlike the exact
    // native libgme hook it remains valid if an offline/research producer uses a
    // higher host rate. Normal component playback is standardized at 48 kHz.
    spc_runtime_host_pipeline<8, 4, 8, 32> pipeline;
    pipeline.reset(spatial_source_host_discontinuity::initialize, 200);
    CHECK(pipeline.consume_snesapu_reference_window(
        200,
        frames,
        0,
        96000,
        96000,
        {nullptr, 0, false},
        source));
    CHECK(pipeline.expected_reference_frame() == 204);
    CHECK(pipeline.buffered_frames() == frames);

    const float sqrt_half = static_cast<float>(std::sqrt(0.5));

    // Downstream host chunk size is unrelated to the producer block size.
    const auto first = pipeline.pull(2);
    CHECK(first.reference_frame_start == 200);
    CHECK(first.session_epoch == 1);
    CHECK(first.sources.lane_count == 10);
    CHECK(first.sources.frame_count == 2);
    CHECK(first.sources.lanes[0].kind == spatial_audio_lane_kind::dry_source);
    CHECK(first.sources.lanes[8].kind == spatial_audio_lane_kind::shared_effect_return);
    CHECK(first.sources.lanes[9].kind == spatial_audio_lane_kind::shared_effect_return);
    CHECK(first.sources.lanes[0].availability[0] == 1u);
    CHECK(first.sources.lanes[8].availability[1] == 1u);
    CHECK(std::abs(first.sources.lanes[0].mono_pcm[0] - sqrt_half) < 1.0e-6f);
    CHECK(std::abs(first.sources.lanes[0].mono_pcm[1] - 2.0f * sqrt_half) < 1.0e-6f);
    CHECK(first.sources.lanes[0].evidence.stereo_route.left_gain == 1.0f);
    CHECK(first.sources.lanes[0].evidence.stereo_route.right_gain == 0.0f);
    CHECK(first.sources.lanes[0].evidence.stereo_route.gain_preapplied);
    CHECK(first.sources.lanes[8].mono_pcm[0] == 0.1f);
    CHECK(first.sources.lanes[9].mono_pcm[0] == -0.1f);
    CHECK(first.sources.lanes[8].evidence.stereo_route.gain_preapplied);
    CHECK(first.sources.lanes[9].evidence.stereo_route.gain_preapplied);

    const auto second = pipeline.pull(2);
    CHECK(second.reference_frame_start == 202);
    CHECK(second.sources.lane_count == 10);
    CHECK(second.sources.frame_count == 2);
    CHECK(std::abs(second.sources.lanes[0].mono_pcm[0] - 3.0f * sqrt_half) < 1.0e-6f);
    CHECK(std::abs(second.sources.lanes[0].mono_pcm[1] - 4.0f * sqrt_half) < 1.0e-6f);
    CHECK(std::abs(second.sources.lanes[8].mono_pcm[0] - 0.3f) < 1.0e-6f);
    CHECK(std::abs(second.sources.lanes[9].mono_pcm[1] + 0.4f) < 1.0e-6f);
    CHECK(pipeline.buffered_frames() == 0);

    // Exercise current product playback with both possible stale persisted
    // Enhanced bits and both Surround states. Enhanced is hard-disabled, so the
    // protected reference source remains selected in every product combination.
    for (int stale_enhanced = 0; stale_enhanced != 2; ++stale_enhanced) {
        for (int surround = 0; surround != 2; ++surround) {
            spatial_playback_options options;
            options.enhanced = stale_enhanced != 0;
            options.surround = surround != 0;
            CHECK(!uses_enhanced_renderer(options));

            const auto selected_source = uses_enhanced_renderer(options)
                ? higher_quality_source
                : source;
            spc_runtime_host_pipeline<8, 4, 8, 32> combination;
            combination.reset(spatial_source_host_discontinuity::initialize, 1000);
            CHECK(combination.consume_snesapu_reference_window(
                1000,
                frames,
                0,
                48000,
                48000,
                {nullptr, 0, false},
                selected_source));

            const auto chunk = combination.pull(frames);
            CHECK(chunk.sources.frame_count == frames);
            CHECK(chunk.sources.lane_count == 10);
            CHECK(std::abs(
                chunk.sources.lanes[0].mono_pcm[0] - sqrt_half) < 1.0e-6f);
            CHECK(chunk.sources.lanes[8].mono_pcm[0] == 0.1f);
            CHECK(chunk.sources.lanes[9].mono_pcm[0] == -0.1f);

            if (surround != 0) {
                CHECK(resolve_spatial_playback(options) ==
                    spatial_playback_path::source_spatial);
                CHECK(uses_source_renderer(options));

                // The SPC Omniphony wrapper consumes exactly the chunk selected
                // above. With no renderer bound it validates the source handoff
                // but cannot render, proving the source and presentation seams
                // are separate executable stages.
                spc_realtime_musical_omniphony_pipeline<10, 8, 32> omniphony;
                std::array<float, 10 * frames> source_scratch{};
                std::array<float, frames * 2> spatial_stereo{};
                const auto spatial_result = omniphony.process_chunk(
                    chunk,
                    48000.0,
                    source_scratch.data(),
                    source_scratch.size(),
                    spatial_stereo.data(),
                    spatial_stereo.size(),
                    96);
                CHECK(spatial_result.source_chunk_valid);
                CHECK(!spatial_result.omniphony.prepared);
                CHECK(!spatial_result.omniphony.rendered);
            } else {
                CHECK(resolve_spatial_playback(options) ==
                    spatial_playback_path::reference_stereo);
                CHECK(!uses_source_renderer(options));
            }
        }
    }

    // Producer/header mismatch is rejected before runtime evidence can mutate.
    auto bad_header = header;
    bad_header.block_samples = 3;
    const snesapu_source_transport_v2::view mismatched{&bad_header, planes.data()};
    auto onset = spc_runtime_capture_record{};
    onset.trace_index = 0;
    onset.kind = spc_voice_runtime_event_kind::key_on_accepted;
    onset.tick = 4;
    onset.tick_rate = 96000;
    onset.voice = 3;
    onset.fields = to_fields(spc_runtime_capture_field::voice);

    CHECK(!pipeline.consume_snesapu_reference_window(
        204,
        4,
        4,
        96000,
        96000,
        {&onset, 1, false},
        mismatched));
    CHECK(pipeline.last_error() == spc_runtime_host_pipeline_error::snesapu_source_rejected);
    CHECK(pipeline.spatial_state().voices[3].generation == 0);
    CHECK(pipeline.expected_reference_frame() == 204);

    return 0;
}
