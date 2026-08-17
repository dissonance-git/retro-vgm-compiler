#include "components/spc/snesapu_source_transport_v2_storage.h"
#include "components/spc/snesapu_direct_spatial_projection.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

int main() {
    using namespace gameaudio::spc;
    using transport = snesapu_source_transport_v2;

    constexpr std::size_t frames = 3;
    std::array<float, frames * transport::plane_count> sample_major{};
    auto set = [&](std::size_t frame, std::size_t plane, float value) {
        sample_major[frame * transport::plane_count + plane] = value;
    };

    // Voice 0 carries real audio and changes from hard-left to hard-right at
    // frame 1. Other voices stay silent/zero-routed. Shared echo is independent.
    set(0, transport::dry_base + 0, 16384.0f);
    set(1, transport::dry_base + 0, 8192.0f);
    set(2, transport::dry_base + 0, -8192.0f);
    set(0, transport::gain_left_base + 0, 1.0f);
    set(0, transport::gain_right_base + 0, 0.0f);
    set(1, transport::gain_left_base + 0, 0.0f);
    set(1, transport::gain_right_base + 0, 1.0f);
    set(2, transport::gain_left_base + 0, 0.0f);
    set(2, transport::gain_right_base + 0, 1.0f);
    set(0, transport::echo_left_plane, 3276.8f);
    set(1, transport::echo_left_plane, 6553.6f);
    set(2, transport::echo_right_plane, -3276.8f);

    snesapu_source_transport_v2_storage<16> storage;
    assert(storage.load_sample_major(sample_major.data(), frames));
    assert(storage.valid());
    const auto view = storage.view();
    assert(view.valid());
    assert(view.frame_count() == frames);
    assert(std::abs(view.dry_voice(0)[0] - 0.5f) < 1.0e-6f);
    assert(view.gain_left(0)[0] == 1.0f);
    assert(view.gain_right(0)[1] == 1.0f);
    assert(std::abs(view.echo_left()[0] - 0.1f) < 1.0e-5f);

    snesapu_direct_spatial_projection_storage<16> projection;
    projection.reset(7u);
    assert(projection.build(view));
    assert(projection.valid());
    const auto& block = projection.block();
    assert(block.lane_count == 10u);
    assert(block.frame_count == frames);
    assert(block.evidence_event_count == 1u);
    assert(block.evidence_events[0].frame_offset == 1u);
    assert(block.evidence_events[0].lane_index == 0u);
    assert(block.evidence_events[0].evidence.stereo_route.left_gain == 0.0f);
    assert(block.evidence_events[0].evidence.stereo_route.right_gain == 1.0f);
    assert(block.evidence_events[0].evidence.stereo_route.gain_preapplied);
    assert(block.lanes[0].evidence.stereo_route.left_gain == 1.0f);
    assert(block.lanes[0].evidence.stereo_route.right_gain == 0.0f);
    assert(block.lanes[0].evidence.generation == 7u);

    // Hard-left unity route has energy gain sqrt(1/2), already applied once.
    assert(std::abs(block.lanes[0].mono_pcm[0]
        - 0.5f * std::sqrt(0.5f)) < 1.0e-6f);
    assert(block.lanes[8].kind ==
        vgmtooling::model::spatial_audio_lane_kind::shared_effect_return);
    assert(block.lanes[9].kind ==
        vgmtooling::model::spatial_audio_lane_kind::shared_effect_return);

    // Parent requests can be larger than the native 1024-frame wire packet.
    // load_planar_slice imports one compact renderer-sized window without
    // changing plane identity or treating gain controls as audio.
    constexpr std::size_t parent_frames = 5;
    std::array<float, parent_frames * transport::plane_count> parent{};
    for (std::size_t plane = 0; plane < transport::plane_count; ++plane) {
        for (std::size_t frame = 0; frame < parent_frames; ++frame)
            parent[plane * parent_frames + frame] =
                static_cast<float>(plane * 100u + frame);
    }
    snesapu_source_transport_v2_storage<16> slice;
    assert(slice.load_planar_slice(parent.data(), parent_frames, 2u, 2u));
    const auto slice_view = slice.view();
    assert(slice_view.frame_count() == 2u);
    assert(slice_view.plane(3)[0] == 302.0f);
    assert(slice_view.plane(3)[1] == 303.0f);

    // Wire receive validates the header before exposing a payload buffer.
    transport::header header{};
    header.magic = transport::magic;
    header.version = transport::version;
    header.header_size = static_cast<std::uint16_t>(sizeof(header));
    header.block_samples = 2u;
    header.plane_count = static_cast<std::uint16_t>(transport::plane_count);
    header.sample_format = transport::format_float32;
    header.audio_lanes = static_cast<std::uint16_t>(transport::audio_lane_count);
    snesapu_source_transport_v2_storage<16> wire;
    assert(wire.begin_wire_receive(header));
    assert(wire.wire_value_count() == 2u * transport::plane_count);
    for (std::size_t i = 0; i < wire.wire_value_count(); ++i)
        wire.wire_write_data()[i] = static_cast<float>(i);
    assert(wire.commit_wire_receive());
    assert(wire.valid());
    assert(wire.view().plane(1)[0] == 2.0f);

    header.reserved32 = 1u;
    assert(!wire.begin_wire_receive(header));
    assert(!wire.valid());

    return 0;
}
