#include "../../model/realtime_spatial_scene_dsp.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

using dsp_type = vgmtooling::model::realtime_spatial_scene_dsp<4, 8>;
using storage_type = vgmtooling::model::realtime_spatial_scene_block_storage<4, 8>;

vgmtooling::model::spatial_source_evidence make_source(
    std::uint64_t source_id,
    std::uint64_t generation,
    float foreground,
    float width,
    float diffuse = 0.0f,
    float vertical = 0.0f,
    float confidence = 1.0f)
{
    vgmtooling::model::spatial_source_evidence source{};
    source.source_id = source_id;
    source.generation = generation;
    source.presentation.foreground = foreground;
    source.presentation.width = width;
    source.presentation.diffuse = diffuse;
    source.presentation.vertical_affinity = vertical;
    source.presentation.confidence = confidence;
    source.presentation.authority = vgmtooling::model::spatial_evidence_authority::inferred;
    return source;
}

vgmtooling::model::spatial_audio_lane_view make_lane(
    const vgmtooling::model::spatial_source_evidence& source)
{
    vgmtooling::model::spatial_audio_lane_view lane{};
    lane.evidence = source;
    return lane;
}

bool near(float left, float right, float tolerance = 1.0e-6f)
{
    return std::fabs(left - right) <= tolerance;
}

void assert_same_prefix(
    const vgmtooling::model::realtime_spatial_control_span& left,
    const vgmtooling::model::realtime_spatial_control_span& right)
{
    assert(left.lane_index == right.lane_index);
    assert(left.frame_offset == right.frame_offset);
    assert(left.frame_count == right.frame_count);
    assert(left.evidence.source_id == right.evidence.source_id);
    assert(left.evidence.generation == right.evidence.generation);
    assert(near(left.start.foreground, right.start.foreground));
    assert(near(left.target.foreground, right.target.foreground));
    assert(near(left.start.width, right.start.width));
    assert(near(left.target.width, right.target.width));
}

} // namespace

int main()
{
    constexpr double sample_rate = 48000.0;
    constexpr std::size_t block_frames = 480;

    // A newly observed source begins from its current evidence. There is no
    // synthetic fade from an invented neutral pose and no whole-song prepass.
    dsp_type dsp{};
    storage_type output{};
    auto source = make_source(101, 3, 1.0f, 0.30f, 0.10f, -0.20f, 0.90f);
    auto lane = make_lane(source);
    vgmtooling::model::spatial_source_block_view first_block{&lane, 1, block_frames};
    assert(dsp.process(first_block, sample_rate, output));
    assert(output.valid());
    assert(output.frame_count() == block_frames);
    assert(output.span_count() == 1);
    assert(near(output.span(0).start.foreground, 1.0f));
    assert(near(output.span(0).target.foreground, 1.0f));
    assert(near(output.span(0).start.width, 0.30f));
    assert(!output.span(0).evidence.authored_position_present);
    assert(output.span(0).evidence.presentation.authority
        == vgmtooling::model::spatial_evidence_authority::inferred);

    // A role change inside the next block is represented at its exact frame
    // boundary. The prefix remains untouched and the fall is smoothed rather
    // than snapping the renderer control from foreground to background.
    const auto background = make_source(101, 3, 0.0f, 0.80f, 0.70f, 0.40f, 0.85f);
    const vgmtooling::model::spatial_source_evidence_event event{
        240,
        0,
        background,
    };
    vgmtooling::model::spatial_source_block_view changed_block{
        &lane,
        1,
        block_frames,
        &event,
        1,
    };
    assert(dsp.process(changed_block, sample_rate, output));
    assert(output.span_count() == 2);
    const auto& before_change = output.span(0);
    const auto& after_change = output.span(1);
    assert(before_change.frame_offset == 0 && before_change.frame_count == 240);
    assert(after_change.frame_offset == 240 && after_change.frame_count == 240);
    assert(near(before_change.target.foreground, 1.0f));
    assert(near(after_change.start.foreground, 1.0f));
    assert(near(after_change.target.foreground, 0.0f));
    assert(after_change.fall_coefficient > 0.0f && after_change.fall_coefficient < 1.0f);

    // The smoothed state is causal memory, not a precached soundtrack. The
    // following block starts from the result accumulated through the previous
    // samples and continues toward the current target.
    lane.evidence = background;
    vgmtooling::model::spatial_source_block_view carry_block{&lane, 1, block_frames};
    assert(dsp.process(carry_block, sample_rate, output));
    assert(output.span_count() == 1);
    assert(output.span(0).start.foreground > 0.0f);
    assert(output.span(0).start.foreground < 1.0f);
    assert(near(output.span(0).target.foreground, 0.0f));

    // Future events in the same transport block cannot rewrite an earlier
    // prefix. Two controllers with the same past produce the same control span
    // before their suffixes diverge.
    dsp_type causal_a{};
    dsp_type causal_b{};
    storage_type out_a{};
    storage_type out_b{};
    lane.evidence = source;
    assert(causal_a.process(first_block, sample_rate, out_a));
    assert(causal_b.process(first_block, sample_rate, out_b));

    const auto middle = make_source(101, 3, 0.50f, 0.45f, 0.20f, 0.0f, 0.90f);
    const auto late_a = make_source(101, 3, 0.10f, 0.90f, 0.80f, 0.50f, 0.90f);
    const auto late_b = make_source(101, 3, 0.95f, 0.10f, 0.05f, -0.50f, 0.90f);
    const vgmtooling::model::spatial_source_evidence_event events_a[] = {
        {120, 0, middle},
        {360, 0, late_a},
    };
    const vgmtooling::model::spatial_source_evidence_event events_b[] = {
        {120, 0, middle},
        {360, 0, late_b},
    };
    const vgmtooling::model::spatial_source_block_view causal_block_a{
        &lane, 1, block_frames, events_a, 2};
    const vgmtooling::model::spatial_source_block_view causal_block_b{
        &lane, 1, block_frames, events_b, 2};
    assert(causal_a.process(causal_block_a, sample_rate, out_a));
    assert(causal_b.process(causal_block_b, sample_rate, out_b));
    assert(out_a.span_count() == 3 && out_b.span_count() == 3);
    assert_same_prefix(out_a.span(0), out_b.span(0));
    assert_same_prefix(out_a.span(1), out_b.span(1));
    assert(out_a.span(2).target.foreground != out_b.span(2).target.foreground);

    // Reusing one physical lane for a new source episode is an identity
    // boundary. The old source's smoothed role must not bleed into the new one.
    dsp_type identity_dsp{};
    storage_type identity_output{};
    lane.evidence = source;
    assert(identity_dsp.process(first_block, sample_rate, identity_output));
    const auto replacement = make_source(202, 1, 0.20f, 0.90f, 0.80f, 0.70f, 0.95f);
    const vgmtooling::model::spatial_source_evidence_event replacement_event{100, 0, replacement};
    const vgmtooling::model::spatial_source_block_view replacement_block{
        &lane, 1, block_frames, &replacement_event, 1};
    assert(identity_dsp.process(replacement_block, sample_rate, identity_output));
    assert(identity_output.span_count() == 2);
    assert(identity_output.span(1).evidence.source_id == 202);
    assert(near(identity_output.span(1).start.foreground, 0.20f));
    assert(near(identity_output.span(1).target.foreground, 0.20f));

    // Malformed timelines fail closed and do not advance persistent state.
    dsp_type atomic_a{};
    dsp_type atomic_b{};
    storage_type atomic_out_a{};
    storage_type atomic_out_b{};
    lane.evidence = source;
    assert(atomic_a.process(first_block, sample_rate, atomic_out_a));
    assert(atomic_b.process(first_block, sample_rate, atomic_out_b));

    const vgmtooling::model::spatial_source_evidence_event bad_events[] = {
        {300, 0, middle},
        {200, 0, background},
    };
    const vgmtooling::model::spatial_source_block_view bad_block{
        &lane, 1, block_frames, bad_events, 2};
    assert(!atomic_a.process(bad_block, sample_rate, atomic_out_a));
    assert(!atomic_out_a.valid());

    const vgmtooling::model::spatial_source_evidence_event good_event{200, 0, middle};
    const vgmtooling::model::spatial_source_block_view good_block{
        &lane, 1, block_frames, &good_event, 1};
    assert(atomic_a.process(good_block, sample_rate, atomic_out_a));
    assert(atomic_b.process(good_block, sample_rate, atomic_out_b));
    assert(atomic_out_a.span_count() == atomic_out_b.span_count());
    for (std::size_t i = 0; i < atomic_out_a.span_count(); ++i)
        assert_same_prefix(atomic_out_a.span(i), atomic_out_b.span(i));

    // Invalid configuration is rejected without poisoning the active one.
    auto invalid_smoothing = dsp.smoothing();
    invalid_smoothing.rise_seconds = -1.0f;
    assert(!dsp.set_smoothing(invalid_smoothing));

    return 0;
}
