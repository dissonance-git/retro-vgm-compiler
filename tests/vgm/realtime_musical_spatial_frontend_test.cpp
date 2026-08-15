#include "../../model/realtime_musical_spatial_frontend.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

vgmtooling::model::spatial_audio_lane_view make_lane(
    const float* pcm,
    std::uint64_t source_id)
{
    vgmtooling::model::spatial_audio_lane_view lane{};
    lane.mono_pcm = pcm;
    lane.evidence.source_id = source_id;
    lane.evidence.generation = 1;
    return lane;
}

} // namespace

int main()
{
    using namespace vgmtooling::model;

    constexpr double sample_rate = 48000.0;
    constexpr std::size_t frame_count = 4800;
    constexpr double pi = 3.141592653589793238462643383279502884;

    std::array<float, frame_count> low{};
    std::array<float, frame_count> high{};
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const double time = static_cast<double>(frame) / sample_rate;
        low[frame] = 0.5f * static_cast<float>(std::sin(2.0 * pi * 100.0 * time));
        high[frame] = 0.5f * static_cast<float>(std::sin(2.0 * pi * 8000.0 * time));
    }

    using frontend_type = realtime_musical_spatial_frontend<4, 8, 8>;
    frontend_type frontend{};
    frontend_type::handoff_storage handoff{};

    auto source_lane = make_lane(low.data(), 1);
    const spatial_source_block_view block{&source_lane, 1, frame_count};

    // Before any audio has completed, prepare_block has no learned future state
    // to offer. It may carry current source evidence, but not inspect current PCM.
    assert(frontend.prepare_block(block, handoff));
    assert(handoff.valid());
    assert(!handoff.lane(0).roles_available);
    assert(handoff.history_seconds() == 0.0);

    // Learning occurs only after the block completes. The following block can
    // then receive that past-only musical-role memory.
    assert(frontend.complete_block(block, sample_rate));
    assert(frontend.prepare_block(block, handoff));
    assert(handoff.lane(0).roles_available);
    assert(handoff.lane(0).roles.foundation.confidence > 0.0f);
    assert(std::fabs(handoff.history_seconds() - 0.10) < 1.0e-9);

    // No-lookahead regression: two frontends with identical completed history
    // must prepare identical role state even when their current PCM differs.
    frontend_type causal_a{};
    frontend_type causal_b{};
    assert(causal_a.complete_block(block, sample_rate));
    assert(causal_b.complete_block(block, sample_rate));

    auto current_low = make_lane(low.data(), 1);
    auto current_high = make_lane(high.data(), 1);
    const spatial_source_block_view low_block{&current_low, 1, frame_count};
    const spatial_source_block_view high_block{&current_high, 1, frame_count};
    frontend_type::handoff_storage low_handoff{};
    frontend_type::handoff_storage high_handoff{};
    assert(causal_a.prepare_block(low_block, low_handoff));
    assert(causal_b.prepare_block(high_block, high_handoff));
    assert(low_handoff.lane(0).roles_available);
    assert(high_handoff.lane(0).roles_available);
    assert(low_handoff.lane(0).roles.foundation.score
        == high_handoff.lane(0).roles.foundation.score);
    assert(low_handoff.history_seconds() == high_handoff.history_seconds());

    // Completing a multi-source block advances musical time exactly once.
    std::array<spatial_audio_lane_view, 2> lanes{
        make_lane(low.data(), 2),
        make_lane(high.data(), 3),
    };
    const spatial_source_block_view multi_lane_block{
        lanes.data(),
        lanes.size(),
        frame_count,
    };
    const double before_multi_lane = frontend.tracker().stream_seconds();
    assert(frontend.complete_block(multi_lane_block, sample_rate));
    assert(std::fabs(
        frontend.tracker().stream_seconds() - before_multi_lane - 0.10) < 1.0e-9);

    // Timed source identity changes receive a role-memory lookup at the exact
    // event boundary during prepare. Unknown identities remain unavailable.
    spatial_source_evidence replacement = lanes[0].evidence;
    replacement.source_id = 999;
    const spatial_source_evidence_event identity_event{100, 0, replacement};
    const spatial_source_block_view event_block{
        lanes.data(),
        lanes.size(),
        frame_count,
        &identity_event,
        1,
    };
    assert(frontend.prepare_block(event_block, handoff));
    assert(handoff.event_count() == 1);
    assert(handoff.event(0).frame_offset == 100);
    assert(!handoff.event(0).roles_available);

    // The block-level acoustic observer refuses to merge two identities into one
    // summary. Completion therefore loses learning for this ambiguous block, but
    // stream time still advances because the audio itself already passed.
    const double before_failed_completion = frontend.tracker().stream_seconds();
    assert(!frontend.complete_block(event_block, sample_rate));
    assert(std::fabs(
        frontend.tracker().stream_seconds() - before_failed_completion - 0.10) < 1.0e-9);

    return 0;
}
