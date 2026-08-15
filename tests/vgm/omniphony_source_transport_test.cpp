#include "../../model/omniphony_source_transport.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

int main()
{
    using namespace vgmtooling::model;

    static_assert(omniphony_source_abi_major_required == 0);
    static_assert(omniphony_source_abi_minor_required >= 3);

    // Omniphony source_ffi ABI 0.3 uses the same C layout on its Rust side.
    // Pin the binary shape so a future field insertion/reorder cannot silently
    // reinterpret musical evidence across the DLL boundary.
    static_assert(sizeof(omniphony_source_evidence_v1_transport) == 72);
    static_assert(alignof(omniphony_source_evidence_v1_transport) == 8);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, lane_kind) == 0);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, flags) == 4);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, source_id) == 8);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, persistent_part_id) == 16);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, left_gain) == 24);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, authored_x) == 32);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, foundation) == 44);
    static_assert(offsetof(omniphony_source_evidence_v1_transport, confidence) == 64);
    static_assert(sizeof(omniphony_source_evidence_event_v1_transport) == 80);
    static_assert(offsetof(omniphony_source_evidence_event_v1_transport, evidence) == 8);

    constexpr std::size_t frames = 4;
    const std::array<float, frames> a{0.1f, 0.2f, 0.3f, 0.4f};
    const std::array<float, frames> b{-0.1f, -0.2f, -0.3f, -0.4f};

    std::array<spatial_audio_lane_view, 2> lanes{};
    lanes[0].mono_pcm = a.data();
    lanes[0].evidence.source_id = 100;
    lanes[0].evidence.generation = 1;
    lanes[0].evidence.stereo_route.present = true;
    lanes[0].evidence.stereo_route.left_gain = 1.0f;
    lanes[0].evidence.stereo_route.right_gain = 0.25f;
    lanes[0].evidence.persistent_part_present = true;
    lanes[0].evidence.persistent_part_id = 900;
    lanes[0].evidence.persistent_part_confidence = 0.90f;
    lanes[0].evidence.presentation.foundation = 0.8f;
    lanes[0].evidence.presentation.confidence = 0.7f;

    lanes[1].mono_pcm = b.data();
    lanes[1].kind = spatial_audio_lane_kind::shared_effect_return;
    lanes[1].evidence.source_id = 200;
    lanes[1].evidence.presentation.diffuse = 1.0f;
    lanes[1].evidence.presentation.width = 0.65f;
    lanes[1].evidence.presentation.confidence = 0.95f;

    spatial_source_evidence changed = lanes[0].evidence;
    changed.generation = 2;
    changed.presentation.foreground = 0.6f;
    const spatial_source_evidence_event event{2, 0, changed};
    const spatial_source_block_view block{
        lanes.data(),
        lanes.size(),
        frames,
        &event,
        1,
    };

    omniphony_source_transport_storage<4, 4> transport{};
    assert(transport.build(block));
    assert(transport.valid());
    assert(transport.lane_count() == 2);
    assert(transport.event_count() == 1);
    assert(transport.frame_count() == frames);

    const auto& first = transport.lanes()[0];
    assert(first.lane_kind == omniphony_source_lane_dry);
    assert((first.flags & omniphony_source_flag_native_stereo_route) != 0);
    assert((first.flags & omniphony_source_flag_persistent_part) != 0);
    assert(first.persistent_part_id == 900);
    assert(first.left_gain == 1.0f);
    assert(first.right_gain == 0.25f);
    assert(first.foundation == 0.8f);
    assert(first.confidence == 0.7f);

    const auto& wet = transport.lanes()[1];
    assert(wet.lane_kind == omniphony_source_lane_shared_wet);
    assert(wet.diffuse == 1.0f);
    assert(wet.width == 0.65f);

    // Generation is part of the renderer-local episode token even though the
    // ABI 0.3 record has only one u64 source identity coordinate.
    assert(transport.events()[0].frame_offset == 2);
    assert(transport.events()[0].lane_index == 0);
    assert(transport.events()[0].evidence.source_id != first.source_id);

    std::array<float, frames * 2> interleaved{};
    assert(transport.interleave_pcm(block, interleaved.data(), interleaved.size()));
    assert(interleaved[0] == a[0]);
    assert(interleaved[1] == b[0]);
    assert(interleaved[2] == a[1]);
    assert(interleaved[3] == b[1]);
    assert(interleaved[6] == a[3]);
    assert(interleaved[7] == b[3]);

    // Reference mixes remain protected controls, not object lanes.
    auto invalid_lanes = lanes;
    invalid_lanes[0].kind = spatial_audio_lane_kind::reference_mix;
    const spatial_source_block_view invalid_block{
        invalid_lanes.data(),
        invalid_lanes.size(),
        frames,
    };
    assert(!transport.build(invalid_block));

    // Missing PCM evidence is not silently rendered as zero.
    std::array<std::uint8_t, frames> availability{1, 1, 0, 1};
    lanes[0].availability = availability.data();
    const spatial_source_block_view unavailable_block{
        lanes.data(),
        lanes.size(),
        frames,
        &event,
        1,
    };
    assert(transport.build(unavailable_block));
    assert(!transport.interleave_pcm(
        unavailable_block,
        interleaved.data(),
        interleaved.size()));

    return 0;
}
