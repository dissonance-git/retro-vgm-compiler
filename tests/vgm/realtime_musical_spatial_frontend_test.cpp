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
    // to offer. The renderer-ready view therefore preserves current raw evidence
    // and the same PCM pointer without manufacturing presentation confidence.
    assert(frontend.prepare_block(block, handoff));
    assert(handoff.valid());
    assert(!handoff.lane(0).roles_available);
    assert(handoff.history_seconds() == 0.0);
    assert(handoff.projected_view().lane_count == 1);
    assert(handoff.projected_view().lanes[0].mono_pcm == low.data());
    assert(handoff.projected_view().lanes[0].evidence.presentation.confidence == 0.0f);

    // Learning occurs only after the block completes. The following block can
    // then receive that past-only musical-role memory and project it into the
    // presentation vocabulary consumed by the realtime spatial DSP.
    assert(frontend.complete_block(block, sample_rate));
    assert(source_lane.evidence.presentation.confidence == 0.0f); // raw evidence stayed raw
    assert(frontend.prepare_block(block, handoff));
    assert(handoff.lane(0).roles_available);
    assert(handoff.lane(0).roles.foundation.confidence > 0.0f);
    assert(handoff.projected_view().lanes[0].evidence.presentation.foundation > 0.0f);
    assert(handoff.projected_view().lanes[0].evidence.presentation.confidence > 0.0f);
    assert(handoff.projected_view().lanes[0].evidence.presentation.vertical_affinity == 0.0f);
    assert(!handoff.projected_view().lanes[0].evidence.authored_position_present);
    assert(std::fabs(handoff.history_seconds() - 0.10) < 1.0e-9);

    // No-lookahead regression: two frontends with identical completed history
    // must prepare identical role and projected presentation state even when
    // their current PCM differs.
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
    assert(low_handoff.projected_view().lanes[0].evidence.presentation.foundation
        == high_handoff.projected_view().lanes[0].evidence.presentation.foundation);
    assert(low_handoff.projected_view().lanes[0].evidence.presentation.confidence
        == high_handoff.projected_view().lanes[0].evidence.presentation.confidence);
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
    // event boundary during prepare. Unknown identities remain unavailable and
    // their projected event keeps the exact frame boundary without inheriting
    // presentation memory from the physical lane's previous occupant.
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
    assert(handoff.projected_view().evidence_event_count == 1);
    assert(handoff.projected_view().evidence_events[0].frame_offset == 100);
    assert(handoff.projected_view().evidence_events[0].lane_index == 0);
    assert(handoff.projected_view().evidence_events[0].evidence.source_id == 999);
    assert(handoff.projected_view().evidence_events[0].evidence.presentation.confidence == 0.0f);

    // The block-level acoustic observer refuses to merge two identities into one
    // summary. Completion therefore loses learning for this ambiguous block, but
    // stream time still advances because the audio itself already passed.
    const double before_failed_completion = frontend.tracker().stream_seconds();
    assert(!frontend.complete_block(event_block, sample_rate));
    assert(std::fabs(
        frontend.tracker().stream_seconds() - before_failed_completion - 0.10) < 1.0e-9);

    // Omniphony's terminal-event rule is stricter: an event at exactly `frames`
    // is a zero-length next-block state transition. No current audio belongs to
    // replacement source 777, so completion must succeed for the old source and
    // must not train source 777 from the old source's PCM.
    frontend_type terminal_frontend{};
    auto terminal_lane = make_lane(low.data(), 70);
    spatial_source_evidence terminal_replacement = terminal_lane.evidence;
    terminal_replacement.source_id = 777;
    terminal_replacement.generation = 2;
    const spatial_source_evidence_event terminal_event{
        frame_count,
        0,
        terminal_replacement,
    };
    const spatial_source_block_view terminal_block{
        &terminal_lane,
        1,
        frame_count,
        &terminal_event,
        1,
    };
    assert(terminal_frontend.prepare_block(terminal_block, handoff));
    assert(handoff.event_count() == 1);
    assert(handoff.event(0).frame_offset == frame_count);
    assert(terminal_frontend.complete_block(terminal_block, sample_rate));

    auto next_lane = make_lane(low.data(), 777);
    next_lane.evidence.generation = 2;
    const spatial_source_block_view next_block{&next_lane, 1, frame_count};
    assert(terminal_frontend.prepare_block(next_block, handoff));
    assert(!handoff.lane(0).roles_available);

    // The actually heard source did earn memory from the completed block.
    assert(terminal_frontend.prepare_block(terminal_block, handoff));
    assert(handoff.lane(0).roles_available);

    return 0;
}
