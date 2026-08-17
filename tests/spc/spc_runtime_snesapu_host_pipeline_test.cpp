#include "components/spc/spc_runtime_host_pipeline.h"

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

    // SRCE v2 is already at the protected host render rate, so unlike the exact
    // native libgme hook it is valid at 96 kHz without pretending a 32 kHz lane
    // is magically full-band. The producer did the source-rate realization.
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
