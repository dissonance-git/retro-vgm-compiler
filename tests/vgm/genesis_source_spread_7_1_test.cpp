#include "components/vgm/enhancement/genesis_source_spread_7_1.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

using namespace gameaudio::vgm;
using vgmtooling::model::surround_7_1_bed_storage;
using vgmtooling::model::surround_7_1_channel;

bool near(float a, float b, float tolerance = 1.0e-5f)
{
    return std::fabs(a - b) <= tolerance;
}

std::size_t channel_index(surround_7_1_channel channel)
{
    return surround_7_1_bed_storage<2>::index(channel);
}

} // namespace

int main()
{
    selected_source_queue<genesis_recomposition_source_count, 4> queue;
    queue.reset(100u);

    for (std::uint64_t ordinal = 100u; ordinal < 102u; ++ordinal) {
        selected_source_frame<genesis_recomposition_source_count> frame{};
        frame.ordinal = ordinal;
        frame.source[0] = {1.0, 0.5, true, true};
        assert(queue.push_reference(frame));
    }

    selected_source_block_storage<genesis_recomposition_source_count, 2> selected;
    assert(selected.consume(queue, 100u, 2u));

    genesis_source_episode_block<4> episodes{};
    episodes.initial_depth.fill(genesis_episode_depth_slots[0]);
    episodes.events[0] = {
        1u, 0u, genesis_episode_depth_slots[5], 2u};
    episodes.event_count = 1u;
    episodes.valid = true;

    // The protected reference may contain other chips. Only the selected
    // Genesis lane is redistributed; the unmatched residual must stay front L/R.
    const double reference[4] = {1.25, 0.75, 1.25, 0.75};
    constexpr float passthrough_left = 0.25f;
    constexpr float passthrough_right = 0.25f;
    surround_7_1_bed_storage<2> bed;
    assert(project_genesis_source_spread_7_1(
        selected, episodes, reference, 2u, bed));

    const float* out = bed.data();
    assert(out != nullptr);

    const auto early =
        genesis_source_spread_gains_for_depth(genesis_episode_depth_slots[0]);
    const auto late =
        genesis_source_spread_gains_for_depth(genesis_episode_depth_slots[5]);

    const std::size_t f0 = 0u;
    const std::size_t f1 = vgmtooling::model::surround_7_1_channel_count;

    assert(near(
        out[f0 + channel_index(surround_7_1_channel::front_left)],
        passthrough_left + early.front));
    assert(near(
        out[f1 + channel_index(surround_7_1_channel::front_left)],
        passthrough_left + late.front));
    assert(near(early.front, late.front));

    assert(near(
        out[f0 + channel_index(surround_7_1_channel::side_left)],
        early.side));
    assert(near(
        out[f0 + channel_index(surround_7_1_channel::back_left)],
        early.back));
    assert(
        out[f0 + channel_index(surround_7_1_channel::side_left)]
        > out[f0 + channel_index(surround_7_1_channel::back_left)]);
    assert(
        out[f1 + channel_index(surround_7_1_channel::back_left)]
        > out[f1 + channel_index(surround_7_1_channel::side_left)]);

    const float early_power =
        early.front * early.front
        + early.side * early.side
        + early.back * early.back;
    const float late_power =
        late.front * late.front
        + late.side * late.side
        + late.back * late.back;
    assert(near(early_power, 1.0f));
    assert(near(late_power, 1.0f));

    assert(near(
        out[f0 + channel_index(surround_7_1_channel::front_right)],
        passthrough_right + 0.5f * early.front));
    assert(near(
        out[f1 + channel_index(surround_7_1_channel::back_right)],
        0.5f * late.back));

    assert(near(
        out[f0 + channel_index(surround_7_1_channel::front_center)],
        0.0f));
    assert(near(
        out[f0 + channel_index(surround_7_1_channel::lfe)],
        0.0f));

    return 0;
}
