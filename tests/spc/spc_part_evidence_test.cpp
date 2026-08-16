#include "components/spc/spc_part_evidence.h"

#include <cassert>
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

node_id add_sample(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::sample_buffer;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::value;
    value.label = label;
    value.attributes.push_back({"encoding", std::string{"BRR"}, evidence_status::exact, 1.0, ""});
    return graph.add_node(std::move(value));
}

node_id add_key_on(
    musical_execution_graph& graph,
    node_id episode,
    node_id sample,
    std::int64_t tick,
    std::uint64_t voice,
    std::uint64_t source_index,
    std::uint64_t pitch_rate) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = "S-DSP key_on_accepted";
    event.active = time_span{{time_domain::device, tick, 32000, 0}, std::nullopt};
    event.attributes.push_back({"event_kind", std::string{"key_on_accepted"}, evidence_status::exact, 1.0, ""});
    event.attributes.push_back({"physical_voice", voice, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"source_index", source_index, evidence_status::exact, 1.0, "slot"});
    event.attributes.push_back({"pitch_rate", pitch_rate, evidence_status::exact, 1.0, "device_native"});
    event.attributes.push_back({"noise_enabled", false, evidence_status::exact, 1.0, ""});
    event.provenance.push_back({
        evidence_status::exact,
        1.0,
        "synthetic-spc",
        std::nullopt,
        "synthetic instrumented DSP observation",
        to_flags(provenance_flag::runtime_capture),
    });
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
    reference.attributes.push_back({
        "reference_kind",
        std::string{"runtime_sample_source"},
        evidence_status::derived,
        1.0,
        "",
    });
    graph.add_edge(std::move(reference));
    return event_id;
}

bool has_evidence(
    const persistent_part_hypothesis& hypothesis,
    persistent_part_evidence_kind kind,
    persistent_part_evidence_polarity polarity) {
    for (const auto& item : hypothesis.evidence) {
        if (item.kind == kind && item.polarity == polarity)
            return true;
    }
    return false;
}

const attribute* find_attr(const node& value, const char* key) {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id sample_a = add_sample(graph, "BRR sample A runtime version");
    const node_id sample_b = add_sample(graph, "BRR sample B runtime version");

    const node_id first = add_episode(graph, 0, 3200, 0);
    const node_id second = add_episode(graph, 3520, 6400, 0);
    const node_id overlap = add_episode(graph, 2000, 5000, 1);
    const node_id changed_source = add_episode(graph, 6720, 9000, 0);

    const node_id first_onset = add_key_on(graph, first, sample_a, 0, 0, 7, 0x1000);
    add_key_on(graph, second, sample_a, 3520, 0, 7, 0x1200);
    add_key_on(graph, overlap, sample_a, 2000, 1, 7, 0x1100);
    add_key_on(graph, changed_source, sample_b, 6720, 0, 7, 0x1200);

    spc_part_continuity_policy policy;
    policy.max_gap_seconds = 0.02;
    policy.max_pitch_interval_octaves = 1.0;

    const auto strong = infer_spc_persistent_part(
        graph,
        first,
        second,
        "synthetic-spc",
        policy);
    assert(strong.identity_bearing_support);
    assert(strong.cross_domain_grounded);
    assert(strong.confidence >= 0.75);
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::source_identity,
        persistent_part_evidence_polarity::supports));
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::temporal_adjacency,
        persistent_part_evidence_polarity::supports));
    assert(has_evidence(
        strong,
        persistent_part_evidence_kind::pitch_trajectory_continuity,
        persistent_part_evidence_polarity::supports));

    const node_id part = add_persistent_part_hypothesis(graph, strong);
    assert(graph.find_node(part) != nullptr);
    assert(graph.edges_to(part, edge_kind::groups_into).size() == 2);

    const auto conflict = infer_spc_persistent_part(
        graph,
        first,
        overlap,
        "synthetic-spc",
        policy);
    assert(conflict.strong_conflict_present);
    assert(conflict.confidence <= persistent_part_strong_conflict_confidence_ceiling);
    assert(has_evidence(
        conflict,
        persistent_part_evidence_kind::simultaneous_conflict,
        persistent_part_evidence_polarity::counters));

    const auto source_change = infer_spc_persistent_part(
        graph,
        second,
        changed_source,
        "synthetic-spc",
        policy);
    assert(!source_change.identity_bearing_support);
    assert(source_change.confidence <= persistent_part_no_identity_confidence_ceiling);
    assert(has_evidence(
        source_change,
        persistent_part_evidence_kind::identity_discontinuity,
        persistent_part_evidence_polarity::counters));

    // The performance lift exposes the runtime evidence at the same semantic
    // layer used by the Genesis adapter without inventing note names or parts.
    const auto performance_id = add_spc_performance_observation(graph, first_onset);
    assert(performance_id.has_value());
    const node* performance = graph.find_node(*performance_id);
    assert(performance != nullptr);
    assert(performance->kind == node_kind::musical_event);
    assert(performance->layer == semantic_layer::musical_performance);
    assert(std::get<std::string>(find_attr(*performance, "event_kind")->value) ==
           "source_activity_onset");
    assert(std::get<std::uint64_t>(find_attr(*performance, "runtime_sample_version_id")->value) ==
           sample_a);
    assert(std::get<std::string>(find_attr(*performance, "persistent_part_identity")->value) ==
           "unresolved");
    assert(graph.edges_from(*performance_id, edge_kind::realizes).size() == 1);
    assert(graph.edges_from(*performance_id, edge_kind::references).size() == 1);

    // Routing-only DSP events are not promoted into musical-performance events.
    node routing;
    routing.kind = node_kind::trace_event;
    routing.layer = semantic_layer::synthesis;
    routing.attributes.push_back({"event_kind", std::string{"routing_state_changed"}, evidence_status::exact, 1.0, ""});
    const node_id routing_id = graph.add_node(std::move(routing));
    assert(!add_spc_performance_observation(graph, routing_id).has_value());

    return 0;
}
