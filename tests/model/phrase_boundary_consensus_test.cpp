#include "model/phrase_boundary_consensus.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

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

phrase_boundary_hypothesis strong_part_boundary(
    std::int64_t tick,
    double timing_confidence,
    double motif_confidence,
    const char* source_prefix) {
    return make_phrase_boundary_hypothesis(
        {time_domain::source, tick, 0, 0},
        0.95,
        {
            {
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                evidence_status::derived,
                timing_confidence,
                std::string{source_prefix} + ":timing",
                "part-level temporal opening",
                {},
            },
            {
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::supports,
                evidence_status::hypothesis,
                motif_confidence,
                std::string{source_prefix} + ":motif",
                "motif closes immediately before the boundary",
                {},
            },
        });
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id melody = add_part(graph, "melody part");
    const node_id bass = add_part(graph, "bass part");
    const node_id inner = add_part(graph, "inner-voice part");

    const auto melody_boundary = strong_part_boundary(1000, 0.91, 0.88, "melody");
    const auto bass_boundary = strong_part_boundary(1010, 0.87, 0.84, "bass");
    const auto later_boundary = strong_part_boundary(1200, 0.90, 0.82, "inner");

    assert(close_enough(melody_boundary.confidence, 0.88));
    assert(close_enough(bass_boundary.confidence, 0.84));

    const auto single = make_phrase_boundary_consensus(
        {{melody, melody_boundary}},
        20);
    assert(!single.cross_part_grounded);
    assert(single.supporting_parts.size() == 1);
    assert(close_enough(single.independent_part_ceiling, 0.88));
    assert(close_enough(single.confidence, single_part_global_phrase_ceiling));

    const auto convergent = make_phrase_boundary_consensus(
        {
            {melody, melody_boundary},
            {bass, bass_boundary},
        },
        20);
    assert(convergent.cross_part_grounded);
    assert(convergent.supporting_parts.size() == 2);
    assert(convergent.representative.tick == 1005);
    assert(convergent.alignment_span.start.tick == 1000);
    assert(convergent.alignment_span.end.has_value());
    assert(convergent.alignment_span.end->tick == 1010);
    assert(close_enough(convergent.independent_part_ceiling, 0.84));
    assert(close_enough(convergent.confidence, 0.84));

    const auto groups = group_phrase_boundaries_across_parts(
        {
            {melody, melody_boundary},
            {bass, bass_boundary},
            {inner, later_boundary},
        },
        20);
    assert(groups.size() == 2);
    assert(groups[0].cross_part_grounded);
    assert(groups[0].representative.tick == 1005);
    assert(!groups[1].cross_part_grounded);
    assert(groups[1].representative.tick == 1200);

    const node_id boundary = add_phrase_boundary_consensus(graph, convergent);
    const node* materialized = graph.find_node(boundary);
    assert(materialized != nullptr);
    assert(materialized->kind == node_kind::musical_relation);
    assert(materialized->layer == semantic_layer::musical_structure);
    assert(materialized->active.has_value());
    assert(materialized->active->start.tick == 1000);
    assert(materialized->active->end.has_value());
    assert(materialized->active->end->tick == 1010);
    assert(graph.edges_to(boundary, edge_kind::derived_from).size() == 2);

    bool rejected = false;
    try {
        (void)make_phrase_boundary_consensus(
            {
                {melody, melody_boundary},
                {inner, later_boundary},
            },
            20);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);

    return 0;
}
