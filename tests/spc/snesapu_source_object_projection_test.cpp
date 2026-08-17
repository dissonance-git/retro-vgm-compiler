#include "components/spc/snesapu_source_object_projection.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::spc;
    using namespace vgmtooling::model;

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
        for (std::size_t frame = 0; frame < frames; ++frame)
            plane(snesapu_source_transport_v2::dry_base + voice)[frame] = 1.0f;
    }

    // Hard-left voice: exact source energy becomes sqrt(1/2), while native
    // route remains pose evidence and is explicitly marked preapplied.
    for (std::size_t frame = 0; frame < frames; ++frame) {
        plane(snesapu_source_transport_v2::gain_left_base + 0)[frame] = 1.0f;
        plane(snesapu_source_transport_v2::gain_right_base + 0)[frame] = 0.0f;
    }

    // Center voice with a negative signed route on both sides. Energy remains
    // positive in mono PCM; polarity survives only in route evidence.
    for (std::size_t frame = 0; frame < frames; ++frame) {
        plane(snesapu_source_transport_v2::gain_left_base + 1)[frame] = -1.0f;
        plane(snesapu_source_transport_v2::gain_right_base + 1)[frame] = -1.0f;
    }

    // One voice exercises a sample-exact route trajectory and a timed event.
    plane(snesapu_source_transport_v2::gain_left_base + 2)[0] = 1.0f;
    plane(snesapu_source_transport_v2::gain_left_base + 2)[1] = 0.75f;
    plane(snesapu_source_transport_v2::gain_left_base + 2)[2] = 0.50f;
    plane(snesapu_source_transport_v2::gain_left_base + 2)[3] = 0.25f;
    plane(snesapu_source_transport_v2::gain_right_base + 2)[0] = 0.0f;
    plane(snesapu_source_transport_v2::gain_right_base + 2)[1] = 0.25f;
    plane(snesapu_source_transport_v2::gain_right_base + 2)[2] = 0.50f;
    plane(snesapu_source_transport_v2::gain_right_base + 2)[3] = 0.75f;

    // Remaining voices are full center to keep all producer planes finite.
    for (std::size_t voice = 3; voice < 8; ++voice) {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            plane(snesapu_source_transport_v2::gain_left_base + voice)[frame] = 1.0f;
            plane(snesapu_source_transport_v2::gain_right_base + voice)[frame] = 1.0f;
        }
    }

    plane(snesapu_source_transport_v2::echo_left_plane)[0] = 0.10f;
    plane(snesapu_source_transport_v2::echo_left_plane)[1] = 0.20f;
    plane(snesapu_source_transport_v2::echo_left_plane)[2] = 0.30f;
    plane(snesapu_source_transport_v2::echo_left_plane)[3] = 0.40f;
    plane(snesapu_source_transport_v2::echo_right_plane)[0] = -0.10f;
    plane(snesapu_source_transport_v2::echo_right_plane)[1] = -0.20f;
    plane(snesapu_source_transport_v2::echo_right_plane)[2] = -0.30f;
    plane(snesapu_source_transport_v2::echo_right_plane)[3] = -0.40f;

    const snesapu_source_transport_v2::view source{&header, planes.data()};
    assert(source.valid());

    std::array<spatial_audio_lane_view, 8> evidence_lanes{};
    for (std::size_t voice = 0; voice < evidence_lanes.size(); ++voice) {
        evidence_lanes[voice].kind = spatial_audio_lane_kind::dry_source;
        evidence_lanes[voice].evidence.family = spatial_source_family::spc;
        evidence_lanes[voice].evidence.source_id = voice + 1;
        evidence_lanes[voice].evidence.generation = 7;
        evidence_lanes[voice].evidence.physical_slot_present = true;
        evidence_lanes[voice].evidence.physical_slot = static_cast<std::uint32_t>(voice);
    }

    spatial_source_evidence_event event{};
    event.frame_offset = 2;
    event.lane_index = 2;
    event.evidence = evidence_lanes[2].evidence;
    event.evidence.stereo_route.present = true;
    event.evidence.stereo_route.left_gain = 0.25f; // deliberately stale
    event.evidence.stereo_route.right_gain = 0.75f;
    event.evidence.stereo_route.authority = spatial_evidence_authority::device_authored;

    spatial_source_block_view evidence_segment{};
    evidence_segment.lanes = evidence_lanes.data();
    evidence_segment.lane_count = evidence_lanes.size();
    evidence_segment.frame_count = frames;
    evidence_segment.evidence_events = &event;
    evidence_segment.evidence_event_count = 1;

    snesapu_source_object_projection_storage<frames, 4> projection;
    assert(projection.build(source, 0, evidence_segment, 3));
    assert(projection.valid());

    const auto& block = projection.block();
    assert(block.lane_count == 10);
    assert(block.frame_count == frames);
    for (std::size_t voice = 0; voice < 8; ++voice)
        assert(block.lanes[voice].kind == spatial_audio_lane_kind::dry_source);
    assert(block.lanes[8].kind == spatial_audio_lane_kind::shared_effect_return);
    assert(block.lanes[9].kind == spatial_audio_lane_kind::shared_effect_return);

    const float sqrt_half = static_cast<float>(std::sqrt(0.5));
    for (std::size_t frame = 0; frame < frames; ++frame)
        assert(std::abs(block.lanes[0].mono_pcm[frame] - sqrt_half) < 1.0e-6f);
    assert(std::abs(block.lanes[1].mono_pcm[0] - 1.0f) < 1.0e-6f);
    assert(block.lanes[1].evidence.stereo_route.left_gain < 0.0f);
    assert(block.lanes[1].evidence.stereo_route.right_gain < 0.0f);
    assert(block.lanes[0].evidence.stereo_route.gain_preapplied);
    assert(block.lanes[1].evidence.stereo_route.gain_preapplied);

    // The event is corrected from the stale snapshot to the exact coefficient
    // present on the producer trajectory at frame 2.
    assert(block.evidence_event_count == 1);
    assert(block.evidence_events[0].frame_offset == 2);
    assert(block.evidence_events[0].lane_index == 2);
    assert(block.evidence_events[0].evidence.stereo_route.gain_preapplied);
    assert(block.evidence_events[0].evidence.stereo_route.left_gain == 0.50f);
    assert(block.evidence_events[0].evidence.stereo_route.right_gain == 0.50f);

    // Shared wet is already post-EVOL. No extra arithmetic is applied here.
    assert(block.lanes[8].mono_pcm[3] == 0.40f);
    assert(block.lanes[9].mono_pcm[3] == -0.40f);
    assert(block.lanes[8].evidence.stereo_route.gain_preapplied);
    assert(block.lanes[9].evidence.stereo_route.gain_preapplied);
    assert(block.lanes[8].evidence.stereo_route.left_gain == 1.0f);
    assert(block.lanes[8].evidence.stereo_route.right_gain == 0.0f);
    assert(block.lanes[9].evidence.stereo_route.left_gain == 0.0f);
    assert(block.lanes[9].evidence.stereo_route.right_gain == 1.0f);

    // A missing/nonfinite producer fact is a transport failure, not silence.
    plane(snesapu_source_transport_v2::gain_left_base + 4)[1] = NAN;
    assert(!projection.build(source, 0, evidence_segment, 3));
    assert(projection.last_error() == snesapu_source_object_projection_error::nonfinite_source);

    return 0;
}
