#include "model/persistent_part_observation_feature.h"
#include "model/persistent_part_hypothesis.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>

using namespace vgmtooling::model;

namespace {

node_id add_episode(musical_execution_graph& graph, const char* label, std::int64_t start) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = label;
    value.active = time_span{
        {time_domain::device, start, 32000, 0},
        time_coordinate{time_domain::device, start + 100, 32000, 0},
    };
    value.attributes.push_back({
        "identity_scope",
        std::string{"physical_voice_episode"},
        evidence_status::derived,
        1.0,
        "",
    });
    return graph.add_node(std::move(value));
}

node_id add_observation(
    musical_execution_graph& graph,
    node_id episode,
    edge_kind relation_kind,
    const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    const node_id event_id = graph.add_node(std::move(value));

    edge relation;
    relation.kind = relation_kind;
    relation.from = event_id;
    relation.to = episode;
    relation.provenance.push_back({
        evidence_status::derived,
        1.0,
        "fixture-observation",
        std::nullopt,
        "synthetic observation-to-episode bridge",
    });
    graph.add_edge(std::move(relation));
    return event_id;
}

node_id add_strong_part(
    musical_execution_graph& graph,
    node_id first,
    node_id second,
    const char* source) {
    auto hypothesis = make_persistent_part_hypothesis(
        0.88,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                source,
                "same source identity",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.84,
                source,
                "successive observations are temporally adjacent",
                {first, second},
            },
        });
    assert(hypothesis.confidence >= persistent_part_feature_threshold);
    return add_persistent_part_hypothesis(graph, hypothesis);
}

node_id add_weak_part(
    musical_execution_graph& graph,
    node_id first,
    node_id second) {
    auto hypothesis = make_persistent_part_hypothesis(
        0.95,
        {first, second},
        {{
            persistent_part_evidence_kind::physical_slot_continuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::derived,
            0.95,
            "fixture-slot-only",
            "same physical slot only",
            {first, second},
        }});
    assert(hypothesis.confidence < persistent_part_feature_threshold);
    return add_persistent_part_hypothesis(graph, hypothesis);
}

bool has_support_edge(const analysis_feature& feature, edge_id edge) {
    return std::find(feature.support_edges.begin(), feature.support_edges.end(), edge) !=
           feature.support_edges.end();
}

} // namespace

int main() {
    musical_execution_graph graph;

    const node_id a = add_episode(graph, "episode A", 0);
    const node_id b = add_episode(graph, "episode B", 120);
    const node_id strong_part = add_strong_part(graph, a, b, "fixture-strong");

    const analysis_feature direct = persistent_part_identity_from_observation(
        graph, a, "fixture-direct");
    assert(direct.availability == feature_availability::present);
    assert(std::get<std::uint64_t>(*direct.value) == strong_part);
    assert(direct.confidence.has_value() && *direct.confidence >= persistent_part_feature_threshold);

    const node_id event = add_observation(graph, a, edge_kind::realizes, "performance event");
    const auto bridges = graph.edges_from(event, edge_kind::realizes);
    assert(bridges.size() == 1);

    const analysis_feature through_event = persistent_part_identity_from_observation(
        graph, event, "fixture-event");
    assert(through_event.availability == feature_availability::present);
    assert(std::get<std::uint64_t>(*through_event.value) == strong_part);
    assert(has_support_edge(through_event, bridges[0]->id));

    const node_id c = add_episode(graph, "episode C", 300);
    const node_id d = add_episode(graph, "episode D", 420);
    (void)add_weak_part(graph, c, d);
    const analysis_feature weak = persistent_part_identity_from_observation(
        graph, c, "fixture-weak");
    assert(weak.availability == feature_availability::unknown);
    assert(!weak.value.has_value());

    const node_id e = add_episode(graph, "episode E", 600);
    const node_id f = add_episode(graph, "episode F", 720);
    const node_id g = add_episode(graph, "episode G", 840);
    (void)add_strong_part(graph, e, f, "fixture-branch-one");
    (void)add_strong_part(graph, e, g, "fixture-branch-two");
    const analysis_feature competing = persistent_part_identity_from_observation(
        graph, e, "fixture-competing");
    assert(competing.availability == feature_availability::unknown);
    assert(!competing.value.has_value());

    const node_id ambiguous_event = add_observation(
        graph, a, edge_kind::realizes, "ambiguous event");
    edge second_realization;
    second_realization.kind = edge_kind::realizes;
    second_realization.from = ambiguous_event;
    second_realization.to = c;
    graph.add_edge(std::move(second_realization));

    const analysis_feature ambiguous = persistent_part_identity_from_observation(
        graph, ambiguous_event, "fixture-ambiguous");
    assert(ambiguous.availability == feature_availability::unknown);
    assert(!ambiguous.value.has_value());

    node orphan;
    orphan.kind = node_kind::trace_event;
    orphan.layer = semantic_layer::synthesis;
    orphan.label = "orphan observation";
    const node_id orphan_id = graph.add_node(std::move(orphan));
    const analysis_feature unresolved = persistent_part_identity_from_observation(
        graph, orphan_id, "fixture-orphan");
    assert(unresolved.availability == feature_availability::unknown);
    assert(!unresolved.value.has_value());

    return 0;
}
