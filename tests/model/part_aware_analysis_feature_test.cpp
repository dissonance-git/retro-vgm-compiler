#include "components/spc/spc_analysis_features.h"
#include "components/vgm/enhancement/genesis_analysis_features.h"
#include "model/persistent_part_hypothesis.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>

using namespace vgmtooling::model;

namespace {

node_id add_episode(musical_execution_graph& graph, std::int64_t start, const char* label) {
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

node_id add_strong_part(musical_execution_graph& graph, node_id first, node_id second) {
    const auto hypothesis = make_persistent_part_hypothesis(
        0.88,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                "fixture",
                "same source/program identity",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.84,
                "fixture",
                "adjacent observations",
                {first, second},
            },
        });
    return add_persistent_part_hypothesis(graph, hypothesis);
}

node_id add_genesis_event(musical_execution_graph& graph, node_id episode) {
    node event;
    event.kind = node_kind::musical_event;
    event.layer = semantic_layer::musical_performance;
    event.flow = flow_kind::event;
    event.label = "Genesis onset";
    event.attributes.push_back({"event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"device_family", std::string{"YM2612"}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    event.attributes.push_back({"physical_channel", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    const node_id event_id = graph.add_node(std::move(event));

    edge relation;
    relation.kind = edge_kind::realizes;
    relation.from = event_id;
    relation.to = episode;
    graph.add_edge(std::move(relation));
    return event_id;
}

node_id add_spc_event(musical_execution_graph& graph, node_id episode) {
    node event;
    event.kind = node_kind::trace_event;
    event.layer = semantic_layer::synthesis;
    event.flow = flow_kind::event;
    event.label = "S-DSP key_on_accepted";
    event.attributes.push_back({"event_kind", std::string{"key_on_accepted"}, evidence_status::exact, 1.0, ""});
    event.attributes.push_back({"physical_voice", std::uint64_t{0}, evidence_status::exact, 1.0, "slot"});
    const node_id event_id = graph.add_node(std::move(event));

    edge relation;
    relation.kind = edge_kind::causes;
    relation.from = event_id;
    relation.to = episode;
    graph.add_edge(std::move(relation));
    return event_id;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id first = add_episode(graph, 0, "episode A");
    const node_id second = add_episode(graph, 120, "episode B");
    const node_id part = add_strong_part(graph, first, second);

    const node_id genesis_event = add_genesis_event(graph, first);
    const auto genesis = gameaudio::vgm::extract_genesis_part_aware_performance_analysis_features(
        graph,
        genesis_event);
    const analysis_feature* genesis_part = genesis.find("persistent_part_identity");
    assert(genesis_part != nullptr);
    assert(genesis_part->availability == feature_availability::present);
    assert(std::get<std::uint64_t>(*genesis_part->value) == part);

    const node_id spc_event = add_spc_event(graph, second);
    const auto spc = gameaudio::spc::extract_spc_part_aware_runtime_analysis_features(
        graph,
        spc_event);
    const analysis_feature* spc_part = spc.find("persistent_part_identity");
    assert(spc_part != nullptr);
    assert(spc_part->availability == feature_availability::present);
    assert(std::get<std::uint64_t>(*spc_part->value) == part);

    node global;
    global.kind = node_kind::trace_event;
    global.layer = semantic_layer::synthesis;
    global.flow = flow_kind::event;
    global.label = "S-DSP execution reset";
    global.attributes.push_back({"event_kind", std::string{"execution_reset"}, evidence_status::exact, 1.0, ""});
    const node_id global_id = graph.add_node(std::move(global));
    const auto global_features = gameaudio::spc::extract_spc_part_aware_runtime_analysis_features(
        graph,
        global_id);
    const analysis_feature* global_part = global_features.find("persistent_part_identity");
    assert(global_part != nullptr);
    assert(global_part->availability == feature_availability::not_applicable);

    return 0;
}
