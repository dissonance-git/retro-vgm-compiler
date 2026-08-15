#include "../../model/omniphony_realtime_client.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

std::uint32_t fake_major() { return 0; }
std::uint32_t fake_minor() { return 3; }
std::uint32_t old_minor() { return 2; }

std::int32_t fake_process(
    void* processor,
    const float* input,
    const vgmtooling::model::omniphony_source_evidence_v1_transport* sources,
    std::size_t source_count,
    const vgmtooling::model::omniphony_source_evidence_event_v1_transport* events,
    std::size_t event_count,
    std::size_t frames,
    std::uint64_t sample_pos,
    std::uint32_t ramp_frames,
    float* output)
{
    using namespace vgmtooling::model;
    assert(processor != nullptr);
    assert(source_count == 2);
    assert(frames == 3);
    assert(event_count == 1);
    assert(sample_pos == 1234);
    assert(ramp_frames == 96);
    assert(sources[0].foundation > 0.0f);
    assert(sources[1].lane_kind == omniphony_source_lane_shared_wet);
    assert(events[0].frame_offset == 1);
    assert(events[0].lane_index == 0);
    assert(input[0] == 0.1f);
    assert(input[1] == 1.0f);
    assert(input[4] == 0.3f);
    assert(input[5] == 3.0f);

    for (std::size_t frame = 0; frame < frames; ++frame) {
        output[frame * 2] = input[frame * source_count];
        output[frame * 2 + 1] = input[frame * source_count + 1];
    }
    return 0;
}

} // namespace

int main()
{
    using namespace vgmtooling::model;

    int opaque_processor = 1;
    omniphony_realtime_client<4, 4> client{};
    assert(!client.bind(
        static_cast<void*>(&opaque_processor),
        fake_major,
        old_minor,
        fake_process));
    assert(!client.bound());
    assert(client.bind(
        static_cast<void*>(&opaque_processor),
        fake_major,
        fake_minor,
        fake_process));
    assert(client.bound());

    constexpr std::size_t frames = 3;
    const std::array<float, frames> lead{0.1f, 0.2f, 0.3f};
    const std::array<float, frames> ambience{1.0f, 2.0f, 3.0f};

    std::array<spatial_audio_lane_view, 2> lanes{};
    lanes[0].mono_pcm = lead.data();
    lanes[0].evidence.source_id = 10;
    lanes[0].evidence.generation = 1;
    lanes[0].evidence.presentation.foundation = 0.7f;
    lanes[0].evidence.presentation.confidence = 0.5f;

    lanes[1].kind = spatial_audio_lane_kind::shared_effect_return;
    lanes[1].mono_pcm = ambience.data();
    lanes[1].evidence.source_id = 20;
    lanes[1].evidence.presentation.diffuse = 1.0f;
    lanes[1].evidence.presentation.confidence = 0.9f;

    spatial_source_evidence updated = lanes[0].evidence;
    updated.presentation.foreground = 0.6f;
    const spatial_source_evidence_event event{1, 0, updated};
    const spatial_source_block_view block{
        lanes.data(),
        lanes.size(),
        frames,
        &event,
        1,
    };

    std::array<float, frames * 2> source_scratch{};
    std::array<float, frames * 2> stereo{};
    const auto result = client.process(
        block,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        1234,
        96);
    assert(result.transport_valid);
    assert(result.renderer_status == 0);
    assert(stereo[0] == 0.1f);
    assert(stereo[1] == 1.0f);
    assert(stereo[4] == 0.3f);
    assert(stereo[5] == 3.0f);

    client.unbind();
    assert(!client.bound());
    return 0;
}
