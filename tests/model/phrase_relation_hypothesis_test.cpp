#include "model/phrase_relation_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

node_id add_part(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::part;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = label;
    value.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        0.9,
        "",
    });
    return graph.add_node(std::move(value));
}

phrase_boundary_consensus boundary(
    std::int64_t tick,
    double confidence,
    const std::vector<node_id>& parts) {
    phrase_boundary_consensus result;
    result.representative = {time_domain::source, tick, 0, 0};
    result.alignment_span = time_span{
        result.representative,
        result.representative,
    };
    result.confidence = confidence;
    result.independent_part_ceiling = confidence;
    result.supporting_parts = parts;
    result.cross_part_grounded = parts.size() >= 2;
    return result;
}

node_id add_event(musical_execution_graph& graph, std::int64_t tick, const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::event;
    value.label = label;
    value.active = time_span{{time_domain::source, tick, 0, 0}, std::nullopt};
    return graph.add_node(std::move(value));
}

part_motif_profile profile(
    node_id part,
    std::vector<node_id> sources,
    std::vector<double> rhythm,
    std::vector<double> intervals,
    std::vector<std::int8_t> contour,
    const char* native_basis) {
    part_motif_profile result;
    result.part_id = part;
    result.source_nodes = std::move(sources);
    result.normalized_inter_onset_intervals = std::move(rhythm);
    result.interval_octaves = std::move(intervals);
    result.pitch_contour = std::move(contour);
    result.pitch_basis = native_basis;
    result.interval_semantics = "log2_frequency_ratio_octaves";
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id melody = add_part(graph, "melody");
    const node_id bass = add_part(graph, "bass");
    const std::vector<node_id> parts{melody, bass};

    const auto b0 = boundary(0, 0.90, parts);
    const auto b1 = boundary(400, 0.86, parts);
    const auto b2 = boundary(1000, 0.84, parts);

    const auto phrase_a = make_phrase_region_hypothesis(b0, b1);
    const auto phrase_b = make_phrase_region_hypothesis(b1, b2);
    assert(close_enough(phrase_a.confidence, 0.86));
    assert(close_enough(phrase_b.confidence, 0.84));
    assert(phrase_a.cross_part_grounded);
    assert(phrase_b.cross_part_grounded);

    const node_id phrase_a_id = add_phrase_region_hypothesis(graph, phrase_a);
    const node_id phrase_b_id = add_phrase_region_hypothesis(graph, phrase_b);

    std::vector<node_id> a_nodes{
        add_event(graph, 0, "A1"),
        add_event(graph, 100, "A2"),
        add_event(graph, 200, "A3"),
        add_event(graph, 300, "A4"),
    };
    std::vector<node_id> b_nodes{
        add_event(graph, 400, "B1"),
        add_event(graph, 500, "B2"),
        add_event(graph, 600, "B3"),
        add_event(graph, 800, "B4"),
    };

    const auto motif_a = profile(
        melody,
        a_nodes,
        {1.0, 1.0, 1.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "genesis-native");
    const auto motif_b = profile(
        melody,
        b_nodes,
        {1.0, 1.0, 1.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "spc-native");

    const auto recurrence = infer_motif_transformation(motif_a, motif_b);
    assert(recurrence.kind == motif_transformation_kind::near_recurrence);
    const auto phrase_recurrence = infer_phrase_relation(
        graph,
        phrase_a_id,
        phrase_b_id,
        recurrence);
    assert(phrase_recurrence.kind == phrase_relation_kind::recurrence);
    assert(close_enough(phrase_recurrence.confidence, 0.84));

    const edge_id recurrence_edge = add_phrase_relation_hypothesis(
        graph,
        phrase_recurrence);
    const edge* materialized = graph.find_edge(recurrence_edge);
    assert(materialized != nullptr);
    assert(materialized->kind == edge_kind::repeats);
    assert(materialized->from == phrase_a_id);
    assert(materialized->to == phrase_b_id);

    const auto changed_b = profile(
        melody,
        b_nodes,
        {0.5, 1.0, 1.5},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "spc-native");
    const auto rhythmic_variant = infer_motif_transformation(motif_a, changed_b);
    assert(rhythmic_variant.kind == motif_transformation_kind::rhythmic_variant);
    const auto varied = infer_phrase_relation(
        graph,
        phrase_a_id,
        phrase_b_id,
        rhythmic_variant);
    assert(varied.kind == phrase_relation_kind::varied_recurrence);
    const edge_id varied_edge = add_phrase_relation_hypothesis(graph, varied);
    const edge* varied_materialized = graph.find_edge(varied_edge);
    assert(varied_materialized != nullptr);
    assert(varied_materialized->kind == edge_kind::transforms);

    // A motif cannot be used to claim A -> A' if its source events do not
    // actually live inside the proposed phrase regions.
    std::vector<node_id> outside_nodes{
        add_event(graph, 1100, "outside1"),
        add_event(graph, 1200, "outside2"),
        add_event(graph, 1300, "outside3"),
        add_event(graph, 1400, "outside4"),
    };
    const auto outside = profile(
        melody,
        outside_nodes,
        {1.0, 1.0, 1.0},
        {2.0 / 12.0, 2.0 / 12.0, -3.0 / 12.0},
        {1, 1, -1},
        "other-native");
    const auto outside_relation = infer_motif_transformation(motif_a, outside);

    bool rejected = false;
    try {
        (void)infer_phrase_relation(
            graph,
            phrase_a_id,
            phrase_b_id,
            outside_relation);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    return 0;
}
