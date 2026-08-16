#include "model/motif_transformation_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

part_motif_profile profile(
    node_id part,
    std::vector<node_id> source_nodes,
    std::vector<double> rhythm,
    std::vector<double> intervals,
    std::vector<std::int8_t> contour,
    const char* basis = "synthetic") {
    part_motif_profile value;
    value.part_id = part;
    value.source_nodes = std::move(source_nodes);
    value.normalized_inter_onset_intervals = std::move(rhythm);
    value.interval_octaves = std::move(intervals);
    value.pitch_contour = std::move(contour);
    value.pitch_basis = basis;
    value.interval_semantics = "log2_frequency_ratio_octaves";
    return value;
}

std::vector<node_id> add_sources(musical_execution_graph& graph, int count, const char* label) {
    std::vector<node_id> ids;
    for (int index = 0; index < count; ++index) {
        node value;
        value.kind = node_kind::musical_event;
        value.layer = semantic_layer::musical_performance;
        value.flow = flow_kind::event;
        value.label = label;
        ids.push_back(graph.add_node(std::move(value)));
    }
    return ids;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const auto first_nodes = add_sources(graph, 4, "first motif event");
    const auto second_nodes = add_sources(graph, 4, "second motif event");

    const auto original = profile(
        10,
        first_nodes,
        {1.0, 1.0, 2.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "genesis-native");

    const auto recurrence = profile(
        20,
        second_nodes,
        {1.0, 1.0, 2.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "spc-native");
    const auto recurrence_hypothesis = infer_motif_transformation(original, recurrence);
    assert(recurrence_hypothesis.kind == motif_transformation_kind::near_recurrence);
    assert(std::fabs(recurrence_hypothesis.confidence - 1.0) < 1e-12);

    const auto rhythmic = profile(
        30,
        second_nodes,
        {0.5, 1.0, 2.5},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1});
    const auto rhythmic_hypothesis = infer_motif_transformation(original, rhythmic);
    assert(rhythmic_hypothesis.kind == motif_transformation_kind::rhythmic_variant);
    assert(rhythmic_hypothesis.confidence > 0.85);

    const auto interval_variant = profile(
        40,
        second_nodes,
        {1.0, 1.0, 2.0},
        {1.0 / 12.0, 3.0 / 12.0, -2.0 / 12.0},
        {1, 1, -1});
    const auto interval_hypothesis = infer_motif_transformation(original, interval_variant);
    assert(interval_hypothesis.kind == motif_transformation_kind::interval_variant);
    assert(interval_hypothesis.confidence > 0.80);

    part_motif_profile rhythm_only = recurrence;
    rhythm_only.interval_octaves.reset();
    rhythm_only.pitch_contour.reset();
    rhythm_only.pitch_basis.clear();
    rhythm_only.interval_semantics.clear();
    const auto rhythm_echo = infer_motif_transformation(original, rhythm_only);
    assert(rhythm_echo.kind == motif_transformation_kind::rhythm_only_echo);
    assert(std::fabs(rhythm_echo.confidence - rhythm_only_motif_identity_ceiling) < 1e-12);

    const node_id relation = add_motif_transformation_hypothesis(
        graph,
        recurrence_hypothesis);
    const node* relation_node = graph.find_node(relation);
    assert(relation_node != nullptr);
    assert(relation_node->kind == node_kind::musical_relation);
    assert(relation_node->layer == semantic_layer::musical_structure);
    assert(graph.edges_to(relation, edge_kind::derived_from).size() == 8);

    return 0;
}
