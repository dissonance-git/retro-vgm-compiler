#include "model/musical_execution_graph.h"

#include <cassert>
#include <set>
#include <string>
#include <utility>

using namespace vgmtooling::model;

namespace {

node_id add_stream(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::acoustic_contribution;
    value.layer = semantic_layer::acoustic_realization;
    value.flow = flow_kind::stream;
    value.label = label;
    return graph.add_node(std::move(value));
}

node_id add_operation(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::bus;
    value.layer = semantic_layer::acoustic_realization;
    value.flow = flow_kind::stream;
    value.label = label;
    value.provenance.push_back({
        evidence_status::derived,
        1.0,
        "transformation-identity-regression",
        std::nullopt,
        "reified processing identity for grouped input/output incidence",
    });
    return graph.add_node(std::move(value));
}

edge_id route(musical_execution_graph& graph, node_id from, node_id to) {
    edge value;
    value.kind = edge_kind::routes_to;
    value.from = from;
    value.to = to;
    return graph.add_edge(std::move(value));
}

std::set<std::string> incoming_labels(const musical_execution_graph& graph, node_id operation) {
    std::set<std::string> result;
    for (const auto* relation : graph.edges_to(operation, edge_kind::routes_to)) {
        const auto* source = graph.find_node(relation->from);
        assert(source != nullptr);
        result.insert(source->label);
    }
    return result;
}

std::set<std::string> outgoing_labels(const musical_execution_graph& graph, node_id operation) {
    std::set<std::string> result;
    for (const auto* relation : graph.edges_from(operation, edge_kind::routes_to)) {
        const auto* target = graph.find_node(relation->to);
        assert(target != nullptr);
        result.insert(target->label);
    }
    return result;
}

using endpoint_pair = std::pair<std::string, std::string>;

std::set<endpoint_pair> endpoint_projection(const musical_execution_graph& graph) {
    std::set<endpoint_pair> result;
    for (const auto* operation : graph.nodes_of_kind(node_kind::bus)) {
        const auto inputs = incoming_labels(graph, operation->id);
        const auto outputs = outgoing_labels(graph, operation->id);
        for (const auto& input : inputs) {
            for (const auto& output : outputs) {
                result.emplace(input, output);
            }
        }
    }
    return result;
}

} // namespace

int main() {
    // H1: one joint many-input / many-output transformation.
    musical_execution_graph joint;
    const auto joint_a = add_stream(joint, "A");
    const auto joint_b = add_stream(joint, "B");
    const auto joint_l = add_stream(joint, "L");
    const auto joint_r = add_stream(joint, "R");
    const auto mixer = add_operation(joint, "MIX");

    route(joint, joint_a, mixer);
    route(joint, joint_b, mixer);
    route(joint, mixer, joint_l);
    route(joint, mixer, joint_r);

    const auto joint_operations = joint.nodes_of_kind(node_kind::bus);
    assert(joint_operations.size() == 1);
    assert(incoming_labels(joint, mixer) == std::set<std::string>({"A", "B"}));
    assert(outgoing_labels(joint, mixer) == std::set<std::string>({"L", "R"}));
    assert(joint.edges().size() == 4);

    // H2: two independent processors with the same endpoint projection.
    musical_execution_graph split;
    const auto split_a = add_stream(split, "A");
    const auto split_b = add_stream(split, "B");
    const auto split_l = add_stream(split, "L");
    const auto split_r = add_stream(split, "R");
    const auto p1 = add_operation(split, "P1");
    const auto p2 = add_operation(split, "P2");

    route(split, split_a, p1);
    route(split, p1, split_l);
    route(split, p1, split_r);
    route(split, split_b, p2);
    route(split, p2, split_l);
    route(split, p2, split_r);

    const auto split_operations = split.nodes_of_kind(node_kind::bus);
    assert(split_operations.size() == 2);
    assert(incoming_labels(split, p1) == std::set<std::string>({"A"}));
    assert(incoming_labels(split, p2) == std::set<std::string>({"B"}));
    assert(outgoing_labels(split, p1) == std::set<std::string>({"L", "R"}));
    assert(outgoing_labels(split, p2) == std::set<std::string>({"L", "R"}));
    assert(split.edges().size() == 6);

    const std::set<endpoint_pair> expected_projection{
        {"A", "L"},
        {"A", "R"},
        {"B", "L"},
        {"B", "R"},
    };

    // Flattening both histories to source/output endpoint pairs aliases them.
    assert(endpoint_projection(joint) == expected_projection);
    assert(endpoint_projection(split) == expected_projection);
    assert(endpoint_projection(joint) == endpoint_projection(split));

    // Reified operation identity/incidence remains different and recoverable.
    assert(joint_operations.size() != split_operations.size());

    return 0;
}
