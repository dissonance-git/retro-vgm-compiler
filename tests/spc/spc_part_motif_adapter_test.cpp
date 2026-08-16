#include "components/spc/spc_part_motif_adapter.h"
#include "model/persistent_part_trajectory.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

namespace {

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    std::uint64_t voice) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = "S-DSP physical voice episode";
    value.active = time_span{
        {time_domain::device, start, 32000, 0},
        time_coordinate{time_domain::device, end, 32000, 0},
    };
    value.attributes.push_back({"physical_voice", voice, evidence_status::derived, 1.0, "slot"});
    return graph.add_node(std::move(value));
}

node_id add_sample(musical_execution_graph& graph) {
    node value;
    value.kind = node_kind::sample_buffer;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::value;
    value.label = "BRR runtime version";
    value.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    return graph.add_node(std::move(value));
}

node_id add_key_on(
    musical_execution_graph& graph,
    node_id episode,
    node_id sample,
    std::int64_t tick,
    std::uint64_t voice,
    std::uint64_t pitch_rate) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = "S-DSP key_on_accepted";
    event.active = time_span{{time_domain::device, tick, 32000, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{"key_on_accepted"}, evidence_status::exact, 1.0, ""});
    event.attributes.push_back({"physical_voice", voice, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"source_index", std::uint64_t{7}, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    const node_id event_id = graph.add_node(std::move(event));

    edge cause;
    cause.kind = edge_kind::causes;
    cause.from = event_id;
    cause.to = episode;
    graph.add_edge(std::move(cause));

    edge reference;
    reference.kind = edge_kind::references;
    reference.from = event_id;
    reference.to = sample;
    graph.add_edge(std::move(reference));
    return event_id;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id sample = add_sample(graph);
    const node_id first = add_episode(graph, 0, 3200, 0);
    const node_id second = add_episode(graph, 3520, 6400, 0);
    const node_id third = add_episode(graph, 6720, 9600, 0);

    add_key_on(graph, first, sample, 0, 0, 0x1000);
    add_key_on(graph, second, sample, 3520, 0, 0x1200);
    add_key_on(graph, third, sample, 6720, 0, 0x0f00);

    spc_part_continuity_policy policy;
    policy.max_gap_seconds = 0.02;
    policy.max_pitch_interval_octaves = 1.0;

    const auto first_link = infer_spc_persistent_part(
        graph, first, second, "synthetic-spc", policy);
    const auto second_link = infer_spc_persistent_part(
        graph, second, third, "synthetic-spc", policy);
    assert(first_link.confidence >= persistent_part_trajectory_link_threshold);
    assert(second_link.confidence >= persistent_part_trajectory_link_threshold);

    const auto trajectory = make_persistent_part_trajectory({first_link, second_link});
    const node_id part = add_persistent_part_trajectory(graph, trajectory);

    const auto observations = collect_spc_part_gestures(graph, part);
    assert(observations.size() == 3);
    assert(observations[0].part_id == part);
    const std::string expected_basis = "spc_brr_runtime_version:" + std::to_string(sample);
    assert(observations[0].pitch_basis == expected_basis);

    const auto profile = make_spc_part_motif_profile(graph, part);
    assert(profile.has_value());
    assert(profile->part_id == part);
    assert(profile->source_nodes.size() == 3);
    assert(profile->normalized_inter_onset_intervals.size() == 2);
    assert(profile->interval_octaves.has_value());
    assert(profile->interval_octaves->size() == 2);
    assert(profile->pitch_contour.has_value());
    assert(profile->pitch_basis == expected_basis);

    // With one exact BRR runtime version, unknown root tuning cancels in the
    // ratio, so this interval is legitimate without inventing an absolute note.
    const double expected = std::log2(0x1200 / static_cast<double>(0x1000));
    assert(std::fabs((*profile->interval_octaves)[0] - expected) < 1e-12);

    return 0;
}
