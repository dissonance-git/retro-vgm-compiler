#include "components/vgm/enhancement/genesis_part_motif_adapter.h"
#include "model/persistent_part_trajectory.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

namespace {

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    std::uint64_t fingerprint) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = "YM2612 physical voice episode";
    value.active = time_span{
        {time_domain::source, start, 0, 0},
        time_coordinate{time_domain::source, end, 0, 0},
    };
    value.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    value.attributes.push_back({"instrument_program_fingerprint", fingerprint, evidence_status::derived, 1.0, "fnv1a64"});
    return graph.add_node(std::move(value));
}

node_id add_onset(
    musical_execution_graph& graph,
    node_id episode,
    std::int64_t tick,
    std::uint16_t fnum,
    std::uint8_t block) {
    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = "YM2612 pitched_activity_onset";
    event.active = time_span{{time_domain::source, tick, 0, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_pitch_code", static_cast<std::uint64_t>(fnum), evidence_status::derived, 1.0, "device_native"});
    event.attributes.push_back({"device_pitch_block", static_cast<std::uint64_t>(block), evidence_status::derived, 1.0, "device_native"});
    const node_id event_id = graph.add_node(std::move(event));

    edge realization;
    realization.kind = edge_kind::realizes;
    realization.from = event_id;
    realization.to = episode;
    graph.add_edge(std::move(realization));
    return event_id;
}

} // namespace

int main() {
    ym2612_channel_state patch{};
    patch.algorithm = 4;
    patch.feedback = 5;
    patch.fms = 2;
    patch.operators[0].multiple = 2;
    patch.operators[1].total_level = 19;
    patch.operators[2].attack_rate = 27;
    const std::uint64_t fingerprint = ym2612_program_fingerprint(patch);

    musical_execution_graph graph;
    const node_id first = add_episode(graph, 0, 1000, fingerprint);
    const node_id second = add_episode(graph, 1100, 2000, fingerprint);
    const node_id third = add_episode(graph, 2100, 3000, fingerprint);

    add_onset(graph, first, 0, 0x300, 3);
    add_onset(graph, second, 1100, 0x360, 3);
    add_onset(graph, third, 2100, 0x330, 3);

    genesis_part_continuity_policy policy;
    policy.max_gap_ticks = 500;
    policy.max_pitch_interval_octaves = 1.0;

    const auto first_link = infer_genesis_persistent_part(
        graph, first, second, "synthetic-genesis", policy);
    const auto second_link = infer_genesis_persistent_part(
        graph, second, third, "synthetic-genesis", policy);
    assert(first_link.confidence >= persistent_part_trajectory_link_threshold);
    assert(second_link.confidence >= persistent_part_trajectory_link_threshold);

    const auto trajectory = make_persistent_part_trajectory({first_link, second_link});
    const node_id part = add_persistent_part_trajectory(graph, trajectory);

    const auto observations = collect_genesis_part_gestures(graph, part);
    assert(observations.size() == 3);
    assert(observations[0].source_node != 0);
    assert(observations[0].part_id == part);
    assert(observations[0].pitch_basis == "genesis_ym2612_relative_frequency_code");

    const auto profile = make_genesis_part_motif_profile(graph, part);
    assert(profile.has_value());
    assert(profile->part_id == part);
    assert(profile->source_nodes.size() == 3);
    assert(profile->normalized_inter_onset_intervals.size() == 2);
    assert(profile->interval_octaves.has_value());
    assert(profile->interval_octaves->size() == 2);
    assert(profile->pitch_contour.has_value());
    assert(profile->pitch_basis == "genesis_ym2612_relative_frequency_code");
    assert(profile->pitch_range_octaves.has_value());
    assert(*profile->pitch_range_octaves > 0.0);

    // The adapter preserves the frequency-code relation instead of rounding to
    // a MIDI note. Check the first source-relative interval directly.
    const double expected = std::log2(0x360 / static_cast<double>(0x300));
    assert(std::fabs((*profile->interval_octaves)[0] - expected) < 1e-12);

    return 0;
}
